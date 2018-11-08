#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "transaction.h"
#include "dballe/sql/sql.h"
#include <dballe/db/v7/driver.h>
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/types.h"
#include "dballe/core/record.h"
#include "dballe/var.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "wreport/var.h"
#include <unordered_map>
#include <cstring>
#include <cassert>

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
    std::shared_ptr<v7::Transaction> tr;

    /** Modifier flags to enable special query behaviours */
    const unsigned int modifiers;

    Base(std::shared_ptr<v7::Transaction> tr, unsigned int modifiers)
        : tr(tr), modifiers(modifiers)
    {
    }

    virtual ~Base() {}

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override = 0;
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

    void discard() override
    {
        at_start = false;
        cur = results.end();
    }

    dballe::DBStation get_station() const override { return cur->station; }
#if 0
    int get_station_id() const override { return cur->station.id; }
    std::string get_report() const override { return cur->station.report; }
    Coords get_coords() const override { return cur->station.coords; }
    Ident get_ident() const override { return cur->station.ident; }
#endif

    void to_record(Record& rec) override
    {
        cur->to_record(*this->tr, rec);
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
    dballe::DBStation station;

    StationResult(const dballe::DBStation& station) : station(station) {}

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s\n", station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get());
    }

    int get_station_id() const { return station.id; }
    void to_record(v7::Transaction& tr, Record& rec) const
    {
        core::Record& r = core::Record::downcast(rec);
        tr.repinfo().to_record(station.report, r);
        rec.set_dbstation(station);
        Tracer<> trc(tr.trc ? tr.trc->trace_add_station_vars() : nullptr);
        tr.station().add_station_vars(trc, station.id, rec);
    }
};

struct Stations : public VectorBase<CursorStation, StationResult>
{
    using VectorBase::VectorBase;

    void load(Tracer<>& trc, const StationQueryBuilder& qb)
    {
        results.clear();
        this->tr->station().run_station_query(trc, qb, [&](const dballe::DBStation& desc) {
            results.emplace_back(desc);
        });
        at_start = true;
        cur = results.begin();
    }
};


struct StationDataResult
{
    dballe::DBStation station;
    int id_data;
    Var* var;

    StationDataResult(const dballe::DBStation& station, int id_data, Var* var) : station(station), id_data(id_data), var(var) {}
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

    int get_station_id() const { return station.id; }

    void to_record(v7::Transaction& tr, Record& rec) const
    {
        core::Record& r = core::Record::downcast(rec);
        tr.repinfo().to_record(station.report, r);
        rec.set_dbstation(station);
        r.count = id_data;
        r.var = var->code();
        rec.clear_vars();
        // TODO: this could be optimized with a move, but it would mean that
        // to_record can only be called once. That is currently the case, but
        // not explicitly specified anywhere, so the change needs to happen
        // when we can check what breaks.
        rec.set(*var);
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d ",
                station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get(), id_data);
        var->print_without_attrs(out, "\n");
    }
};

template<typename Interface, typename Result>
struct BaseData : public VectorBase<Interface, Result>
{
    BaseData(DataQueryBuilder& qb, unsigned int modifiers)
        : VectorBase<Interface, Result>(qb.tr, modifiers)
    {
    }
};


struct StationData : public BaseData<CursorStationData, StationDataResult>
{
    using BaseData::BaseData;

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }

    void load(Tracer<>& trc, const DataQueryBuilder& qb)
    {
        results.clear();
        this->tr->station_data().run_station_data_query(trc, qb, [&](const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var) {
            results.emplace_back(station, id_data, var.release());
        });
        at_start = true;
        cur = results.begin();
    }

    wreport::Varcode get_varcode() const override { return cur->var->code(); }
    wreport::Var get_var() const override { return *cur->var; }
    int attr_reference_id() const override { return cur->id_data; }

    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)>&& dest, bool force_read) override
    {
        if (!force_read && modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES)
        {
            for (const Var* a = cur->var->next_attr(); a != NULL; a = a->next_attr())
                dest(std::unique_ptr<wreport::Var>(new Var(*a)));
        } else {
            tr->attr_query_station(attr_reference_id(), std::move(dest));
        }
    }
};


struct DataResult : public StationDataResult
{
    int id_levtr;
    Datetime datetime;

    using StationDataResult::StationDataResult;

    DataResult(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, Var* var)
        : StationDataResult(station, id_data, var), id_levtr(id_levtr), datetime(datetime) {}

    void to_record(v7::Transaction& tr, Record& rec) const
    {
        StationDataResult::to_record(tr, rec);
        rec.set_datetime(datetime);
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d %4d ",
                station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get(), id_levtr, id_data);
        datetime.print_iso8601(out, ' ');
        fprintf(out, " ");
        var->print_without_attrs(out, "\n");
    }
};


struct Data : public BaseData<CursorData, DataResult>
{
    using BaseData::BaseData;

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }

    void load(Tracer<>& trc, const DataQueryBuilder& qb)
    {
        results.clear();
        set<int> ids;
        this->tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
            results.emplace_back(station, id_levtr, datetime, id_data, var.release());
            ids.insert(id_levtr);
        });
        at_start = true;
        cur = results.begin();

        this->tr->levtr().prefetch_ids(trc, ids);
    }

    const LevTrEntry& get_levtr(int id_levtr) const
    {
        // We prefetch levtr info for all IDs, so we do not need to hit the database here
        return tr->levtr().lookup_cache(id_levtr);
    }

    Level get_level() const override { return get_levtr(cur->id_levtr).level; }
    Trange get_trange() const override { return get_levtr(cur->id_levtr).trange; }
    Datetime get_datetime() const override { return cur->datetime; }
    wreport::Varcode get_varcode() const override { return cur->var->code(); }
    wreport::Var get_var() const override { return *cur->var; }
    int attr_reference_id() const override { return cur->id_data; }

    void to_record(Record& rec) override
    {
        VectorBase::to_record(rec);
        const LevTrEntry& levtr = get_levtr(cur->id_levtr);
        rec.set_level(levtr.level);
        rec.set_trange(levtr.trange);
    }

    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)>&& dest, bool force_read) override
    {
        if (!force_read && modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES)
        {
            for (const Var* a = cur->var->next_attr(); a != NULL; a = a->next_attr())
                dest(std::unique_ptr<wreport::Var>(new Var(*a)));
        } else {
            tr->attr_query_data(attr_reference_id(), std::move(dest));
        }
    }
};

