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

#include "station.h"
#include "dballe/db/odbc/internals.h"
#include <sqltypes.h>
#include <cstring>
#include <sql.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

#define TABLETYPE "ENGINE=InnoDB;"
static const char* init_queries_mysql[] = {
    "CREATE TABLE station ("
    "   id         INTEGER auto_increment PRIMARY KEY,"
    "   lat        INTEGER NOT NULL,"
    "   lon        INTEGER NOT NULL,"
    "   ident      CHAR(64),"
    "   UNIQUE INDEX(lat, lon, ident(8)),"
    "   INDEX(lon)"
    ") " TABLETYPE,
};
static const char* init_queries_postgres[] = {
    "CREATE TABLE station ("
    "   id         SERIAL PRIMARY KEY,"
    "   lat        INTEGER NOT NULL,"
    "   lon        INTEGER NOT NULL,"
    "   ident      VARCHAR(64)"
    ") ",
    "CREATE UNIQUE INDEX pa_uniq ON station(lat, lon, ident)",
    "CREATE INDEX pa_lon ON station(lon)",
};
static const char* init_queries_sqlite[] = {
    "CREATE TABLE station ("
    "   id         INTEGER PRIMARY KEY,"
    "   lat        INTEGER NOT NULL,"
    "   lon        INTEGER NOT NULL,"
    "   ident      CHAR(64),"
    "   UNIQUE (lat, lon, ident)"
    ") ",
    "CREATE INDEX pa_lon ON station(lon)",
};
static const char* init_queries_oracle[] = {
    "CREATE TABLE station ("
    "   id         INTEGER PRIMARY KEY,"
    "   lat        INTEGER NOT NULL,"
    "   lon        INTEGER NOT NULL,"
    "   ident      VARCHAR2(64),"
    "   UNIQUE (lat, lon, ident)"
    ") ",
    "CREATE INDEX pa_lon ON station(lon)",
    "CREATE SEQUENCE seq_station",
};

/**
 * Precompiled queries to manipulate the station table
 */
class ODBCStation : public Station
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

public:
    ODBCStation(ODBCConnection& conn);
    ~ODBCStation();
    ODBCStation(const ODBCStation&) = delete;
    ODBCStation(const ODBCStation&&) = delete;
    ODBCStation& operator=(const ODBCStation&) = delete;

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

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

ODBCStation::ODBCStation(ODBCConnection& conn)
    : conn(conn), seq_station(0), sfstm(0), smstm(0), sstm(0), istm(0), ustm(0), dstm(0)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident=?";
    const char* select_query =
        "SELECT lat, lon, ident FROM station WHERE id=?";
    const char* insert_query =
        "INSERT INTO station (lat, lon, ident)"
        " VALUES (?, ?, ?);";
    const char* update_query =
        "UPDATE station SET lat=?, lon=?, ident=? WHERE id=?";
    const char* remove_query =
        "DELETE FROM station WHERE id=?";

    /* Override queries for some databases */
    switch (conn.server_type)
    {
        case ORACLE:
            seq_station = new db::Sequence(conn, "station_id_seq");
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (seq_station.NextVal, ?, ?, ?)";
            break;
        case POSTGRES:
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (nextval(pg_get_serial_sequence('station', 'id')), ?, ?, ?)";
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sfstm = conn.odbcstatement().release();
    sfstm->bind_in(1, lat);
    sfstm->bind_in(2, lon);
    sfstm->bind_out(1, id);
    sfstm->prepare(select_fixed_query);

    /* Create the statement for select mobile */
    smstm = conn.odbcstatement().release();
    smstm->bind_in(1, lat);
    smstm->bind_in(2, lon);
    smstm->bind_in(3, ident, ident_ind);
    smstm->bind_out(1, id);
    smstm->prepare(select_mobile_query);

    /* Create the statement for select station data */
    sstm = conn.odbcstatement().release();
    sstm->bind_in(1, id);
    sstm->bind_out(1, lat);
    sstm->bind_out(2, lon);
    sstm->bind_out(3, ident, sizeof(ident), ident_ind);
    sstm->prepare(select_query);

    /* Create the statement for insert */
    istm = conn.odbcstatement().release();
    istm->bind_in(1, lat);
    istm->bind_in(2, lon);
    istm->bind_in(3, ident, ident_ind);
    istm->prepare(insert_query);

    /* Create the statement for update */
    ustm = conn.odbcstatement().release();
    ustm->bind_in(1, lat);
    ustm->bind_in(2, lon);
    ustm->bind_in(3, ident, ident_ind);
    ustm->bind_in(4, id);
    ustm->prepare(update_query);

    /* Create the statement for remove */
    dstm = conn.odbcstatement().release();
    dstm->bind_in(1, id);
    dstm->prepare(remove_query);
}

