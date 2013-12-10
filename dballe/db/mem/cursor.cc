/*
 * db/mem/cursor - iterate results of queries on mem databases
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "cursor.h"
#include "db.h"
#include "dballe/db/modifiers.h"
#include "dballe/core/record.h"
#include <queue>
#include <stack>

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {
namespace db {
namespace mem {

Cursor::Cursor(mem::DB& db, unsigned modifiers)
    : db(db), modifiers(modifiers), count(0), cur_station(0), cur_value(0), cur_var(0)
{
}

Cursor::~Cursor()
{
}

dballe::DB& Cursor::get_db() const { return db; }

int Cursor::remaining() const
{
    return count;
}

int Cursor::attr_reference_id() const
{
    // Default implementation for cursors for which this does not make any sense
    return 0;
}

int Cursor::get_station_id() const { return cur_station->id; }
double Cursor::get_lat() const { return cur_station->coords.dlat(); }
double Cursor::get_lon() const { return cur_station->coords.dlon(); }
const char* Cursor::get_ident(const char* def) const
{
    if (cur_station->mobile)
        return cur_station->ident.c_str();
    else
        return def;
}
const char* Cursor::get_rep_memo(const char* def) const { return cur_station->report.c_str(); }

Level Cursor::get_level() const
{
    if (cur_value)
        return cur_value->levtr.level;
    else
        return Level();
}
Trange Cursor::get_trange() const
{
    if (cur_value)
        return cur_value->levtr.trange;
    else
        return Trange();
}
void Cursor::get_datetime(int (&dt)[6]) const
{
    if (cur_value)
    {
        dt[0] = cur_value->datetime.date.year;
        dt[1] = cur_value->datetime.date.month;
        dt[2] = cur_value->datetime.date.day;
        dt[3] = cur_value->datetime.time.hour;
        dt[4] = cur_value->datetime.time.minute;
        dt[5] = cur_value->datetime.time.second;
    } else {
        dt[0] = 1000;
        dt[1] = 1;
        dt[2] = 1;
        dt[3] = 0;
        dt[4] = 0;
        dt[5] = 0;
    }
}
wreport::Varcode Cursor::get_varcode() const
{
    if (cur_var)
        return cur_var->code();
    else
        return (wreport::Varcode)0;
}
wreport::Var Cursor::get_var() const
{
    if (cur_var)
        return Var(*cur_var, false);
    else
        throw error_consistency("get_var not supported on this query");
}

void Cursor::to_record_station(Record& rec)
{
    rec.set(DBA_KEY_ANA_ID, (int)cur_station->id);
    rec.set(cur_station->coords);
    if (cur_station->mobile)
    {
        rec.set(DBA_KEY_IDENT, cur_station->ident);
        rec.key(DBA_KEY_MOBILE).seti(1);
    } else {
        rec.key_unset(DBA_KEY_IDENT);
        rec.key(DBA_KEY_MOBILE).seti(0);
    }
    rec.set(DBA_KEY_REP_MEMO, cur_station->report);
    rec.set(DBA_KEY_PRIORITY, db.repinfo.get_prio(cur_station->report));
}

void Cursor::to_record_levtr(Record& rec)
{
    rec.set(cur_value->levtr.level);
    rec.set(cur_value->levtr.trange);
}

void Cursor::to_record_varcode(Record& rec)
{
    wreport::Varcode code = cur_var->code();
    char bname[7];
    snprintf(bname, 7, "B%02d%03d", WR_VAR_X(code), WR_VAR_Y(code));
    rec.key(DBA_KEY_VAR).setc(bname);
}

void Cursor::to_record_value(Record& rec)
{
    to_record_levtr(rec);
    rec.set(cur_value->datetime);
    to_record_varcode(rec);
    rec.set(*cur_var);
}

unsigned Cursor::query_attrs(const std::vector<wreport::Varcode>& qcs, Record& attrs)
{
    // Default implementation for cursors for which this does not make any sense
    return 0;
}

namespace {

struct StationVarInserter
{
    Record& rec;

    StationVarInserter(Record& rec) : rec(rec) {}

    void insert(const StationValue* val)
    {
        rec.set(*val->var);
    }
};

}

void Cursor::add_station_info(Record& rec)
{
    db.memdb.stationvalues.fill_record(*cur_station, rec);
}

namespace {

struct StationResultQueue : public stack<const Station*>
{
    StationResultQueue(DB&, Results<Station>& res)
    {
        res.copy_valptrs_to(stl::pusher(*this));
    }
};

struct StationValueResultQueue : public stack<const StationValue*>
{
    StationValueResultQueue(DB&, Results<StationValue>& res)
    {
        res.copy_valptrs_to(stl::pusher(*this));
    }
};

struct CompareData
{
    const ValueStorage<Value>& values;

    CompareData(const ValueStorage<Value>& values) : values(values) {}

    // Return an inverse comparison, so that the priority queue gives us the
    // smallest items first
    bool operator() (size_t x, size_t y) const
    {
        const Value& vx = *values[x];
        const Value& vy = *values[y];

        if (vx.station.coords < vy.station.coords) return false;
        if (vx.station.coords > vy.station.coords) return true;
        if (vx.station.ident < vy.station.ident) return false;
        if (vx.station.ident > vy.station.ident) return true;

        if (int res = vx.datetime.compare(vy.datetime)) return res > 0;
        if (int res = vx.levtr.level.compare(vy.levtr.level)) return res > 0;
        if (int res = vx.levtr.trange.compare(vy.levtr.trange)) return res > 0;

        return vx.station.report > vy.station.report;
    }
};

struct DataResultQueue : public priority_queue<size_t, vector<size_t>, CompareData>
{
    DataResultQueue(DB&, Results<Value>& res)
        : priority_queue<size_t, vector<size_t>, CompareData>(CompareData(res.values))
    {
        res.copy_indices_to(stl::pusher(*this));
    }
};

struct DataBestKey
{
    const ValueStorage<Value>& values;
    size_t idx;

    DataBestKey(const ValueStorage<Value>& values, size_t idx)
        : values(values), idx(idx) {}

    const Value& value() const { return *values[idx]; }

    // Normal sort here, but we ignore report so that two values which only
    // differ in report are considered the same
    bool operator<(const DataBestKey& o) const
    {
        const Value& vx = value();
        const Value& vy = o.value();

        if (vx.station.coords < vy.station.coords) return true;
        if (vx.station.coords > vy.station.coords) return false;
        if (vx.station.ident < vy.station.ident) return true;
        if (vx.station.ident > vy.station.ident) return false;

        if (int res = vx.datetime.compare(vy.datetime)) return res < 0;
        if (int res = vx.levtr.level.compare(vy.levtr.level)) return res < 0;
        if (int res = vx.levtr.trange.compare(vy.levtr.trange)) return res < 0;

        // They are the same
        return false;
    }
};

struct DataBestResultQueue : public map<DataBestKey, size_t>
{
    const ValueStorage<Value>& values;
    std::map<std::string, int> prios;

    DataBestResultQueue(DB& db, Results<Value>& res)
        : values(res.values), prios(db.get_repinfo_priorities())
    {
        res.copy_indices_to(stl::pusher(*this));
    }

    /**
     * Add val to the map, but in case of conflict it only keeps the value with
     * the highest priority.
     */
    void push(size_t val)
    {
        iterator i = find(DataBestKey(values, val));
        if (i == end())
            insert(make_pair(DataBestKey(values, val), val));
        else
        {
            const Value& vx = i->first.value();
            const Value& vy = *values[val];
            if (prios[vx.station.report] < prios[vy.station.report])
                i->second = val;
        }
    }

    size_t top() const
    {
        return begin()->second;
    }

    void pop()
    {
        erase(begin());
    }
};

}

