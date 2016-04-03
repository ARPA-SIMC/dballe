#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "dballe/sql/sql.h"
#include <dballe/db/v7/driver.h>
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/types.h"
#include "dballe/record.h"
#include "dballe/var.h"
#include "dballe/core/query.h"
#include "dballe/core/structbuf.h"
#include "wreport/var.h"
#include <unordered_map>
#include <cstdio>
#include <cstring>

namespace {

// Consts used for to_record_todo
const unsigned int TOREC_PSEUDOANA   = 1 << 0;
const unsigned int TOREC_BASECONTEXT = 1 << 1;
const unsigned int TOREC_DATACONTEXT = 1 << 2;
const unsigned int TOREC_DATA        = 1 << 3;

}

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

namespace {

/**
 * Simple typedef to make typing easier, and also to help some versions of swig
 * match this complex type
 */
typedef std::vector<wreport::Varcode> AttrList;


/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
template<typename Interface>
struct Base : public Interface
{
    /// Database to operate on
    v7::DB& db;

    /** Modifier flags to enable special query behaviours */
    const unsigned int modifiers;

    Base(v7::DB& db, unsigned int modifiers)
        : db(db), modifiers(modifiers)
    {
    }

    virtual ~Base() {}

    dballe::DB& get_db() const override { return db; }

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override = 0;

    /// Run the query in qb and fill results with its output
    virtual void load(const QueryBuilder& qb) = 0;
};


template<typename Interface, typename Result>
struct VectorBase : public Base<Interface>
{
    using Base<Interface>::Base;

    std::vector<Result> results;
    typename std::vector<Result>::const_iterator cur;
    bool at_start;

    int remaining() const override
    {
        if (at_start)
            return results.size();
        else
            return results.end() - cur - 1;
    }

    bool next() override
    {
        if (at_start)
            at_start = false;
        else
            ++cur;
        return cur != results.end();
    }

    void discard_rest() override
    {
        at_start = false;
        cur = results.end();
    }

    int get_station_id() const override { return cur->get_station_id(); }
    const char* get_rep_memo() const override { return this->db.repinfo().get_rep_memo(cur->get_station().rep); }
    double get_lat() const override { return cur->get_station().coords.dlat(); }
    double get_lon() const override { return cur->get_station().coords.dlon(); }
    const char* get_ident(const char* def=0) const override
    {
        if (cur->get_station().ident.is_missing())
            return def;
        else
            return cur->get_station().ident.get();
    }

    void to_record(Record& rec) override
    {
        cur->to_record(this->db, rec);
    }

    unsigned test_iterate(FILE* dump=0) override
    {
        unsigned count;
        for (count = 0; next(); ++count)
            if (dump)
                cur->dump(dump);
        return count;
    }
};


struct StationResult
{
    int id;
    StationDesc station;

    StationResult(int id, const StationDesc& station) : id(id), station(station) {}

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %3d %02.4f %02.4f %-10s\n", id, station.rep, station.coords.dlat(), station.coords.dlon(), station.ident.get());
    }

    int get_station_id() const { return id; }
    const StationDesc& get_station() const { return station; }
    void to_record(v7::DB& db, Record& rec) const
    {
        rec.seti("ana_id", id);
        db.repinfo().to_record(station.rep, rec);
        station.to_record(rec);
        db.station().add_station_vars(id, rec);
    }
};

struct Stations : public VectorBase<CursorStation, StationResult>
{
    using VectorBase::VectorBase;

    void load(const QueryBuilder& qb) override
    {
        results.clear();
        this->db.driver().run_station_query(qb, [&](int id, const StationDesc& desc) {
            results.emplace_back(id, desc);
        });
        at_start = true;
        cur = results.begin();
    }
};


struct StationDataResult
{
    typedef std::unordered_map<int, StationDesc>::iterator station_t;
    station_t station;
    int id_data;
    Var* var;

    StationDataResult(station_t station, int id_data, Var* var) : station(station), id_data(id_data), var(var) {}
    StationDataResult(const StationDataResult&) = delete;
    StationDataResult(StationDataResult&& o) : station(o.station), id_data(o.id_data), var(o.var) { o.var = nullptr; }
    StationDataResult& operator=(const StationDataResult&) = delete;
    StationDataResult& operator=(StationDataResult&& o)
    {
        if (this == &o) return *this;
        delete var;
        var = o.var;
        o.var = nullptr;
        station = o.station;
        id_data = o.id_data;
        return *this;
    }
    ~StationDataResult() { delete var; }