ODBCStation::~ODBCStation()
{
    if (sfstm) delete sfstm;
    if (smstm) delete smstm;
    if (sstm) delete sstm;
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (dstm) delete dstm;
    if (seq_station) delete seq_station;
}

void ODBCStation::set_ident(const char* val)
{
    if (val)
    {
        int len = strlen(val);
        if (len > 64) len = 64;
        memcpy(ident, val, len);
        ident[len] = 0;
        ident_ind = len; 
    } else {
        ident[0] = 0;
        ident_ind = SQL_NULL_DATA; 
    }
}

int ODBCStation::get_id(int lat, int lon, const char* ident)
{
    this->lat = lat;
    this->lon = lon;
    set_ident(ident);
    ODBCStatement* stm = ident_ind == SQL_NULL_DATA ? sfstm : smstm;
    stm->execute();
    if (stm->fetch_expecting_one())
        return id;

    throw error_notfound("station not found in the database");
}

void ODBCStation::get_data(int qid)
{
    id = qid;
    sstm->execute();
    if (!sstm->fetch_expecting_one())
        error_notfound::throwf("looking for information for station id %d", qid);
    if (ident_ind == SQL_NULL_DATA)
        ident[0] = 0;
}

int ODBCStation::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    this->lat = lat;
    this->lon = lon;
    set_ident(ident);

    // Trying querying
    ODBCStatement* stm = ident_ind == SQL_NULL_DATA ? sfstm : smstm;
    stm->execute();
    if (stm->fetch_expecting_one())
    {
        if (inserted) *inserted = false;
        return id;
    }

    // If nothing was found, insert it
    if (inserted) *inserted = true;
    istm->execute_and_close();
    if (seq_station)
        return seq_station->read();
    else
        return conn.get_last_insert_id();
}

void ODBCStation::update()
{
    ustm->execute_and_close();
}

void ODBCStation::remove()
{
    dstm->execute_and_close();
}

void ODBCStation::dump(FILE* out)
{
    int id;
    int lat;
    int lon;
    char ident[64];
    SQLLEN ident_ind;

    auto stm = conn.odbcstatement();
    stm->bind_out(1, id);
    stm->bind_out(2, lat);
    stm->bind_out(3, lon);
    stm->bind_out(4, ident, 64, ident_ind);
    stm->exec_direct("SELECT id, lat, lon, ident FROM station");
    int count;
    fprintf(out, "dump of table station:\n");
    for (count = 0; stm->fetch(); ++count)
        if (ident_ind == SQL_NULL_DATA)
            fprintf(out, " %d, %.5f, %.5f\n", (int)id, lat/100000.0, lon/100000.0);
        else
            fprintf(out, " %d, %.5f, %.5f, %.*s\n", (int)id, lat/10000.0, lon/10000.0, (int)ident_ind, ident);
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
    stm->close_cursor();
}

void Station::reset_db(ODBCConnection& conn)
{
    conn.drop_table_if_exists("station");
    conn.drop_sequence_if_exists("seq_station");

    /* Allocate statement handle */
    auto stm = conn.odbcstatement();

    const char** queries = NULL;
    int query_count = 0;
    switch (conn.server_type)
    {
        case db::MYSQL:
            queries = init_queries_mysql;
            query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
        case db::SQLITE:
            queries = init_queries_sqlite;
            query_count = sizeof(init_queries_sqlite) / sizeof(init_queries_sqlite[0]); break;
        case db::ORACLE:
            queries = init_queries_oracle;
            query_count = sizeof(init_queries_oracle) / sizeof(init_queries_oracle[0]); break;
        case db::POSTGRES:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
        default:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
    }
    /* Create tables */
    for (int i = 0; i < query_count; i++)
        stm->exec_direct_and_close(queries[i]);
}

Station::~Station()
{
}

std::unique_ptr<Station> Station::create(db::ODBCConnection& conn)
{
    return unique_ptr<Station>(new ODBCStation(conn));
}

}
}
}
