#ifndef DBALLE_DB_V7_MYSQL_DRIVER_H
#define DBALLE_DB_V7_MYSQL_DRIVER_H

#include <dballe/db/v7/driver.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

struct Driver : public v7::Driver
{
    dballe::sql::MySQLConnection& conn;

    Driver(dballe::sql::MySQLConnection& conn);
    virtual ~Driver();

    std::unique_ptr<v7::Repinfo> create_repinfo(v7::Transaction& tr) override;
    std::unique_ptr<v7::Station> create_station(v7::Transaction& tr) override;
    std::unique_ptr<v7::LevTr> create_levtr(v7::Transaction& tr) override;
    std::unique_ptr<v7::StationData> create_station_data(v7::Transaction& tr) override;
    std::unique_ptr<v7::Data> create_data(v7::Transaction& tr) override;
    void run_station_query(const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station&)>) override;
    void create_tables_v7() override;
    void delete_tables_v7() override;
    void vacuum_v7() override;
};

}
}
}
}
#endif