    int get_station_id() const { return station->first; }
    const StationDesc& get_station() const { return station->second; }

    void to_record(v7::DB& db, Record& rec) const
    {
        rec.seti("ana_id", station->first);
        db.repinfo().to_record(station->second.rep, rec);
        station->second.to_record(rec);
        rec.seti("context_id", id_data);

        char bname[7];
        snprintf(bname, 7, "B%02d%03d", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        rec.setc("var", bname);

        rec.clear_vars();
        // TODO: this could be optimized with a move, but it would mean that
        // to_record can only be called once. That is currently the case, but
        // not explicitly specified anywhere, so the change needs to happen
        // when we can check what breaks.
        rec.set(*var);
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %3d %02.4f %02.4f %-10s %4d ",
                station->first, station->second.rep, station->second.coords.dlat(), station->second.coords.dlon(), station->second.ident.get(), id_data);
        var->print_without_attrs(out, "\n");
    }
};


struct StationData : public VectorBase<CursorStationData, StationDataResult>
{
    using VectorBase::VectorBase;

    std::unordered_map<int, StationDesc> stations;

    void load(const QueryBuilder& qb) override
    {
        stations.clear();
        results.clear();
        this->db.driver().run_station_data_query(qb, [&](int id_station, const StationDesc& station, int id_data, std::unique_ptr<wreport::Var> var) {
            std::unordered_map<int, StationDesc>::iterator i = stations.find(id_station);
            if (i == stations.end())
                tie(i, std::ignore) = stations.insert(make_pair(id_station, station));
            results.emplace_back(i, id_data, var.release());
        });
        at_start = true;
        cur = results.begin();
    }

    wreport::Varcode get_varcode() const override { return cur->var->code(); }
    wreport::Var get_var() const override { return *cur->var; }
    int attr_reference_id() const override { return cur->id_data; }
};


template<typename Interface>
struct OldBase : public Base<Interface>
{
    /// Current result element being iterated
    int cur = -1;

    using Base<Interface>::Base;

    /// Results from the query
    Structbuf<v7::SQLRecordV7> results;

    /// Prefetched mapping between levtr IDs and their values
    std::map<int, LevTrDesc> levtrs;

    unsigned size() const { return results.size(); }

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    int remaining() const override
    {
        if (cur == -1) return size();
        return size() - cur - 1;
    }

    bool next() override
    {
        ++cur;
        return (size_t)cur < size();
    }

    void discard_rest() override { cur = size(); }

    int get_station_id() const override { return results[this->cur].out_ana_id; }
    double get_lat() const override { return (double)results[this->cur].out_lat / 100000.0; }
    double get_lon() const override { return (double)results[this->cur].out_lon / 100000.0; }
    const char* get_ident(const char* def=0) const override
    {
        if (results[this->cur].out_ident_size == -1 || results[this->cur].out_ident[0] == 0)
            return def;
        return results[this->cur].out_ident;
    }
    const char* get_rep_memo() const override
    {
        return this->db.repinfo().get_rep_memo(results[this->cur].out_rep_cod);
    }

    const LevTrDesc& get_levtr(int id) const
    {
        auto i = levtrs.find(id);
        if (i == levtrs.end()) error_notfound::throwf("levtrs with id %d not found", id);
        return i->second;
    }

    void load(const QueryBuilder& qb) override
    {
        set<int> ids;
        this->db.driver().run_built_query_v7(qb, [&](v7::SQLRecordV7& rec) {
            results.append(rec);
            if (qb.select_varinfo) ids.insert(rec.out_id_ltr);
        });
        // We are done adding, prepare the structbuf for reading
        results.ready_to_read();
        // And prefetch the LevTr data
        this->db.lev_tr().prefetch_ids(ids, levtrs);
    }

    void to_record_pseudoana(Record& rec)
    {
        rec.seti("ana_id", results[this->cur].out_ana_id);
        rec.seti("lat", results[this->cur].out_lat);
        rec.seti("lon", results[this->cur].out_lon);
        if (results[this->cur].out_ident_size != -1 && results[this->cur].out_ident[0] != 0)
        {
            rec.setc("ident", results[this->cur].out_ident);
            rec.seti("mobile", 1);
        } else {
            rec.unset("ident");
            rec.seti("mobile", 0);
        }
    }

