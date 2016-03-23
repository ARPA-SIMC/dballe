#ifndef DBALLE_DB_V7_SQLITE_DRIVER_H
#define DBALLE_DB_V7_SQLITE_DRIVER_H

#include <dballe/db/v7/driver.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

struct Driver : public v7::Driver
{
    dballe::sql::SQLiteConnection& conn;

    Driver(dballe::sql::SQLiteConnection& conn);
    virtual ~Driver();

    std::unique_ptr<v7::Repinfo> create_repinfo() override;
    std::unique_ptr<v7::Station> create_station() override;
    std::unique_ptr<v7::LevTr> create_levtr() override;
    std::unique_ptr<v7::Data> create_data() override;
    std::unique_ptr<v7::Attr> create_attr() override;
    void run_built_query_v7(const v7::QueryBuilder& qb, std::function<void(v7::SQLRecordV7& rec)> dest) override;
    void create_tables_v7() override;
    void delete_tables_v7() override;
    void vacuum_v7() override;
    void exec_no_data(const std::string& query) override;
    void explain(const std::string& query) override;
};

}
}
}
}
#endif
