#ifndef DBALLE_DB_V7_MYSQL_STATION_H
#define DBALLE_DB_V7_MYSQL_STATION_H

#include <dballe/db/v7/station.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

/**
 * Precompiled queries to manipulate the station table
 */
class MySQLStation : public v7::Station
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::MySQLConnection& conn;

    void
    _dump(std::function<void(int, int, const Coords& coords, const char* ident)>
              out) override;

public:
    MySQLStation(v7::Transaction& tr, dballe::sql::MySQLConnection& conn);
    ~MySQLStation();
    MySQLStation(const MySQLStation&)            = delete;
    MySQLStation(const MySQLStation&&)           = delete;
    MySQLStation& operator=(const MySQLStation&) = delete;

    DBStation lookup(Tracer<>& trc, int id_station) override;
    int maybe_get_id(Tracer<>& trc, const dballe::DBStation& st) override;
    int insert_new(Tracer<>& trc, const dballe::DBStation& desc) override;
    void get_station_vars(
        Tracer<>& trc, int id_station,
        std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(Tracer<>& trc, int id_station,
                          DBValues& values) override;
    void
    run_station_query(Tracer<>& trc, const v7::StationQueryBuilder& qb,
                      std::function<void(const dballe::DBStation&)>) override;
};

} // namespace mysql
} // namespace v7
} // namespace db
} // namespace dballe
#endif