    void to_record_repinfo(Record& rec)
    {
        this->db.repinfo().to_record(results[this->cur].out_rep_cod, rec);
    }

    void to_record_ltr(Record& rec)
    {
        if (results[this->cur].out_id_ltr != -1)
        {
            auto& ltr = levtrs[results[this->cur].out_id_ltr];
            rec.set(ltr.level);
            rec.set(ltr.trange);
        }
        else
        {
            rec.unset("leveltype1");
            rec.unset("l1");
            rec.unset("leveltype2");
            rec.unset("l2");
            rec.unset("pindicator");
            rec.unset("p1");
            rec.unset("p2");
        }
    }

    void to_record_datetime(Record& rec)
    {
        rec.set(results[this->cur].out_datetime);
    }

    void to_record_varcode(Record& rec)
    {
        char bname[7];
        snprintf(bname, 7, "B%02d%03d",
                WR_VAR_X(results[this->cur].out_varcode),
                WR_VAR_Y(results[this->cur].out_varcode));
        rec.setc("var", bname);
    }

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec)
    {
        this->db.station().add_station_vars(results[this->cur].out_ana_id, rec);
    }
};

struct Data : public OldBase<CursorData>
{
    using OldBase::OldBase;
    ~Data() {}
    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        if (results[this->cur].out_id_ltr == -1)
            rec.unset("context_id");
        else
            rec.seti("context_id", results[this->cur].out_id_data);
        to_record_varcode(rec);
        to_record_ltr(rec);
        to_record_datetime(rec);

        rec.clear_vars();
        rec.set(newvar(results[this->cur].out_varcode, results[this->cur].out_value));
    }
    unsigned test_iterate(FILE* dump=0) override
    {
        //auto r = Record::create();
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

    Level get_level() const override { return get_levtr(results[this->cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[this->cur].out_id_ltr).trange; }
    Datetime get_datetime() const override { return results[this->cur].out_datetime; }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[this->cur].out_varcode; }
    wreport::Var get_var() const override { return Var(varinfo(results[this->cur].out_varcode), results[this->cur].out_value); }
    int attr_reference_id() const override { return results[this->cur].out_id_data; }

#if 0
void Cursor::query_attrs(function<void(unique_ptr<Var>&&)> dest)
{
    db.query_attrs(results[this->cur].out_id_data, results[this->cur].out_varcode, dest);
}

void Cursor::attr_insert(const Values& attrs)
{
    db.attr_insert(results[this->cur].out_id_data, results[this->cur].out_varcode, attrs);
}

void Cursor::attr_remove(const AttrList& qcs)
{
    db.attr_remove(results[this->cur].out_id_data, results[this->cur].out_varcode, qcs);
}
#endif
};

struct Summary : public OldBase<CursorSummary>
{
    using OldBase::OldBase;
    ~Summary() {}
    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        to_record_varcode(rec);
        to_record_ltr(rec);

        if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
        {
            rec.seti("context_id", results[this->cur].out_id_data);
            rec.set(DatetimeRange(results[this->cur].out_datetime, results[this->cur].out_datetimemax));
        }
    }
    unsigned test_iterate(FILE* dump=0) override
    {
        auto r = Record::create();
        unsigned count;
        for (count = 0; next(); ++count)
        {
            if (dump)
            {
                to_record(*r);
                fprintf(dump, "%02d %s %03d %s %04d-%02d-%02d %02d:%02d:%02d  %04d-%02d-%02d %02d:%02d:%02d  %d\n",
                        r->enq("ana_id", -1),
                        r->enq("rep_memo", ""),
                        (int)results[this->cur].out_id_ltr,
                        r->enq("var", ""),
                        r->enq("yearmin", 0), r->enq("monthmin", 0), r->enq("daymin", 0),
                        r->enq("hourmin", 0), r->enq("minumin", 0), r->enq("secmin", 0),
                        r->enq("yearmax", 0), r->enq("monthmax", 0), r->enq("daymax", 0),
                        r->enq("hourmax", 0), r->enq("minumax", 0), r->enq("secmax", 0),
                        r->enq("limit", -1));
            }
        }
        return count;
    }

    Level get_level() const override { return get_levtr(results[this->cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[this->cur].out_id_ltr).trange; }
    DatetimeRange get_datetimerange() const override
    {
        return DatetimeRange(results[this->cur].out_datetime, results[this->cur].out_datetimemax);
    }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[this->cur].out_varcode; }
    size_t get_count() const override { return results[this->cur].out_id_data; }
};