template<typename QUEUE>
struct CursorSorted : public Cursor
{
    QUEUE queue;
    bool first;

    bool next()
    {
        if (count == 0 || queue.empty())
            return false;

        if (first)
            first = false;
        else
            queue.pop();

        --count;
        return !queue.empty();
    }

    void discard_rest()
    {
        // We cannot call queue.clear()
        // FIXME: we can probably get rid of discard_rest() and do proper
        // review and destructor work. This is only used to flush DB cursors to
        // avoid shutting down a partial query for some databases. But really,
        // is it needed at all?
        count = 0;
    }

    template<typename T>
    CursorSorted(mem::DB& db, unsigned modifiers, T& res)
        : Cursor(db, modifiers), queue(db, res), first(true)
    {
        count = queue.size();
    }
};

struct CursorStations : public CursorSorted<StationResultQueue>
{
    CursorStations(mem::DB& db, unsigned modifiers, Results<Station>& res)
        : CursorSorted<StationResultQueue>(db, modifiers, res)
    {
    }

    bool next()
    {
        if (CursorSorted<StationResultQueue>::next())
        {
            this->cur_station = queue.top();
            this->cur_value = 0;
            this->cur_var = 0;
            return true;
        }
        return false;
    }

    void to_record(Record& rec)
    {
        this->to_record_station(rec);
        this->add_station_info(rec);
    }
};

