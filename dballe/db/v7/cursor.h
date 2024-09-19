#ifndef DBA_DB_V7_CURSOR_H
#define DBA_DB_V7_CURSOR_H

#include <dballe/types.h>
#include <dballe/db/db.h>
#include <dballe/db/v7/transaction.h>
#include <dballe/db/v7/repinfo.h>
#include <dballe/db/v7/levtr.h>
#include <dballe/values.h>
#include <memory>
#include <deque>

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

struct Stations;
struct StationData;
struct Data;
struct Summary;

/**
 * Row resulting from a station query
 */
struct StationRow
{
    dballe::DBStation station;
    mutable std::unique_ptr<DBValues> values;

    StationRow(const dballe::DBStation& station) : station(station) {}

    void dump(FILE* out) const;
};

struct StationDataRow
{
    dballe::DBStation station;
    DBValue value;

    StationDataRow(const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var) : station(station), value(id_data, std::move(var)) {}
    StationDataRow(const StationDataRow&) = delete;
    StationDataRow(StationDataRow&& o) = default;
    StationDataRow& operator=(const StationDataRow&) = delete;
    StationDataRow& operator=(StationDataRow&& o) = default;
    ~StationDataRow() {}

    void dump(FILE* out) const;
};

struct DataRow : public StationDataRow
{
    int id_levtr;
    Datetime datetime;

    using StationDataRow::StationDataRow;

    DataRow(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)
        : StationDataRow(station, id_data, std::move(var)), id_levtr(id_levtr), datetime(datetime) {}

    void dump(FILE* out) const;
};

struct SummaryRow
{
    dballe::DBStation station;
    int id_levtr;
    wreport::Varcode code;
    DatetimeRange dtrange;
    size_t count = 0;

    SummaryRow(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& dtrange, size_t count)
        : station(station), id_levtr(id_levtr), code(code), dtrange(dtrange), count(count) {}

    void dump(FILE* out) const;
};


template<typename Cursor>
struct ImplTraits
{
};

template<>
struct ImplTraits<Stations>
{
    typedef dballe::CursorStation Interface;
    typedef db::CursorStation Parent;
    typedef StationRow Row;
};

template<>
struct ImplTraits<StationData>
{
    typedef dballe::CursorStationData Interface;
    typedef db::CursorStationData Parent;
    typedef StationDataRow Row;
};

template<>
struct ImplTraits<Data>
{
    typedef dballe::CursorData Interface;
    typedef db::CursorData Parent;
    typedef DataRow Row;
};

template<>
struct ImplTraits<Summary>
{
    typedef dballe::CursorSummary Interface;
    typedef db::CursorSummary Parent;
    typedef SummaryRow Row;
};


/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
template<typename Impl>
struct Base : public ImplTraits<Impl>::Parent
{
    typedef typename ImplTraits<Impl>::Row Row;
    typedef typename ImplTraits<Impl>::Interface Interface;

    /// Database to operate on
    std::shared_ptr<v7::Transaction> tr;

    /// Storage for the raw database results
    std::deque<Row> results;

    /// True if we are at the start of the iteration
    bool at_start = true;

    Base(std::shared_ptr<v7::Transaction> tr)
        : tr(tr)
    {
    }

    virtual ~Base() {}

    int remaining() const override;
    bool has_value() const override { return !at_start && !results.empty(); }
    bool next() override
    {
        if (at_start)
            at_start = false;
        else if (!results.empty())
            results.pop_front();
        return !results.empty();
    }

    void discard() override
    {
        at_start = false;
        results.clear();
        tr.reset();
    }

    dballe::DBStation get_station() const override { return row().station; }

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override;

    inline static std::shared_ptr<Impl> downcast(std::shared_ptr<Interface> c)
    {
        auto res = std::dynamic_pointer_cast<Impl>(c);
        if (!res)
            throw std::runtime_error("Attempted to downcast the wrong kind of cursor");
        return res;
    }

    const Row& row() const { return results.front(); }

protected:
    int get_priority() const { return tr->repinfo().get_priority(results.front().station.report); }
};

extern template class Base<Stations>;
extern template class Base<StationData>;
extern template class Base<Data>;
extern template class Base<Summary>;


/// CursorStation implementation
struct Stations : public Base<Stations>
{
    using Base::Base;
    DBValues get_values() const override;

    void remove() override;
    void enq(impl::Enq& enq) const override;

protected:
    const DBValues& values() const;
    void load(Tracer<>& trc, const StationQueryBuilder& qb);

    friend std::shared_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
};

/// CursorStationData implementation
struct StationData : public Base<StationData>
{
    bool with_attributes;

    StationData(DataQueryBuilder& qb, bool with_attributes);
    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }
    wreport::Varcode get_varcode() const override { return row().value.code(); }
    wreport::Var get_var() const override { return *row().value; }
    int attr_reference_id() const override { return row().value.data_id; }
    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override;
    void remove() override;
    void enq(impl::Enq& enq) const override;

protected:
    void load(Tracer<>& trc, const DataQueryBuilder& qb);

    friend std::shared_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
};

template<typename Impl>
struct LevTrBase : public Base<Impl>
{
protected:
    // Cached levtr for the current row
    mutable const LevTrEntry* levtr = nullptr;

    const LevTrEntry& get_levtr() const
    {
        if (levtr == nullptr)
            // We prefetch levtr info for all IDs, so we do not need to hit the database here
            levtr = &(this->tr->levtr().lookup_cache(this->results.front().id_levtr));
        return *levtr;
    }

public:
    using Base<Impl>::Base;

    bool next() override
    {
        levtr = nullptr;
        return Base<Impl>::next();
    }

    void discard() override
    {
        levtr = nullptr;
        Base<Impl>::discard();
    }
};

/// CursorData implementation
struct Data : public LevTrBase<Data>
{
protected:
    int insert_cur_prio;

    /// Append or replace the last result according to priority. Returns false if the value has been ignored.
    bool add_to_best_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var);
    /// Append or replace the last result according to datetime. Returns false if the value has been ignored.
    bool add_to_last_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var);

    void load(Tracer<>& trc, const DataQueryBuilder& qb);
    void load_best(Tracer<>& trc, const DataQueryBuilder& qb);
    void load_last(Tracer<>& trc, const DataQueryBuilder& qb);

public:
    bool with_attributes;

    Data(DataQueryBuilder& qb, bool with_attributes);

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }

    Datetime get_datetime() const override { return row().datetime; }
    wreport::Varcode get_varcode() const override { return row().value.code(); }
    wreport::Var get_var() const override { return *row().value; }
    int attr_reference_id() const override { return row().value.data_id; }
    Level get_level() const override { return get_levtr().level; }
    Trange get_trange() const override { return get_levtr().trange; }

    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override;
    void remove() override;
    void enq(impl::Enq& enq) const override;

protected:
    friend std::shared_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
};

/// CursorSummary implementation
struct Summary : public LevTrBase<Summary>
{
    using LevTrBase<Summary>::LevTrBase;

    DatetimeRange get_datetimerange() const override { return row().dtrange; }
    Level get_level() const override { return get_levtr().level; }
    Trange get_trange() const override { return get_levtr().trange; }
    wreport::Varcode get_varcode() const override { return row().code; }
    size_t get_count() const override { return row().count; }
    void remove() override;
    void enq(impl::Enq& enq) const override;

protected:
    void load(Tracer<>& trc, const SummaryQueryBuilder& qb);

    friend std::shared_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
};


std::shared_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::shared_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::shared_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::shared_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
void run_delete_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool station_vars, bool explain);

}
}
}
}
#endif