struct Best : public Data
{
    using Data::Data;

    int insert_cur_prio;

    /// Append or replace the last result according to priotity. Returns false if the value has been ignored.
    bool add_to_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)
    {
        int prio = tr->repinfo().get_priority(station.report);

        if (results.empty()) goto append;
        if (station.coords != results.back().station.coords) goto append;
        if (station.ident != results.back().station.ident) goto append;
        if (id_levtr != results.back().id_levtr) goto append;
        if (datetime != results.back().datetime) goto append;
        if (var->code() != results.back().var->code()) goto append;

        if (prio <= insert_cur_prio) return false;

        // Replace
        results.back().station = station;
        results.back().id_data = id_data;
        delete results.back().var;
        results.back().var = var.release();
        insert_cur_prio = prio;
        return true;

    append:
        results.emplace_back(station, id_levtr, datetime, id_data, var.release());
        insert_cur_prio = prio;
        return true;
    }

    void load(Tracer<>& trc, const DataQueryBuilder& qb)
    {
        results.clear();
        set<int> ids;
        this->tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
            if (add_to_results(station, id_levtr, datetime, id_data, move(var)))
                ids.insert(id_levtr);
        });
        at_start = true;
        cur = results.begin();

        this->tr->levtr().prefetch_ids(trc, ids);
    }

    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)>&& dest, bool force_read) override
    {
        if (!force_read && modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES)
        {
            for (const Var* a = cur->var->next_attr(); a != NULL; a = a->next_attr())
                dest(std::unique_ptr<wreport::Var>(new Var(*a)));
        } else {
            tr->attr_query_data(attr_reference_id(), std::move(dest));
        }
    }
};


struct SummaryResult
{
    dballe::DBStation station;
    int id_levtr;
    wreport::Varcode code;
    DatetimeRange datetime;
    size_t count = 0;

    SummaryResult(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t count)
        : station(station), id_levtr(id_levtr), code(code), datetime(datetime), count(count) {}

    int get_station_id() const { return station.id; }

    void to_record(v7::Transaction& tr, Record& rec) const
    {
        core::Record& r = core::Record::downcast(rec);
        tr.repinfo().to_record(station.report, r);
        r.set_dbstation(station);
        r.var = code;
        if (count > 0)
        {
            r.count = count;
            r.set(datetime);
        }
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d %d%02d%03d\n",
                station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get(), id_levtr, WR_VAR_FXY(code));
    }
};

struct Summary : public VectorBase<CursorSummary, SummaryResult>
{
    using VectorBase::VectorBase;

    const LevTrEntry& get_levtr(int id_levtr) const
    {
        // We prefetch levtr info for all IDs, so we do not need to hit the database here
        return tr->levtr().lookup_cache(id_levtr);
    }

    Level get_level() const override { return get_levtr(cur->id_levtr).level; }
    Trange get_trange() const override { return get_levtr(cur->id_levtr).trange; }

    DatetimeRange get_datetimerange() const override
    {
        return this->cur->datetime;
    }
    wreport::Varcode get_varcode() const override { return this->cur->code; }
    size_t get_count() const override { return this->cur->count; }

    void to_record(Record& rec) override
    {
        VectorBase::to_record(rec);
        const LevTrEntry& levtr = get_levtr(cur->id_levtr);
        rec.set_level(levtr.level);
        rec.set_trange(levtr.trange);
    }

    void load(Tracer<>& trc, const SummaryQueryBuilder& qb)
    {
        results.clear();
        set<int> ids;
        this->tr->data().run_summary_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t count) {
            results.emplace_back(station, id_levtr, code, datetime, count);
            ids.insert(id_levtr);
        });
        at_start = true;
        cur = results.begin();

        this->tr->levtr().prefetch_ids(trc, ids);
    }
};

}

unique_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();

    StationQueryBuilder qb(tr, q, modifiers);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Stations(tr, modifiers);
    unique_ptr<db::CursorStation> res(resptr);
    resptr->load(trc, qb);
    return res;
}

unique_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, true);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<db::CursorStationData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        throw error_unimplemented("best queries of station vars");
        //auto resptr = new Best(tr, modifiers);
        //res.reset(resptr);
        //resptr->load(qb);
    } else {
        auto resptr = new StationData(qb, modifiers);
        res.reset(resptr);
        resptr->load(trc, qb);
    }
    return res;
}

unique_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<CursorData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        auto resptr = new Best(qb, modifiers);
        res.reset(resptr);
        resptr->load(trc, qb);
    } else {
        auto resptr = new Data(qb, modifiers);
        res.reset(resptr);
        resptr->load(trc, qb);
    }
    return res;
}

unique_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on summary queries");

    SummaryQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Summary(tr, modifiers);
    unique_ptr<CursorSummary> res(resptr);
    resptr->load(trc, qb);
    return res;
}

void run_delete_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool station_vars, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on delete queries");

    IdQueryBuilder qb(tr, q, modifiers, station_vars);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    tr->data().remove(trc, qb);
}


}
}
}
}
