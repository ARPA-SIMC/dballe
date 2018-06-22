#ifndef DBALLE_DB_V7_MYSQL_DATA_H
#define DBALLE_DB_V7_MYSQL_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace mysql {
struct DB;

template<typename Parent>
class MySQLDataCommon : public Parent
{
protected:
    /// DB connection
    dballe::sql::MySQLConnection& conn;

#if 0
    /// Precompiled read attributes statement
    dballe::sql::MySQLStatement* read_attrs_stm = nullptr;
    /// Precompiled write attributes statement
    dballe::sql::MySQLStatement* write_attrs_stm = nullptr;
    /// Precompiled remove attributes statement
    dballe::sql::MySQLStatement* remove_attrs_stm = nullptr;
    /// Precompiled select statement
    dballe::sql::MySQLStatement* sstm = nullptr;
    /// Precompiled insert statement
    dballe::sql::MySQLStatement* istm = nullptr;
    /// Precompiled update statement
    dballe::sql::MySQLStatement* ustm = nullptr;
#endif

public:
    MySQLDataCommon(v7::Transaction& tr, dballe::sql::MySQLConnection& conn);
    MySQLDataCommon(const MySQLDataCommon&) = delete;
    MySQLDataCommon(const MySQLDataCommon&&) = delete;
    MySQLDataCommon& operator=(const MySQLDataCommon&) = delete;
    ~MySQLDataCommon();

    void update(dballe::db::v7::Transaction& t, std::vector<typename Parent::BatchValue>& vars, bool with_attrs) override;
    void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(int id_data, const Values& values) override;
    void remove_all_attrs(int id_data) override;
    void remove(const v7::IdQueryBuilder& qb) override;
};

extern template class MySQLDataCommon<StationData>;
extern template class MySQLDataCommon<Data>;

/**
 * Precompiled query to manipulate the station data table
 */
class MySQLStationData : public MySQLDataCommon<StationData>
{
public:
    using MySQLDataCommon::MySQLDataCommon;

    MySQLStationData(v7::Transaction& tr, dballe::sql::MySQLConnection& conn);

    void query(int id_station, std::function<void(int id, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs) override;
    void run_station_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

/**
 * Precompiled query to manipulate the data table
 */
class MySQLData : public MySQLDataCommon<Data>
{
public:
    using MySQLDataCommon::MySQLDataCommon;

    MySQLData(v7::Transaction& tr, dballe::sql::MySQLConnection& conn);

    void query(int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs) override;
    void run_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void run_summary_query(const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)>) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

}
}
}
}
#endif
