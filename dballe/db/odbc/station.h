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

#ifndef DBALLE_DB_ODBC_STATION_H
#define DBALLE_DB_ODBC_STATION_H

#include <dballe/db/sql/station.h>
#include <sqltypes.h>
#include <functional>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {
namespace db {
struct ODBCConnection;
struct ODBCStatement;
struct Sequence;

namespace odbc {

/**
 * Precompiled queries to manipulate the station table
 */
class ODBCStationV5 : public sql::Station
{
protected:
    /**
     * DB connection.
     */
    ODBCConnection& conn;

    /** Station ID sequence, when the DB requires it */
    db::Sequence* seq_station;

    /** Precompiled select fixed station query */
    ODBCStatement* sfstm;
    /** Precompiled select mobile station query */
    ODBCStatement* smstm;
    /** Precompiled select data by station id query */
    ODBCStatement* sstm;
    /** Precompiled insert query */
    ODBCStatement* istm;
    /** Precompiled update query */
    ODBCStatement* ustm;
    /** Precompiled delete query */
    ODBCStatement* dstm;

    /** Station ID SQL parameter */
    int id;
    /** Station latitude SQL parameter */
    int lat;
    /** Station longitude SQL parameter */
    int lon;
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

    void impl_add_station_vars(const char* query, int id_station, Record& rec);

public:
    ODBCStationV5(ODBCConnection& conn);
    ~ODBCStationV5();
    ODBCStationV5(const ODBCStationV5&) = delete;
    ODBCStationV5(const ODBCStationV5&&) = delete;
    ODBCStationV5& operator=(const ODBCStationV5&) = delete;

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It throws an exception if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int get_id(int lat, int lon, const char* ident=NULL) override;

    /**
     * Get the station ID given latitude, longitude and mobile identifier.
     *
     * It creates the station record if it does not exist.
     *
     * @return
     *   Resulting ID of the station
     */
    int obtain_id(int lat, int lon, const char* ident=NULL, bool* inserted=NULL) override;

    void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};


class ODBCStationV6 : public ODBCStationV5
{
public:
    ODBCStationV6(ODBCConnection& conn);
    void get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void add_station_vars(int id_station, Record& rec) override;
};

}

}
}
#endif
