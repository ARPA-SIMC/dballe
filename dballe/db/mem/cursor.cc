#include "cursor.h"
#include "db.h"
#include "dballe/core/record.h"
#include "dballe/core/query.h"
#include <queue>
#include <stack>
#include <sstream>

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {
namespace db {
namespace mem {

namespace cursor {

const memdb::Value& DataBestKey::value() const { return *values[idx]; }

bool DataBestKey::operator<(const DataBestKey& o) const
{
    // Normal sort here, but we ignore report so that two values which only
    // differ in report are considered the same
    const memdb::Value& vx = value();
    const memdb::Value& vy = o.value();

    if (int res = vx.station.coords.compare(vy.station.coords)) return res < 0;
    if (vx.station.ident < vy.station.ident) return true;
    if (vx.station.ident > vy.station.ident) return false;

    if (int res = vx.datetime.compare(vy.datetime)) return res < 0;
    if (int res = vx.levtr.level.compare(vy.levtr.level)) return res < 0;
    if (int res = vx.levtr.trange.compare(vy.levtr.trange)) return res < 0;

    if (int res = vx.var->code() - vy.var->code()) return res < 0;

    // They are the same
    return false;
}

std::ostream& operator<<(std::ostream& out, const DataBestKey& k)
{
    const memdb::Value& v = k.value();

    out << v.station.coords
        << "." << v.station.ident
        << ":" << v.levtr.level
        << ":" << v.levtr.trange
        << ":" << v.datetime
        << ":" << varcode_format(v.var->code());
    return out;
}

}

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
    return MISSING_INT;
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
const char* Cursor::get_rep_memo() const { return cur_station->report.c_str(); }

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
Datetime Cursor::get_datetime() const
{
    if (cur_value)
    {
        return cur_value->datetime;
    } else {
        return Datetime();
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
    rec.set("ana_id", (int)cur_station->id);
    core::Record::downcast(rec).set_coords(cur_station->coords);
    if (cur_station->mobile)
    {
        rec.set("ident", cur_station->ident);
        rec.set("mobile", 1);
    } else {
        rec.unset("ident");
        rec.set("mobile", 0);
    }
    rec.set("rep_memo", cur_station->report);
    rec.set("priority", db.repinfo.get_prio(cur_station->report));
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
    rec.setc("var", bname);
}

void Cursor::to_record_value(Record& rec)
{
    to_record_levtr(rec);
    rec.set(cur_value->datetime);
    to_record_varcode(rec);
    rec.set(*cur_var);
}

void Cursor::query_attrs(function<void(unique_ptr<Var>&&)> dest)
{
    // Default implementation for cursors for which this does not make any sense
}

namespace {

struct StationVarInserter
{
    Record& rec;

    StationVarInserter(Record& rec) : rec(rec) {}

    void insert(const memdb::StationValue* val)
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

struct StationResultQueue : public stack<const memdb::Station*>
{
    StationResultQueue(DB&, Results<memdb::Station>& res)
    {
        res.copy_valptrs_to(stl::pusher(*this));
    }
};

struct CompareStationValueResult
{
    // Return an inverse comparison, so that the priority queue gives us the
    // smallest items first
    bool operator() (memdb::StationValue* x, memdb::StationValue* y) const
    {
        if (int res = x->station.coords.compare(y->station.coords)) return res > 0;
        if (x->station.ident < y->station.ident) return false;
        if (x->station.ident > y->station.ident) return true;
        if (int res = x->var->code() - y->var->code()) return res > 0;
        return x->station.report > y->station.report;
    }
};

// TODO: order by code and report
struct StationValueResultQueue : public priority_queue<memdb::StationValue*, vector<memdb::StationValue*>, CompareStationValueResult>
{
    StationValueResultQueue(DB&, Results<memdb::StationValue>& res)
    {
        res.copy_valptrs_to(stl::pusher(*this));
    }
};

struct CompareData
{
    const ValueStorage<memdb::Value>& values;

    CompareData(const ValueStorage<memdb::Value>& values) : values(values) {}

    // Return an inverse comparison, so that the priority queue gives us the
    // smallest items first
    bool operator() (size_t x, size_t y) const
    {
        const memdb::Value& vx = *values[x];
        const memdb::Value& vy = *values[y];

        if (int res = vx.station.coords.compare(vy.station.coords)) return res > 0;
        if (vx.station.ident < vy.station.ident) return false;
        if (vx.station.ident > vy.station.ident) return true;

        if (int res = vx.datetime.compare(vy.datetime)) return res > 0;
        if (int res = vx.levtr.level.compare(vy.levtr.level)) return res > 0;
        if (int res = vx.levtr.trange.compare(vy.levtr.trange)) return res > 0;

        if (int res = vx.var->code() - vy.var->code()) return res > 0;

        return vx.station.report > vy.station.report;
    }
};

struct DataResultQueue : public priority_queue<size_t, vector<size_t>, CompareData>
{
    DataResultQueue(DB&, Results<memdb::Value>& res)
        : priority_queue<size_t, vector<size_t>, CompareData>(CompareData(res.values))
    {
        res.copy_indices_to(stl::pusher(*this));
    }
};

struct DataBestResultQueue : public map<cursor::DataBestKey, size_t>
{
    const ValueStorage<memdb::Value>& values;
    std::map<std::string, int> prios;

    DataBestResultQueue(DB& db, Results<memdb::Value>& res)
        : values(res.values), prios(db.get_repinfo_priorities())
    {
        res.copy_indices_to(stl::pusher(*this));
    }

    void dump(FILE* out) const
    {
        for (const_iterator i = begin(); i != end(); ++i)
        {
            const memdb::Value& k = i->first.value();
            const memdb::Value& v = *values[i->second];

            stringstream buf;
            buf << k.station.coords
                << "\t" << k.station.ident
                << "\t" << k.levtr.level
                << "\t" << k.levtr.trange
                << "\t" << k.datetime
                << ": " << v.station.report
                << "\t";
            v.var->print_without_attrs(buf);
            fputs(buf.str().c_str(), out);
        }
    }

    /**
     * Add val to the map, but in case of conflict it only keeps the value with
     * the highest priority.
     */
    void push(size_t val)
    {
        iterator i = find(cursor::DataBestKey(values, val));
        if (i == end())
            insert(make_pair(cursor::DataBestKey(values, val), val));
        else
        {
            const memdb::Value& vx = *values[i->second];
            const memdb::Value& vy = *values[val];
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
    CursorStations(mem::DB& db, unsigned modifiers, Results<memdb::Station>& res)
        : CursorSorted<StationResultQueue>(db, modifiers, res)
    {
    }

    virtual void attr_insert(const Values& attrs)
    {
        // Attributes for stations do not make any sense
        throw error_unimplemented("CursorStations::attr_insert");
    }

    virtual void attr_remove(const AttrList& qcs)
    {
        // Attributes for stations do not make any sense
        throw error_unimplemented("CursorStations::attr_remove");
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
    CursorStationData(mem::DB& db, unsigned modifiers, Results<memdb::StationValue>& res)
        : CursorSorted<StationValueResultQueue>(db, modifiers, res)
    {
    }

    void query_attrs(function<void(unique_ptr<Var>&&)> dest) override
    {
        queue.top()->query_attrs(dest);
    }

    virtual void attr_insert(const Values& attrs)
    {
        queue.top()->attr_insert(attrs);
    }

    virtual void attr_remove(const AttrList& qcs)
    {
        queue.top()->attr_remove(qcs);
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
    const ValueStorage<memdb::Value>& values;
    size_t cur_idx;

    CursorDataBase(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : CursorSorted<QUEUE>(db, modifiers, res), values(res.values)
    {
    }

    virtual void attr_insert(const Values& attrs)
    {
        this->values[this->cur_idx]->attr_insert(attrs);
    }

    virtual void attr_remove(const AttrList& qcs)
    {
        this->values[this->cur_idx]->attr_remove(qcs);
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
        rec.seti("context_id", this->cur_idx);
        if (this->modifiers & DBA_DB_MODIFIER_ANAEXTRA)
            this->add_station_info(rec);
    }

    void query_attrs(function<void(unique_ptr<Var>&&)> dest) override
    {
        this->cur_value->query_attrs(dest);
    }
};

struct CursorData : public CursorDataBase<DataResultQueue>
{
    CursorData(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : CursorDataBase<DataResultQueue>(db, modifiers, res)
    {
    }
};

struct CursorDataBest : public CursorDataBase<DataBestResultQueue>
{
    CursorDataBest(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
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

    CursorSummary(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
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

    virtual void attr_insert(const Values& attrs)
    {
        throw error_unimplemented("CursorSummary::attr_insert");
    }

    virtual void attr_remove(const AttrList& qcs)
    {
        throw error_unimplemented("CursorSummary::attr_remove");
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

unique_ptr<db::Cursor> Cursor::createStations(mem::DB& db, unsigned modifiers, Results<memdb::Station>& res)
{
    return unique_ptr<db::Cursor>(new CursorStations(db, modifiers, res));
}

unique_ptr<db::Cursor> Cursor::createStationData(mem::DB& db, unsigned modifiers, Results<memdb::StationValue>& res)
{
    return unique_ptr<db::Cursor>(new CursorStationData(db, modifiers, res));
}

unique_ptr<db::Cursor> Cursor::createData(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::Cursor>(new CursorData(db, modifiers, res));
}

unique_ptr<db::Cursor> Cursor::createDataBest(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::Cursor>(new CursorDataBest(db, modifiers, res));
}

unique_ptr<db::Cursor> Cursor::createSummary(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::Cursor>(new CursorSummary(db, modifiers, res));
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
#include "dballe/memdb/results.tcc"
