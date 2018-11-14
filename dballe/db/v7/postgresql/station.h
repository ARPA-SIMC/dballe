#ifndef DBALLE_DB_V7_POSTGRESQL_STATION_H
#define DBALLE_DB_V7_POSTGRESQL_STATION_H

#include <dballe/db/v7/station.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

/**
 * Precompiled queries to manipulate the station table
 */
class PostgreSQLStation : public v7::Station
{
protected:
    /**
     * DB connection.
     */
    dballe::sql::PostgreSQLConnection& conn;

    void _dump(std::function<void(int, int, const Coords& coords, const char* ident)> out) override;

public:
    PostgreSQLStation(v7::Transaction& tr, dballe::sql::PostgreSQLConnection& conn);
    ~PostgreSQLStation();
    PostgreSQLStation(const PostgreSQLStation&) = delete;
    PostgreSQLStation(const PostgreSQLStation&&) = delete;
    PostgreSQLStation& operator=(const PostgreSQLStation&) = delete;

    int maybe_get_id(Tracer<>& trc, const dballe::DBStation& st) override;
    int insert_new(Tracer<>& trc, const dballe::DBStation& desc) override;
    void get_station_vars(Tracer<>& trc, int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(Tracer<>& trc, int id_station, core::DBValues& values) override;
    void run_station_query(Tracer<>& trc, const v7::StationQueryBuilder& qb, std::function<void(const dballe::DBStation&)>) override;
};

}
}
}
}
#endif
