#ifndef DBALLE_DB_V6_POSTGRESQL_DRIVER_H
#define DBALLE_DB_V6_POSTGRESQL_DRIVER_H

#include <dballe/db/v6/driver.h>

namespace dballe {

namespace sql {
struct PostgreSQLConnection;
}

namespace db {
namespace v6 {
namespace postgresql {

struct Driver : public v6::Driver
{
    dballe::sql::PostgreSQLConnection& conn;

    Driver(dballe::sql::PostgreSQLConnection& conn);
    virtual ~Driver();

    std::unique_ptr<v6::Repinfo> create_repinfov6() override;
    std::unique_ptr<v6::Station> create_stationv6() override;
    std::unique_ptr<v6::LevTr> create_levtrv6() override;
    std::unique_ptr<v6::DataV6> create_datav6() override;
    std::unique_ptr<v6::AttrV6> create_attrv6() override;
    void run_built_query_v6(const v6::QueryBuilder& qb, std::function<void(v6::SQLRecordV6& rec)> dest) override;
    void create_tables_v6() override;
    void delete_tables_v6() override;
    void vacuum_v6() override;
    void exec_no_data(const std::string& query) override;
    void explain(const std::string& query) override;
};

}
}
}
}
#endif
