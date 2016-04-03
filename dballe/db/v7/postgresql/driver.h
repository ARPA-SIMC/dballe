#ifndef DBALLE_DB_V7_POSTGRESQL_DRIVER_H
#define DBALLE_DB_V7_POSTGRESQL_DRIVER_H

#include <dballe/db/v7/driver.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

struct Driver : public v7::Driver
{
    dballe::sql::PostgreSQLConnection& conn;

    Driver(dballe::sql::PostgreSQLConnection& conn);
    virtual ~Driver();

    std::unique_ptr<v7::Repinfo> create_repinfo() override;
    std::unique_ptr<v7::Station> create_station() override;
    std::unique_ptr<v7::LevTr> create_levtr() override;
    std::unique_ptr<v7::Data> create_data() override;
    std::unique_ptr<v7::StationData> create_station_data() override;
    void run_built_query_v7(const v7::QueryBuilder& qb, std::function<void(v7::SQLRecordV7& rec)> dest) override;
    void run_station_query(const v7::QueryBuilder& qb, std::function<void(int id, const StationDesc&)>) override;
    void create_tables_v7() override;
    void delete_tables_v7() override;
    void vacuum_v7() override;
};

}
}
}
}
#endif
