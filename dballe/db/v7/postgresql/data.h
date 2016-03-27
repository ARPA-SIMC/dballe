#ifndef DBALLE_DB_V7_POSTGRESQL_DATA_H
#define DBALLE_DB_V7_POSTGRESQL_DATA_H

#include <dballe/db/v7/data.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
namespace postgresql {
struct DB;

class PostgreSQLStationData : public v7::StationData
{
protected:
    dballe::sql::PostgreSQLConnection& conn;

    void _dump(std::function<void(int, int, wreport::Varcode, const char*)> out) override;

public:
    PostgreSQLStationData(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLStationData(const PostgreSQLStationData&) = delete;
    PostgreSQLStationData(const PostgreSQLStationData&&) = delete;
    PostgreSQLStationData& operator=(const PostgreSQLStationData&) = delete;
    ~PostgreSQLStationData();

    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) override;
    void remove(const v7::QueryBuilder& qb) override;
};

class PostgreSQLData : public v7::Data
{
protected:
    dballe::sql::PostgreSQLConnection& conn;

    void _dump(std::function<void(int, int, int, const Datetime&, wreport::Varcode, const char*)> out) override;

public:
    PostgreSQLData(dballe::sql::PostgreSQLConnection& conn);
    PostgreSQLData(const PostgreSQLData&) = delete;
    PostgreSQLData(const PostgreSQLData&&) = delete;
    PostgreSQLData& operator=(const PostgreSQLData&) = delete;
    ~PostgreSQLData();

    void insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode=bulk::UPDATE) override;
    void remove(const v7::QueryBuilder& qb) override;
};

}
}
}
}
#endif
