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

namespace {

/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
template<typename Interface>
class Base : public Interface
{
protected:
    /// Database to operate on
    mem::DB& db;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    /// Number of results still to be fetched
    size_t count = 0;

public:
    virtual ~Base() {}

    dballe::DB& get_db() const override { return db; }
    int remaining() const override { return count; }

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    bool next() override = 0;

    /// Discard the results that have not been read yet
    void discard_rest() override = 0;

#if 0
    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;
#endif

protected:
    /**
     * Create a query cursor
     *
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    Base(mem::DB& db, unsigned modifiers)
        : db(db), modifiers(modifiers)
    {
    }

    void to_record_station(const memdb::Station& st, Record& rec)
    {
        rec.set("ana_id", (int)st.id);
        core::Record::downcast(rec).set_coords(st.coords);
        if (st.mobile)
        {
            rec.set("ident", st.ident);
            rec.set("mobile", 1);
        } else {
            rec.unset("ident");
            rec.set("mobile", 0);
        }
        rec.set("rep_memo", st.report);
        rec.set("priority", db.repinfo.get_prio(st.report));
    }

    void to_record_levtr(const memdb::Value& val, Record& rec)
    {
        rec.set(val.levtr.level);
        rec.set(val.levtr.trange);
    }

    void to_record_varcode(wreport::Varcode code, Record& rec)
    {
        char bname[7];
        snprintf(bname, 7, "B%02d%03d", WR_VAR_X(code), WR_VAR_Y(code));
        rec.setc("var", bname);
    }

    void to_record_value(const memdb::Value& val, Record& rec)
    {
        to_record_levtr(val, rec);
        rec.set(val.datetime);
        to_record_varcode(val.var->code(), rec);
        rec.set(*val.var);
    }

    /// Query extra station info and add it to \a rec
    void add_station_info(const memdb::Station& st, Record& rec)
    {
        db.memdb.stationvalues.fill_record(st, rec);
    }
};

/**
 * Implement next() for cursors that can be implemented on top of a sorted
 * collection offering size(), empty(), pop() and top()
 */
template<typename Interface, typename SortedResults>
struct CursorSorted : public Base<Interface>
{
    typename SortedResults::value_type cur;
    SortedResults results;
    bool first = true;

    bool next() override
    {
        if (this->count == 0 || results.empty())
            return false;

        if (first)
            first = false;
        else
            results.pop();

        --(this->count);
        if (results.empty())
            return false;

        this->cur = results.top();
        return true;
    }

    void discard_rest() override
    {
        // We cannot call results.clear()
        // FIXME: we can probably get rid of discard_rest() and do proper
        // review and destructor work. This is only used to flush DB cursors to
        // avoid shutting down a partial query for some databases. But really,
        // is it needed at all?
        this->count = 0;
    }

    template<typename T>
    CursorSorted(mem::DB& db, unsigned modifiers, T& res)
        : Base<Interface>(db, modifiers), results(db, res)
    {
        this->count = results.size();
    }
};

struct StationResultQueue : public stack<const memdb::Station*>
{
    StationResultQueue(DB&, Results<memdb::Station>& res)
    {
        res.copy_valptrs_to(stl::pusher(*this));
    }
};

struct MemCursorStations : public CursorSorted<CursorStation, StationResultQueue>
{
    MemCursorStations(mem::DB& db, unsigned modifiers, Results<memdb::Station>& res)
        : CursorSorted(db, modifiers, res)
    {
    }

    int get_station_id() const override { return cur->id; }
    double get_lat() const override { return cur->coords.dlat(); }
    double get_lon() const override { return cur->coords.dlon(); }
    const char* get_ident(const char* def=0) const override
    {
        if (cur->mobile)
            return cur->ident.c_str();
        else
            return def;
    }
    const char* get_rep_memo() const override { return cur->report.c_str(); }

    void to_record(Record& rec)
    {
        this->to_record_station(*cur, rec);
        this->add_station_info(*cur, rec);
    }
};

struct CompareStationData
{
    const ValueStorage<memdb::StationValue>& values;

