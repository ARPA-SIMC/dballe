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

    /// Results from the query
    Structbuf<v7::SQLRecordV7> results;

    /// Prefetched mapping between levtr IDs and their values
    std::map<int, LevTrDesc> levtrs;

    /// Current result element being iterated
    int cur = -1;

    Base(v7::DB& db, unsigned int modifiers)
        : db(db), modifiers(modifiers)
    {
    }

    virtual ~Base() {}

    dballe::DB& get_db() const override { return db; }

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    int remaining() const override
    {
        if (cur == -1) return results.size();
        return results.size() - cur - 1;
    }

    bool next() override
    {
        ++cur;
        return (size_t)cur < results.size();
    }


    void discard_rest() override { cur = results.size(); }

    int get_station_id() const override { return results[cur].out_ana_id; }
    double get_lat() const override { return (double)results[cur].out_lat / 100000.0; }
    double get_lon() const override { return (double)results[cur].out_lon / 100000.0; }
    const char* get_ident(const char* def=0) const override
    {
        if (results[cur].out_ident_size == -1 || results[cur].out_ident[0] == 0)
            return def;
        return results[cur].out_ident;
    }
    const char* get_rep_memo() const override
    {
        return db.repinfo().get_rep_memo(results[cur].out_rep_cod);
    }

    const LevTrDesc& get_levtr(int id) const
    {
        auto i = levtrs.find(id);
        if (i == levtrs.end()) error_notfound::throwf("levtrs with id %d not found", id);
        return i->second;
    }

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override = 0;

    /// Run the query in qb and fill results with its output
    virtual void load(const QueryBuilder& qb)
    {
        set<int> ids;
        db.driver().run_built_query_v7(qb, [&](v7::SQLRecordV7& rec) {
            results.append(rec);
            if (qb.select_varinfo) ids.insert(rec.out_id_ltr);
        });
        // We are done adding, prepare the structbuf for reading
        results.ready_to_read();
        // And prefetch the LevTr data
        db.lev_tr().prefetch_ids(ids, levtrs);
    }

    void to_record_pseudoana(Record& rec)
    {
        rec.seti("ana_id", results[cur].out_ana_id);
        rec.seti("lat", results[cur].out_lat);
        rec.seti("lon", results[cur].out_lon);
        if (results[cur].out_ident_size != -1 && results[cur].out_ident[0] != 0)
        {
            rec.setc("ident", results[cur].out_ident);
            rec.seti("mobile", 1);
        } else {
            rec.unset("ident");
            rec.seti("mobile", 0);
        }
    }

    void to_record_repinfo(Record& rec)
    {
        db.repinfo().to_record(results[cur].out_rep_cod, rec);
    }

    void to_record_ltr(Record& rec)
    {
        if (results[cur].out_id_ltr != -1)
        {
            auto& ltr = levtrs[results[cur].out_id_ltr];
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
        rec.set(results[cur].out_datetime);
    }

    void to_record_varcode(Record& rec)
    {
        char bname[7];
        snprintf(bname, 7, "B%02d%03d",
                WR_VAR_X(results[cur].out_varcode),
                WR_VAR_Y(results[cur].out_varcode));
        rec.setc("var", bname);
    }

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec)
    {
        db.station().add_station_vars(results[cur].out_ana_id, rec);
    }
};


struct Stations : public Base<CursorStation>
{
    using Base::Base;
    ~Stations() {}

    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        add_station_info(rec);
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
                fprintf(dump, "%02d %02.4f %02.4f %-10s\n",
                        r->enq("ana_id", -1),
                        r->enq("lat", 0.0),
                        r->enq("lon", 0.0),
                        r->enq("ident", ""));
            }
        }
        return count;
    }
};


