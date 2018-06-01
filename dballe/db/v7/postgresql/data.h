#ifndef DBALLE_DB_V7_POSTGRESQL_DATA_H
#define DBALLE_DB_V7_POSTGRESQL_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/db/v7/cache.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace postgresql {
struct DB;

template<typename Parent>
class PostgreSQLDataCommon : public Parent
{
protected:
    /// DB connection
    dballe::sql::PostgreSQLConnection& conn;
    std::string select_attrs_query_name;
    std::string write_attrs_query_name;
    std::string remove_attrs_query_name;
    std::string remove_data_query_name;

public:
    PostgreSQLDataCommon(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLDataCommon(const PostgreSQLDataCommon&) = delete;
    PostgreSQLDataCommon(const PostgreSQLDataCommon&&) = delete;
    PostgreSQLDataCommon& operator=(const PostgreSQLDataCommon&) = delete;

    void update(dballe::db::v7::Transaction& t, std::vector<typename Parent::BatchValue>& vars, bool with_attrs=false) override;
    void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(int id_data, const Values& values) override;
    void remove_all_attrs(int id_data) override;
    void remove(const v7::IdQueryBuilder& qb) override;
};

extern template class PostgreSQLDataCommon<StationData>;
extern template class PostgreSQLDataCommon<Data>;


class PostgreSQLStationData : public PostgreSQLDataCommon<StationData>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;

    PostgreSQLStationData(dballe::sql::PostgreSQLConnection& conn);

    void query(int id_station, std::function<void(int id, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

class PostgreSQLData : public PostgreSQLDataCommon<Data>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;

    PostgreSQLData(dballe::sql::PostgreSQLConnection& conn);

    void query(int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) override;
    void insert(dballe::db::v7::Transaction& t, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override {}
};

}
}
}
}
#endif