struct CursorStationData : public CursorSorted<StationValueResultQueue>
{
    CursorStationData(mem::DB& db, unsigned modifiers, Results<StationValue>& res)
        : CursorSorted<StationValueResultQueue>(db, modifiers, res)
    {
    }

    bool next()
    {
        if (CursorSorted<StationValueResultQueue>::next())
        {
            cur_var = queue.top()->var;
            cur_station = &(queue.top()->station);
            return true;
        }
        return false;
    }

    void to_record(Record& rec)
    {
        to_record_station(rec);
        to_record_varcode(rec);
        rec.set(*cur_var);
    }
};

template<typename QUEUE>
struct CursorDataBase : public CursorSorted<QUEUE>
{
    const ValueStorage<Value>& values;
    size_t cur_idx;

    CursorDataBase(mem::DB& db, unsigned modifiers, Results<Value>& res)
        : CursorSorted<QUEUE>(db, modifiers, res), values(res.values)
    {
    }

    int attr_reference_id() const
    {
        return cur_idx;
    }

    bool next()
    {
        if (CursorSorted<QUEUE>::next())
        {
            this->cur_idx = this->queue.top();
            this->cur_value = this->values[this->cur_idx];
            this->cur_var = this->cur_value->var;
            this->cur_station = &(this->cur_value->station);
            return true;
        }
        return false;
    }

    void to_record(Record& rec)
    {
        this->to_record_station(rec);
        rec.clear_vars();
        this->to_record_value(rec);
        rec.key(DBA_KEY_CONTEXT_ID).seti(this->cur_idx);
        if (this->modifiers & DBA_DB_MODIFIER_ANAEXTRA)
            this->add_station_info(rec);
    }

    unsigned query_attrs(const AttrList& qcs, Record& attrs)
    {
        return this->db.query_attrs(this->cur_idx, 0, qcs, attrs);
    }
};

struct CursorData : public CursorDataBase<DataResultQueue>
{
    CursorData(mem::DB& db, unsigned modifiers, Results<Value>& res)
        : CursorDataBase<DataResultQueue>(db, modifiers, res)
    {
    }
};

struct CursorDataBest : public CursorDataBase<DataBestResultQueue>
{
    CursorDataBest(mem::DB& db, unsigned modifiers, Results<Value>& res)
        : CursorDataBase<DataBestResultQueue>(db, modifiers, res)
    {
    }
};

struct CursorSummary : public Cursor
{
    memdb::Summary values;
    memdb::Summary::const_iterator iter_cur;
    memdb::Summary::const_iterator iter_end;
    bool first;

    CursorSummary(mem::DB& db, unsigned modifiers, Results<Value>& res)
        : Cursor(db, modifiers), first(true)
    {
        memdb::Summarizer summarizer(values);
        res.copy_valptrs_to(stl::trivial_inserter(summarizer));

        // Start iterating at the beginning
        iter_cur = values.begin();
        iter_end = values.end();

        // Initialize the result count
        count = values.size();
    }

    bool next()
    {
        if (iter_cur == iter_end)
            return false;

        if (first)
            first = false;
        else
            ++iter_cur;

        --count;
        if (iter_cur != iter_end)
        {
            cur_value = &(iter_cur->first.sample);
            cur_var = cur_value->var;
            cur_station = &(iter_cur->first.sample.station);
            return true;
        }
        return false;
    }

    void discard_rest()
    {
        iter_cur = iter_end;
        count = 0;
    }

    void to_record(Record& rec)
    {
        to_record_station(rec);
        to_record_levtr(rec);
        to_record_varcode(rec);
        // TODO: Add stats
    }
};

auto_ptr<db::Cursor> Cursor::createStations(mem::DB& db, unsigned modifiers, Results<Station>& res)
{
    return auto_ptr<db::Cursor>(new CursorStations(db, modifiers, res));
}

auto_ptr<db::Cursor> Cursor::createStationData(mem::DB& db, unsigned modifiers, Results<StationValue>& res)
{
    return auto_ptr<db::Cursor>(new CursorStationData(db, modifiers, res));
}