    CompareStationData(const ValueStorage<memdb::StationValue>& values) : values(values) {}

    // Return an inverse comparison, so that the priority queue gives us the
    // smallest items first
    bool operator() (size_t ix, size_t iy) const
    {
        const memdb::StationValue& x = *values[ix];
        const memdb::StationValue& y = *values[iy];
        if (int res = x.station.coords.compare(y.station.coords)) return res > 0;
        if (x.station.ident < y.station.ident) return false;
        if (x.station.ident > y.station.ident) return true;
        if (int res = x.var->code() - y.var->code()) return res > 0;
        return x.station.report > y.station.report;
    }
};

struct StationValueResultQueue : public priority_queue<size_t, vector<size_t>, CompareStationData>
{
    StationValueResultQueue(DB&, Results<memdb::StationValue>& res)
        : priority_queue<size_t, vector<size_t>, CompareStationData>(CompareStationData(res.values))
    {
        res.copy_indices_to(stl::pusher(*this));
    }
};

struct MemCursorStationData : public CursorSorted<CursorStationData, StationValueResultQueue>
{
    const ValueStorage<memdb::StationValue>& values;

    MemCursorStationData(mem::DB& db, unsigned modifiers, Results<memdb::StationValue>& res)
        : CursorSorted<CursorStationData, StationValueResultQueue>(db, modifiers, res), values(res.values)
    {
    }

    int get_station_id() const override { return values[cur]->station.id; }
    double get_lat() const override { return values[cur]->station.coords.dlat(); }
    double get_lon() const override { return values[cur]->station.coords.dlon(); }
    const char* get_ident(const char* def=0) const override
    {
        if (values[cur]->station.mobile)
            return values[cur]->station.ident.c_str();
        else
            return def;
    }
    const char* get_rep_memo() const override { return values[cur]->station.report.c_str(); }
    wreport::Varcode get_varcode() const override { return values[this->cur]->var->code(); }
    wreport::Var get_var() const override
    {
        Var res(values[this->cur]->var->info());
        res.setval(*values[this->cur]->var);
        return res;
    }
    int attr_reference_id() const override { return this->cur; }

#if 0
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
#endif

