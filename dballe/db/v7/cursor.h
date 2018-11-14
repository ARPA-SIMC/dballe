#ifndef DBA_DB_V7_CURSOR_H
#define DBA_DB_V7_CURSOR_H

#include <dballe/types.h>
#include <dballe/db/db.h>
#include <dballe/db/v7/transaction.h>
#include <dballe/values.h>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

/**
 * Row resulting from a station query
 */
struct StationRow
{
    dballe::DBStation station;
    DBValues values;

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


/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
template<typename Interface, typename Row>
struct Base : public Interface
{
    /// Database to operate on
    std::shared_ptr<v7::Transaction> tr;

    /// Storage for the raw database results
    std::vector<Row> results;

    /// Iterator to the current position in results
    typename std::vector<Row>::const_iterator cur;

    /// True if we are at the start of the iteration
    bool at_start = true;

    Base(std::shared_ptr<v7::Transaction> tr)
        : tr(tr)
    {
    }

    virtual ~Base() {}

    int remaining() const override;
    bool next() override;
    void discard() override;

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    unsigned test_iterate(FILE* dump=0) override;
};


/**
 * CursorStation implementation
 */
struct Stations : public Base<CursorStation, StationRow>
{
    using Base::Base;
    dballe::DBStation get_station() const override { return this->cur->station; }
    DBValues get_values() const override { return this->cur->values; }
    void load(Tracer<>& trc, const StationQueryBuilder& qb);
    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
};


/**
 * CursorStationData implementation
 */
struct StationData : public Base<CursorStationData, StationDataRow>
{
    bool with_attributes;

    StationData(DataQueryBuilder& qb, bool with_attributes);
    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }
    void load(Tracer<>& trc, const DataQueryBuilder& qb);
    dballe::DBStation get_station() const override { return this->cur->station; }
    wreport::Varcode get_varcode() const override { return cur->value.code(); }
    wreport::Var get_var() const override { return *cur->value; }
    int attr_reference_id() const override { return cur->value.data_id; }
    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)>&& dest, bool force_read) override;
    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
};


template<typename Interface, typename Row>
struct LevtrBase : public Base<Interface, Row>
{
    using Base<Interface, Row>::Base;

    // Cached levtr for the current row
    mutable const LevTrEntry* levtr = nullptr;

    bool next() override
    {
        levtr = nullptr;
        return Base<Interface, Row>::next();
    }

    void discard() override
    {
        levtr = nullptr;
        Base<Interface, Row>::discard();
    }

    const LevTrEntry& get_levtr() const;
    Level get_level() const override;
    Trange get_trange() const override;
};


/**
 * CursorData implementation
 */
struct Data : public LevtrBase<CursorData, DataRow>
{
    bool with_attributes;

    Data(DataQueryBuilder& qb, bool with_attributes);

    std::shared_ptr<dballe::db::Transaction> get_transaction() const override { return tr; }

    void load(Tracer<>& trc, const DataQueryBuilder& qb);

    dballe::DBStation get_station() const override { return this->cur->station; }
    Datetime get_datetime() const override { return cur->datetime; }
    wreport::Varcode get_varcode() const override { return cur->value.code(); }
    wreport::Var get_var() const override { return *cur->value; }
    int attr_reference_id() const override { return cur->value.data_id; }

    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)>&& dest, bool force_read) override;

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
};


struct Summary : public LevtrBase<CursorSummary, SummaryRow>
{
    using LevtrBase<CursorSummary, SummaryRow>::LevtrBase;

    dballe::DBStation get_station() const override { return this->cur->station; }
    DatetimeRange get_datetimerange() const override
    {
        return this->cur->dtrange;
    }
    wreport::Varcode get_varcode() const override { return this->cur->code; }
    size_t get_count() const override { return this->cur->count; }

    void load(Tracer<>& trc, const SummaryQueryBuilder& qb);

    bool enqi(const char* key, unsigned len, int& res) const override;
    bool enqd(const char* key, unsigned len, double& res) const override;
    bool enqs(const char* key, unsigned len, std::string& res) const override;
    bool enqf(const char* key, unsigned len, std::string& res) const override;
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
