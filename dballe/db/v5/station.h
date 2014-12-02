/*
 * db/station - station table management
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_V5_STATION_H
#define DBALLE_DB_V5_STATION_H

/** @file
 * @ingroup db
 *
 * Station table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
namespace db {
struct Connection;
struct Statement;
struct Sequence;

namespace v5 {

/**
 * Precompiled queries to manipulate the station table
 */
class Station
{
protected:
    /**
     * DB connection.
     */
    db::Connection& conn;

    /** Station ID sequence, when the DB requires it */
    db::Sequence* seq_station;

    /** Precompiled select fixed station query */
    db::Statement* sfstm;
    /** Precompiled select mobile station query */
    db::Statement* smstm;
    /** Precompiled select data by station id query */
    db::Statement* sstm;
    /** Precompiled insert query */
    db::Statement* istm;
    /** Precompiled update query */
    db::Statement* ustm;
    /** Precompiled delete query */
    db::Statement* dstm;

    /** Station ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id;
    /** Station latitude SQL parameter */
    DBALLE_SQL_C_SINT_TYPE lat;
    /** Station longitude SQL parameter */
    DBALLE_SQL_C_SINT_TYPE lon;
    /** Mobile station identifier SQL parameter */
    char ident[64];
    /** Mobile station identifier indicator */
    SQLLEN ident_ind;

    /**
     * Set the mobile station identifier input value for this ::dba_db_station
     *
     * @param ident
     *   Value to use for ident.  NULL can be used to unset ident.
     */
    void set_ident(const char* ident);

    /**
     * Get station information given a station ID
     *
     * @param id
     *   ID of the station to query
     */
    void get_data(int id);

    /**
     * Update the information about a station entry
     */
    void update();

    /**
     * Remove a station record
     */
    void remove();

public:
    Station(db::Connection& conn);
    ~Station();

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It throws an exception if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int get_id(int lat, int lon, const char* ident=NULL);

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It creates the station record if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int obtain_id(int lat, int lon, const char* ident=NULL, bool* inserted=NULL);

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

    /**
     * Clear (if applicable) and recreate the table structure in the database
     */
    static void reset_db(db::Connection& conn);

private:
    // disallow copy
    Station(const Station&);
    Station& operator=(const Station&);
};

#if 0


#ifdef  __cplusplus
}
#endif

#endif

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