auto_ptr<db::Cursor> Cursor::createData(mem::DB& db, unsigned modifiers, Results<Value>& res)
{
    return auto_ptr<db::Cursor>(new CursorData(db, modifiers, res));
}

auto_ptr<db::Cursor> Cursor::createDataBest(mem::DB& db, unsigned modifiers, Results<Value>& res)
{
    return auto_ptr<db::Cursor>(new CursorDataBest(db, modifiers, res));
}

auto_ptr<db::Cursor> Cursor::createSummary(mem::DB& db, unsigned modifiers, Results<Value>& res)
{
    return auto_ptr<db::Cursor>(new CursorSummary(db, modifiers, res));
}




#if 0
unsigned CursorStations::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %02.4f %02.4f %-10s\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_LAT, 0.0),
                    r.get(DBA_KEY_LON, 0.0),
                    r.get(DBA_KEY_IDENT, ""));
        }
    }
    return count;
}

unsigned CursorData::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
/*
            to_record(r);
            fprintf(dump, "%02d %06d %06d %-10s\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_LAT, 0.0),
                    r.get(DBA_KEY_LON, 0.0),
                    r.get(DBA_KEY_IDENT, ""));
                    */
        }
    }
    return count;
}

void CursorSummary::to_record(Record& rec)
{
    to_record_pseudoana(rec);
    to_record_repinfo(rec);
    //rec.key(DBA_KEY_CONTEXT_ID).seti(sqlrec.out_id_data);
    to_record_varcode(rec);
    to_record_ltr(rec);

    /*
    // Min datetime
    rec.key(DBA_KEY_YEARMIN).seti(sqlrec.out_datetime.year);
    rec.key(DBA_KEY_MONTHMIN).seti(sqlrec.out_datetime.month);
    rec.key(DBA_KEY_DAYMIN).seti(sqlrec.out_datetime.day);
    rec.key(DBA_KEY_HOURMIN).seti(sqlrec.out_datetime.hour);
    rec.key(DBA_KEY_MINUMIN).seti(sqlrec.out_datetime.minute);
    rec.key(DBA_KEY_SECMIN).seti(sqlrec.out_datetime.second);

    // Max datetime
    rec.key(DBA_KEY_YEARMAX).seti(out_datetime_max.year);
    rec.key(DBA_KEY_MONTHMAX).seti(out_datetime_max.month);
    rec.key(DBA_KEY_DAYMAX).seti(out_datetime_max.day);
    rec.key(DBA_KEY_HOURMAX).seti(out_datetime_max.hour);
    rec.key(DBA_KEY_MINUMAX).seti(out_datetime_max.minute);
    rec.key(DBA_KEY_SECMAX).seti(out_datetime_max.second);

    // Abuse id_data and datetime for count and min(datetime)
    rec.key(DBA_KEY_LIMIT).seti(sqlrec.out_id_data);
    */
}

unsigned CursorSummary::test_iterate(FILE* dump)
{
    Record r;
    unsigned count;
    for (count = 0; next(); ++count)
    {
        if (dump)
        {
            to_record(r);
            fprintf(dump, "%02d %03d %03d %s %04d-%02d-%02d %02d:%02d:%02d  %04d-%02d-%02d %02d:%02d:%02d  %d\n",
                    r.get(DBA_KEY_ANA_ID, -1),
                    r.get(DBA_KEY_REP_COD, -1),
                    (int)sqlrec.out_id_ltr,
                    r.get(DBA_KEY_VAR, ""),
                    r.get(DBA_KEY_YEARMIN, 0), r.get(DBA_KEY_MONTHMIN, 0), r.get(DBA_KEY_DAYMIN, 0),
                    r.get(DBA_KEY_HOURMIN, 0), r.get(DBA_KEY_MINUMIN, 0), r.get(DBA_KEY_SECMIN, 0),
                    r.get(DBA_KEY_YEARMAX, 0), r.get(DBA_KEY_MONTHMAX, 0), r.get(DBA_KEY_DAYMAX, 0),
                    r.get(DBA_KEY_HOURMAX, 0), r.get(DBA_KEY_MINUMAX, 0), r.get(DBA_KEY_SECMAX, 0),
                    r.get(DBA_KEY_LIMIT, -1));
        }
    }
    return count;
}
#endif

}
}
}
#include "dballe/memdb/core.tcc"
#include "dballe/memdb/query.tcc"
