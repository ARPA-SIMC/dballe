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

#include "station.h"
#include "dballe/db/odbc/station.h"
#include "dballe/db/odbc/internals.h"

using namespace wreport;
using namespace dballe::db;
using namespace std;

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


namespace dballe {
namespace db {
namespace sql {

Station::~Station()
{
}

#if 0
std::unique_ptr<Station> Station::create(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Station>(new ODBCStation(*c));
    else
        throw error_unimplemented("v5 station not yet implemented for non-ODBC connectors");
}
#endif

void Station::reset_db(Connection& conn)
{
    conn.drop_table_if_exists("station");
    conn.drop_sequence_if_exists("seq_station");

    const char** queries = NULL;
    unsigned query_count = 0;
    switch (conn.server_type)
    {
        case ServerType::MYSQL:
            queries = init_queries_mysql;
            query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
        case ServerType::SQLITE:
            queries = init_queries_sqlite;
            query_count = sizeof(init_queries_sqlite) / sizeof(init_queries_sqlite[0]); break;
        case ServerType::ORACLE:
            queries = init_queries_oracle;
            query_count = sizeof(init_queries_oracle) / sizeof(init_queries_oracle[0]); break;
        case ServerType::POSTGRES:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
        default:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
    }
    /* Create tables */
    for (unsigned i = 0; i < query_count; i++)
        conn.exec(queries[i]);
}

}
}
}
