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

#ifndef DBALLE_DB_POSTGRESQL_V5_STATION_H
#define DBALLE_DB_POSTGRESQL_V5_STATION_H

/** @file
 * @ingroup db
 *
 * Station table management used by the db module.
 */

#include <dballe/db/sql/station.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
struct PostgreSQLConnection;
namespace postgresql {
struct Result;

class StationBase : public sql::Station
{
protected:
    /**
     * DB connection.
     */
    PostgreSQLConnection& conn;

    /// Lookup the ID of a station, returning true if it was found, false if not
    bool maybe_get_id(int lat, int lon, const char* ident, int* id);

    StationBase(const StationBase&) = delete;
    StationBase(const StationBase&&) = delete;
    StationBase& operator=(const StationBase&) = delete;

public:
    StationBase(PostgreSQLConnection& conn);
    ~StationBase();

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

}

namespace v5 {

/**
 * Precompiled queries to manipulate the station table
 */
class PostgreSQLStation : public postgresql::StationBase
{
public:
    PostgreSQLStation(PostgreSQLConnection& conn);
    ~PostgreSQLStation();
};

}

namespace v6 {

class PostgreSQLStation : public postgresql::StationBase
{
public:
    PostgreSQLStation(PostgreSQLConnection& conn);
    ~PostgreSQLStation();
};

}

}
}
#endif
