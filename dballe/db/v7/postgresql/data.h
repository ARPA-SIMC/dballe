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

template<typename Traits>
class PostgreSQLDataCommon : public DataCommon<Traits>
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

    void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void write_attrs(int id_data, const Values& values) override;
    void remove_all_attrs(int id_data) override;
    void remove(const v7::IdQueryBuilder& qb) override;
};

extern template class PostgreSQLDataCommon<StationDataTraits>;
extern template class PostgreSQLDataCommon<DataTraits>;


class PostgreSQLStationData : public PostgreSQLDataCommon<StationDataTraits>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;
    StationValueCache cache;

    PostgreSQLStationData(dballe::sql::PostgreSQLConnection& conn);

    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override { return cache.clear(); }
};

class PostgreSQLData : public PostgreSQLDataCommon<DataTraits>
{
public:
    using PostgreSQLDataCommon::PostgreSQLDataCommon;
    ValueCache cache;

    PostgreSQLData(dballe::sql::PostgreSQLConnection& conn);

    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE, bool with_attrs=false) override;
    void dump(FILE* out) override;
    void clear_cache() override { return cache.clear(); }
};

}
}
}
}
#endif