struct StationData : public Base<CursorStationData>
{
    using Base::Base;
    ~StationData() {}
    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        if (results[cur].out_id_ltr == -1)
            rec.unset("context_id");
        else
            rec.seti("context_id", results[cur].out_id_data);
        to_record_varcode(rec);
        rec.clear_vars();
        rec.set(newvar(results[cur].out_varcode, results[cur].out_value));
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

    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[cur].out_varcode; }
    wreport::Var get_var() const override { return Var(varinfo(results[cur].out_varcode), results[cur].out_value); }
    int attr_reference_id() const override { return results[cur].out_id_data; }
};

struct Data : public Base<CursorData>
{
    using Base::Base;
    ~Data() {}
    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        if (results[cur].out_id_ltr == -1)
            rec.unset("context_id");
        else
            rec.seti("context_id", results[cur].out_id_data);
        to_record_varcode(rec);
        to_record_ltr(rec);
        to_record_datetime(rec);

        rec.clear_vars();
        rec.set(newvar(results[cur].out_varcode, results[cur].out_value));
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

    Level get_level() const override { return get_levtr(results[cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[cur].out_id_ltr).trange; }
    Datetime get_datetime() const override { return results[cur].out_datetime; }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[cur].out_varcode; }
    wreport::Var get_var() const override { return Var(varinfo(results[cur].out_varcode), results[cur].out_value); }
    int attr_reference_id() const override { return results[cur].out_id_data; }

#if 0
void Cursor::query_attrs(function<void(unique_ptr<Var>&&)> dest)
{
    db.query_attrs(results[cur].out_id_data, results[cur].out_varcode, dest);
}

void Cursor::attr_insert(const Values& attrs)
{
    db.attr_insert(results[cur].out_id_data, results[cur].out_varcode, attrs);
}

void Cursor::attr_remove(const AttrList& qcs)
{
    db.attr_remove(results[cur].out_id_data, results[cur].out_varcode, qcs);
}
#endif
};

struct Summary : public Base<CursorSummary>
{
    using Base::Base;
    ~Summary() {}
    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        to_record_varcode(rec);
        to_record_ltr(rec);

        if (modifiers & DBA_DB_MODIFIER_SUMMARY_DETAILS)
        {
            rec.seti("context_id", results[cur].out_id_data);
            rec.set(DatetimeRange(results[cur].out_datetime, results[cur].out_datetimemax));
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
                        (int)results[cur].out_id_ltr,
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

    Level get_level() const override { return get_levtr(results[cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[cur].out_id_ltr).trange; }
    DatetimeRange get_datetimerange() const override
    {
        return DatetimeRange(results[cur].out_datetime, results[cur].out_datetimemax);
    }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[cur].out_varcode; }
    size_t get_count() const override { return results[cur].out_id_data; }
};

struct Best : public Base<CursorData>
{
    using Base::Base;
    ~Best() {}

    void to_record(Record& rec) override
    {
        to_record_pseudoana(rec);
        to_record_repinfo(rec);
        if (results[cur].out_id_ltr == -1)
            rec.unset("context_id");
        else
            rec.seti("context_id", results[cur].out_id_data);
        to_record_varcode(rec);
        to_record_ltr(rec);
        to_record_datetime(rec);

        rec.clear_vars();
        rec.set(newvar(results[cur].out_varcode, results[cur].out_value));
    }
    unsigned test_iterate(FILE* dump=0) override
    {
        // auto r = Record::create();
        unsigned count;
        for (count = 0; next(); ++count)
        {
            /*
            if (dump)
                fprintf(dump, "%03d\n", (int)results[cur].out_id_data);
                */
        }
        return count;
    }

    Level get_level() const override { return get_levtr(results[cur].out_id_ltr).level; }
    Trange get_trange() const override { return get_levtr(results[cur].out_id_ltr).trange; }
    Datetime get_datetime() const override { return results[cur].out_datetime; }
    wreport::Varcode get_varcode() const override { return (wreport::Varcode)results[cur].out_varcode; }
    wreport::Var get_var() const override { return Var(varinfo(results[cur].out_varcode), results[cur].out_value); }
    int attr_reference_id() const override { return results[cur].out_id_data; }


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