struct Best : public OldBase<CursorData>
{
    using OldBase::OldBase;
    ~Best() {}

    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        if (results[this->cur].out_id_ltr == -1)
            rec.unset("context_id");
        else
            rec.seti("context_id", results[this->cur].out_id_data);
        to_record_varcode(rec);
        to_record_ltr(rec);
        to_record_datetime(rec);

        rec.clear_vars();
        rec.set(newvar(results[this->cur].out_varcode, results[this->cur].out_value));
    }
    unsigned test_iterate(FILE* dump=0) override
    {
        // auto r = Record::create();
        unsigned count;
        for (count = 0; next(); ++count)
        {
            /*
            if (dump)
                fprintf(dump, "%03d\n", (int)results[this->cur].out_id_data);
                */
        }
        return count;
    }

    Level get_level() const override { return get_levtr(results[this->cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[this->cur].out_id_ltr).trange; }
    Datetime get_datetime() const override { return results[this->cur].out_datetime; }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[this->cur].out_varcode; }
    wreport::Var get_var() const override { return Var(varinfo(results[this->cur].out_varcode), results[this->cur].out_value); }
    int attr_reference_id() const override { return results[this->cur].out_id_data; }


    void load(const QueryBuilder& qb) override
    {
        db::v7::Repinfo& ri = db.repinfo();
        bool first = true;
        v7::SQLRecordV7 best;

        set<int> ids;
        db.driver().run_built_query_v7(qb, [&](v7::SQLRecordV7& rec) {
            // Fill priority
            rec.priority = ri.get_priority(rec.out_rep_cod);

            // Filter results keeping only those with the best priority
            if (first)
            {
                // The first record initialises 'best'
                best = rec;
                first = false;
            } else if (rec.querybest_fields_are_the_same(best)) {
                // If they match, keep the record with the highest priority
                if (rec.priority > best.priority)
                    best = rec;
            } else {
                if (qb.select_varinfo) ids.insert(best.out_id_ltr);
                // If they don't match, write out the previous best value
                results.append(best);
                // And restart with a new candidate best record for the next batch
                best = rec;
            }
        });

        // Write out the last best value
        if (!first)
        {
            if (qb.select_varinfo) ids.insert(best.out_id_ltr);
            results.append(best);
        }
        db.lev_tr().prefetch_ids(ids, levtrs);

        // We are done adding, prepare the structbuf for reading
        results.ready_to_read();
    }

    friend class Cursor;
};

}

unique_ptr<CursorStation> run_station_query(DB& db, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();

    StationQueryBuilder qb(db, q, modifiers);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        db.conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Stations(db, modifiers);
    unique_ptr<CursorStation> res(resptr);
    resptr->load(qb);
    return res;
}

unique_ptr<CursorStationData> run_station_data_query(DB& db, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(db, q, modifiers, true);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        db.conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<CursorStationData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        throw error_unimplemented("best queries of station vars");
        //auto resptr = new Best(db, modifiers);
        //res.reset(resptr);
        //resptr->load(qb);
    } else {
        auto resptr = new StationData(db, modifiers);
        res.reset(resptr);
        resptr->load(qb);
    }
    return res;
}

unique_ptr<CursorData> run_data_query(DB& db, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(db, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        db.conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<CursorData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        auto resptr = new Best(db, modifiers);
        res.reset(resptr);
        resptr->load(qb);
    } else {
        auto resptr = new Data(db, modifiers);
        res.reset(resptr);
        resptr->load(qb);
    }
    return res;
}

unique_ptr<CursorSummary> run_summary_query(DB& db, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on summary queries");

    SummaryQueryBuilder qb(db, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        db.conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Summary(db, modifiers);
    unique_ptr<CursorSummary> res(resptr);
    resptr->load(qb);
    return res;
}

void run_delete_query(DB& db, const core::Query& q, bool station_vars, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on delete queries");

    IdQueryBuilder qb(db, q, modifiers, station_vars);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        db.conn->explain(qb.sql_query, stderr);
    }

    db.data().remove(qb);
}


}
}
}
}
