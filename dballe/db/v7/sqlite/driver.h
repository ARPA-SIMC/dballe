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
    std::unique_ptr<v7::StationData> create_station_data() override;
    std::unique_ptr<v7::Data> create_data() override;
    void run_station_query(const v7::StationQueryBuilder& qb, std::function<void(int id, const StationDesc&)>) override;
    void run_station_data_query(const v7::DataQueryBuilder& qb, std::function<void(int id_station, const StationDesc& station, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void run_data_query(const v7::DataQueryBuilder& qb, std::function<void(int id_station, const StationDesc& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)>) override;
    void run_summary_query(const v7::SummaryQueryBuilder& qb, std::function<void(int id_station, const StationDesc& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)>) override;
    void create_tables_v7() override;
    void delete_tables_v7() override;
    void vacuum_v7() override;
};

}
}
}
}
#endif
