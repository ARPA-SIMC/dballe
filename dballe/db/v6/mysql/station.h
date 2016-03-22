#ifndef DBALLE_DB_V6_MYSQL_STATION_H
#define DBALLE_DB_V6_MYSQL_STATION_H

#include <dballe/db/v6/station.h>
#include <dballe/sql/fwd.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

/**
 * Precompiled queries to manipulate the station table
 */
class MySQLStationBase : public v6::Station
{
protected:
    /**
     * DB connection.
     */
    sql::MySQLConnection& conn;

    /// Lookup the ID of a station, returning true if it was found, false if not
    bool maybe_get_id(int lat, int lon, const char* ident, int* id);

    /// Run stm, read its output and generate variables to send to dest
    void read_station_vars(const std::string& query, std::function<void(std::unique_ptr<wreport::Var>)> dest);

public:
    MySQLStationBase(dballe::sql::MySQLConnection& conn);
    ~MySQLStationBase();
    MySQLStationBase(const MySQLStationBase&) = delete;
    MySQLStationBase(const MySQLStationBase&&) = delete;
    MySQLStationBase& operator=(const MySQLStationBase&) = delete;

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It throws an exception if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int get_id(int lat, int lon, const char* ident=nullptr) override;

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It creates the station record if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int obtain_id(int lat, int lon, const char* ident=nullptr, bool* inserted=NULL) override;

    void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

class MySQLStationV6 : public MySQLStationBase
{
public:
    MySQLStationV6(dballe::sql::MySQLConnection& conn);
};

}
}
}
}
#endif
