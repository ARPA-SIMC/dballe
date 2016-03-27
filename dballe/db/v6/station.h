#ifndef DBALLE_DB_V6_STATION_H
#define DBALLE_DB_V6_STATION_H

#include <dballe/sql/fwd.h>
#include <memory>
#include <cstdio>

namespace wreport {
struct Var;
}

namespace dballe {
struct Record;

namespace db {
namespace v6 {

struct Station
{
public:
    /// Instantiate a Station object for this connection
    //static std::unique_ptr<Station> create(Connection& conn);

    virtual ~Station();

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It throws an exception if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    virtual int get_id(int lat, int lon, const char* ident=NULL) = 0;

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It creates the station record if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    virtual int obtain_id(int lat, int lon, const char* ident=NULL, bool* inserted=NULL) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;

    /**
     * Export station variables
     */
    virtual void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Add all station variables (without attributes) to rec.
     *
     * If the same variable exists in many different networks, the one with the
     * highest priority will be used.
     */
    virtual void add_station_vars(int id_station, Record& rec) = 0;
};

}
}
}
#endif

