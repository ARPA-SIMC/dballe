#ifndef DBA_DB_V7_CURSOR_H
#define DBA_DB_V7_CURSOR_H

#include <dballe/types.h>
#include <dballe/db/db.h>
#include <dballe/db/v7/transaction.h>
#include <dballe/db/v7/repinfo.h>
#include <dballe/db/v7/levtr.h>
#include <dballe/values.h>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

struct Stations;
struct StationData;
struct Data;
struct Best;
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


template<typename Row>
struct Rows
{
    /// Database to operate on
    std::shared_ptr<v7::Transaction> tr;

    /// Storage for the raw database results
    std::vector<Row> results;

    /// Iterator to the current position in results
    typename std::vector<Row>::const_iterator cur;

    /// True if we are at the start of the iteration
    bool at_start = true;

    Rows(std::shared_ptr<v7::Transaction> tr) : tr(tr) {}

    const Row* operator->() const { return &*cur; }

    int get_priority() const { return tr->repinfo().get_priority(cur->station.report); }

    bool next()
    {
        if (at_start)
            at_start = false;
        else if (cur != results.end())
            ++cur;
        return cur != results.end();
    }

    void discard()
    {
        at_start = false;
        cur = results.end();
    }
};

struct StationRows : public Rows<StationRow>
{
    using Rows::Rows;
    const DBValues& values() const;
    void load(Tracer<>& trc, const StationQueryBuilder& qb);
    template<typename Enq> void enq_generic(Enq& enq) const;
};

struct StationDataRows : public Rows<StationDataRow>
{
    using Rows::Rows;
    void load(Tracer<>& trc, const DataQueryBuilder& qb);
    template<typename Enq> void enq_generic(Enq& enq) const;
};

template<typename Row>
struct LevTrRows : public Rows<Row>
{
    using Rows<Row>::Rows;

    // Cached levtr for the current row
    mutable const LevTrEntry* levtr = nullptr;

    bool next()
    {
        levtr = nullptr;
        return Rows<Row>::next();
    }

    void discard()
    {
        levtr = nullptr;
        Rows<Row>::discard();
    }

    const LevTrEntry& get_levtr() const
    {
        if (levtr == nullptr)
            // We prefetch levtr info for all IDs, so we do not need to hit the database here
            levtr = &(this->tr->levtr().lookup_cache(this->cur->id_levtr));
        return *levtr;
    }
};

struct BaseDataRows : public LevTrRows<DataRow>
{
    using LevTrRows::LevTrRows;
    template<typename Enq> void enq_generic(Enq& enq) const;
};

struct DataRows : public BaseDataRows
{
    using BaseDataRows::BaseDataRows;
    void load(Tracer<>& trc, const DataQueryBuilder& qb);
};

struct BestRows : public BaseDataRows
{
    using BaseDataRows::BaseDataRows;

    int insert_cur_prio;

    /// Append or replace the last result according to priority. Returns false if the value has been ignored.
    bool add_to_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var);

    void load(Tracer<>& trc, const DataQueryBuilder& qb);
};

struct SummaryRows : public LevTrRows<SummaryRow>
{
    using LevTrRows::LevTrRows;
    void load(Tracer<>& trc, const SummaryQueryBuilder& qb);
    template<typename Enq> void enq_generic(Enq& enq) const;
};


template<typename Cursor>
struct ImplTraits
{
};

template<>
struct ImplTraits<Stations>
{
    typedef CursorStation Interface;
    typedef StationRow Row;
    typedef StationRows Rows;
};

template<>
struct ImplTraits<StationData>
{
    typedef CursorStationData Interface;
    typedef StationDataRow Row;
    typedef StationDataRows Rows;
};

template<>
struct ImplTraits<Data>
{
    typedef CursorData Interface;
    typedef DataRow Row;
    typedef DataRows Rows;
};

template<>
struct ImplTraits<Best>
{
    typedef CursorData Interface;
    typedef DataRow Row;
    typedef BestRows Rows;
};

template<>
struct ImplTraits<Summary>
{
    typedef CursorSummary Interface;
    typedef SummaryRow Row;
    typedef SummaryRows Rows;
};


/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
template<typename Impl>
struct Base : public ImplTraits<Impl>::Interface
{
    typedef typename ImplTraits<Impl>::Row Row;
    typedef typename ImplTraits<Impl>::Rows Rows;

    Rows rows;

    Base(std::shared_ptr<v7::Transaction> tr)
        : rows(tr)
    {
    }

    virtual ~Base() {}

    int remaining() const override;
    bool next() override { return rows.next(); }
    void discard() override;

    dballe::DBStation get_station() const override { return rows->station; }

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override;
};

extern template class Base<Stations>;
extern template class Base<StationData>;
extern template class Base<Data>;
extern template class Base<Best>;
extern template class Base<Summary>;


/// CursorStation implementation
struct Stations : public Base<Stations>
{
    using Base::Base;
    DBValues get_values() const override;

    void remove() override;
};

/// CursorStationData implementation
struct StationData : public Base<StationData>
{
    bool with_attributes;

    StationData(DataQueryBuilder& qb, bool with_attributes);
    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return rows.tr; }
    wreport::Varcode get_varcode() const override { return rows->value.code(); }
    wreport::Var get_var() const override { return *rows->value; }
    int attr_reference_id() const override { return rows->value.data_id; }
    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override;
    void remove() override;
};

/// CursorData implementation
struct Data : public Base<Data>
{
    bool with_attributes;

    Data(DataQueryBuilder& qb, bool with_attributes);

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return rows.tr; }

    Datetime get_datetime() const override { return rows->datetime; }
    wreport::Varcode get_varcode() const override { return rows->value.code(); }
    wreport::Var get_var() const override { return *rows->value; }
    int attr_reference_id() const override { return rows->value.data_id; }
    Level get_level() const override { return rows.get_levtr().level; }
    Trange get_trange() const override { return rows.get_levtr().trange; }

    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override;

    template<typename Enq> void enq_generic(Enq& enq) const;
    void remove() override;
};

/// CursorData with query=best implementation
struct Best : public Base<Best>
{
    bool with_attributes;

    Best(DataQueryBuilder& qb, bool with_attributes);

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return rows.tr; }

    Datetime get_datetime() const override { return rows->datetime; }
    wreport::Varcode get_varcode() const override { return rows->value.code(); }
    wreport::Var get_var() const override { return *rows->value; }
    int attr_reference_id() const override { return rows->value.data_id; }
    Level get_level() const override { return rows.get_levtr().level; }
    Trange get_trange() const override { return rows.get_levtr().trange; }

    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override;

    template<typename Enq> void enq_generic(Enq& enq) const;
    void remove() override;
};

/// CursorSummary implementation
struct Summary : public Base<Summary>
{
    using Base<Summary>::Base;

    DatetimeRange get_datetimerange() const override
    {
        return this->rows->dtrange;
    }
    Level get_level() const override { return rows.get_levtr().level; }
    Trange get_trange() const override { return rows.get_levtr().trange; }
    wreport::Varcode get_varcode() const override { return rows->code; }
    size_t get_count() const override { return rows->count; }

    template<typename Enq> void enq_generic(Enq& enq) const;
    void remove() override;
};


std::unique_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::unique_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::unique_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
std::unique_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool explain);
void run_delete_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& query, bool station_vars, bool explain);

}
}
}
}
#endif
