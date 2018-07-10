#ifndef DBALLE_DB_V7_SQLITE_DATA_H
#define DBALLE_DB_V7_SQLITE_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace sqlite {
struct DB;

// Partial implementation of the common parts of StationData and Data
template<typename Parent>
class SQLiteDataCommon : public Parent
{
protected:
    /// DB connection
    dballe::sql::SQLiteConnection& conn;

    /// Precompiled read attributes statement
    dballe::sql::SQLiteStatement* read_attrs_stm = nullptr;
    /// Precompiled write attributes statement
    dballe::sql::SQLiteStatement* write_attrs_stm = nullptr;
    /// Precompiled remove attributes statement
    dballe::sql::SQLiteStatement* remove_attrs_stm = nullptr;
    /// Precompiled select statement
    dballe::sql::SQLiteStatement* sstm = nullptr;
    /// Precompiled insert statement
    dballe::sql::SQLiteStatement* istm = nullptr;
    /// Precompiled update statement
    dballe::sql::SQLiteStatement* ustm = nullptr;

public:
    SQLiteDataCommon(v7::Transaction& tr, dballe::sql::SQLiteConnection& conn);
    SQLiteDataCommon(const SQLiteDataCommon&) = delete;
    SQLiteDataCommon(const SQLiteDataCommon&&) = delete;
    SQLiteDataCommon& operator=(const SQLiteDataCommon&) = delete;
    ~SQLiteDataCommon();

    void update(Tracer<>& trc, std::vector<typename Parent::BatchValue>& vars, bool with_attrs) override;
    void read_attrs(Tracer<>& trc, int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(Tracer<>& trc, int id_data, const Values& values) override;
    void remove_all_attrs(Tracer<>& trc, int id_data) override;
    void remove(Tracer<>& trc, const v7::IdQueryBuilder& qb) override;
};

extern template class SQLiteDataCommon<StationData>;
extern template class SQLiteDataCommon<Data>;

/**
 * Precompiled query to manipulate the station data table
 */
class SQLiteStationData : public SQLiteDataCommon<StationData>
{
public:
    using SQLiteDataCommon::SQLiteDataCommon;

    SQLiteStationData(v7::Transaction& tr, dballe::sql::SQLiteConnection& conn);

    void query(Tracer<>& trc, int id_station, std::function<void(int id, wreport::Varcode code)> dest) override;
    void insert(Tracer<>& trc, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs) override;
    void run_station_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteData : public SQLiteDataCommon<Data>
{
public:
    using SQLiteDataCommon::SQLiteDataCommon;

    SQLiteData(v7::Transaction& tr, dballe::sql::SQLiteConnection& conn);

    void query(Tracer<>& trc, int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) override;
    void insert(Tracer<>& trc, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs) override;
    void run_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void run_summary_query(Tracer<>& trc, const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

}
}
}
}
#endif
