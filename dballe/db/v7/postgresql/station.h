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
    PostgreSQLStation(dballe::sql::PostgreSQLConnection& conn);
    ~PostgreSQLStation();
    PostgreSQLStation(const PostgreSQLStation&) = delete;
    PostgreSQLStation(const PostgreSQLStation&&) = delete;
    PostgreSQLStation& operator=(const PostgreSQLStation&) = delete;

    int maybe_get_id(v7::Transaction& tr, const dballe::Station& st) override;
    const dballe::Station* lookup_id(v7::Transaction& tr, int id) override;
    int obtain_id(v7::Transaction& tr, const dballe::Station& desc) override;

    void get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
};

}
}
}
}
#endif
