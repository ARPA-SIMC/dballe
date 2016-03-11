/*
 * db/station - station table management
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBALLE_DB_SQL_STATION_H
#define DBALLE_DB_SQL_STATION_H

/** @file
 * @ingroup db
 *
 * Station table management used by the db module.
 */

#include <dballe/sql/fwd.h>
#include <memory>
#include <cstdio>

namespace wreport {
struct Var;
}

namespace dballe {
struct Record;

namespace db {
namespace sql {

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