    void to_record(Record& rec)
    {
        to_record_station(values[cur]->station, rec);
        to_record_varcode(values[cur]->var->code(), rec);
        rec.set(*values[cur]->var);
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

struct DataBestResultQueue : public map<DataBestKey, size_t>
{
    typedef size_t value_type;
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
        iterator i = find(DataBestKey(values, val));
        if (i == end())
            insert(make_pair(DataBestKey(values, val), val));
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

template<typename Interface, typename QUEUE>
struct CursorDataBase : public CursorSorted<Interface, QUEUE>
{
    const ValueStorage<memdb::Value>& values;

    CursorDataBase(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : CursorSorted<Interface, QUEUE>(db, modifiers, res), values(res.values)
    {
    }

    int get_station_id() const override { return values[this->cur]->station.id; }
    double get_lat() const override { return values[this->cur]->station.coords.dlat(); }
    double get_lon() const override { return values[this->cur]->station.coords.dlon(); }
    const char* get_ident(const char* def=0) const override
    {
        if (values[this->cur]->station.mobile)
            return values[this->cur]->station.ident.c_str();
        else
            return def;
    }
    const char* get_rep_memo() const override { return values[this->cur]->station.report.c_str(); }
    Level get_level() const override { return values[this->cur]->levtr.level; }
    Trange get_trange() const override { return values[this->cur]->levtr.trange; }
    wreport::Varcode get_varcode() const override { return values[this->cur]->var->code(); }
    Datetime get_datetime() const override { return values[this->cur]->datetime; }
    wreport::Var get_var() const override
    {
        Var res(values[this->cur]->var->info());
        res.setval(*values[this->cur]->var);
        return res;
    }
    int attr_reference_id() const override { return this->cur; }

#if 0
    void attr_insert(const Values& attrs) override
    {
        this->values[this->cur_idx]->attr_insert(attrs);
    }

    void attr_remove(const AttrList& qcs) override
    {
        this->values[this->cur_idx]->attr_remove(qcs);
    }

    void query_attrs(function<void(unique_ptr<Var>&&)> dest) override
    {
        this->cur_value->query_attrs(dest);
    }
#endif

    void to_record(Record& rec) override
    {
        this->to_record_station(values[this->cur]->station, rec);
        rec.clear_vars();
        this->to_record_value(*values[this->cur], rec);
        rec.seti("context_id", this->cur);
    }
};

struct MemCursorData : public CursorDataBase<CursorData, DataResultQueue>
{
    MemCursorData(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : CursorDataBase<CursorData, DataResultQueue>(db, modifiers, res)
    {
    }
};

struct MemCursorDataBest : public CursorDataBase<CursorData, DataBestResultQueue>
{
    MemCursorDataBest(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : CursorDataBase<CursorData, DataBestResultQueue>(db, modifiers, res)
    {
    }
};

struct MemCursorSummary : public Base<CursorSummary>
{
    memdb::Summary values;
    memdb::Summary::const_iterator iter_cur;
    memdb::Summary::const_iterator iter_end;
    bool first;

    MemCursorSummary(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
        : Base<CursorSummary>(db, modifiers), first(true)
    {
        memdb::Summarizer summarizer(values);
        res.copy_valptrs_to(stl::trivial_inserter(summarizer));

        // Start iterating at the beginning
        iter_cur = values.begin();
        iter_end = values.end();

        // Initialize the result count
        count = values.size();
    }

    int get_station_id() const override { return iter_cur->first.sample.station.id; }
    double get_lat() const override { return iter_cur->first.sample.station.coords.dlat(); }
    double get_lon() const override { return iter_cur->first.sample.station.coords.dlon(); }
    const char* get_ident(const char* def=0) const override
    {
        if (iter_cur->first.sample.station.mobile)
            return iter_cur->first.sample.station.ident.c_str();
        else
            return def;
    }
    const char* get_rep_memo() const override { return iter_cur->first.sample.station.report.c_str(); }
    Level get_level() const override { return iter_cur->first.sample.levtr.level; }
    Trange get_trange() const override { return iter_cur->first.sample.levtr.trange; }
    wreport::Varcode get_varcode() const override { return iter_cur->first.sample.var->code(); }
    DatetimeRange get_datetimerange() const override { return DatetimeRange(iter_cur->second.dtmin, iter_cur->second.dtmax); }
    size_t get_count() const override { return iter_cur->second.count; }

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
#if 0
            cur_value = &(iter_cur->first.sample);
            cur_var = cur_value->var;
            cur_station = &(iter_cur->first.sample.station);
#endif
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
        to_record_station(iter_cur->first.sample.station, rec);
        to_record_levtr(iter_cur->first.sample, rec);
        to_record_varcode(iter_cur->first.sample.var->code(), rec);
        if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
        {
            rec.set(get_datetimerange());
            rec.seti("context_id", get_count());
        }
    }
};

}

unique_ptr<db::CursorStation> createStations(mem::DB& db, unsigned modifiers, Results<memdb::Station>& res)
{
    return unique_ptr<db::CursorStation>(new MemCursorStations(db, modifiers, res));
}

unique_ptr<db::CursorStationData> createStationData(mem::DB& db, unsigned modifiers, Results<memdb::StationValue>& res)
{
    return unique_ptr<db::CursorStationData>(new MemCursorStationData(db, modifiers, res));
}

unique_ptr<db::CursorData> createData(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::CursorData>(new MemCursorData(db, modifiers, res));
}

unique_ptr<db::CursorData> createDataBest(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::CursorData>(new MemCursorDataBest(db, modifiers, res));
}

unique_ptr<db::CursorSummary> createSummary(mem::DB& db, unsigned modifiers, Results<memdb::Value>& res)
{
    return unique_ptr<db::CursorSummary>(new MemCursorSummary(db, modifiers, res));
}


DataBestKey::DataBestKey(const memdb::ValueStorage<memdb::Value>& values, size_t idx)
    : values(values), idx(idx) {}

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
}
#include "dballe/memdb/results.tcc"
