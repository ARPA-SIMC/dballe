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

    /// Lookup the ID of a station, returning true if it was found, false if not
    bool maybe_get_id(const StationDesc& st, int* id);

    void _dump(std::function<void(int, int, const Coords& coords, const char* ident)> out) override;

public:
    PostgreSQLStation(dballe::sql::PostgreSQLConnection& conn);
    ~PostgreSQLStation();
    PostgreSQLStation(const PostgreSQLStation&) = delete;
    PostgreSQLStation(const PostgreSQLStation&&) = delete;
    PostgreSQLStation& operator=(const PostgreSQLStation&) = delete;

    stations_t::iterator lookup_id(State& st, int id) override;
    stations_t::iterator get_id(State& st, const StationDesc& desc) override;
    stations_t::iterator obtain_id(State& st, const StationDesc& desc) override;

    void get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
};

}
}
}
}
#endif
