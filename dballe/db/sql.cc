/*
 * db/sql - Generic infrastructure for talking with SQL databases
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
#include "sql.h"
#include "config.h"
#include "sqlite/internals.h"
#ifdef HAVE_ODBC
#include "odbc/internals.h"
#endif
#ifdef HAVE_LIBPQ
#include "postgresql/internals.h"
#endif
#ifdef HAVE_MYSQL
#include "mysql/internals.h"
#endif
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

Connection::~Connection() {}

void Connection::add_datetime(Querybuf& qb, const Datetime& dt) const
{
    qb.appendf("'%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu'",
            dt.year, dt.month, dt.day,
            dt.hour, dt.minute, dt.second);
}

std::unique_ptr<Connection> Connection::create_from_url(const std::string& url)
{
    return create_from_url(url.c_str());
}

std::unique_ptr<Connection> Connection::create_from_url(const char* url)
{
    if (strncmp(url, "sqlite://", 9) == 0)
    {
        unique_ptr<SQLiteConnection> conn(new SQLiteConnection);
        conn->open_file(url + 9);
        return unique_ptr<Connection>(conn.release());
    }
    if (strncmp(url, "sqlite:", 7) == 0)
    {
        unique_ptr<SQLiteConnection> conn(new SQLiteConnection);
        conn->open_file(url + 7);
        return unique_ptr<Connection>(conn.release());
    }
    if (strncmp(url, "mem:", 4) == 0)
    {
        throw error_consistency("SQL connections are not available on mem: databases");
    }
    if (strncmp(url, "postgresql:", 11) == 0)
    {
#ifdef HAVE_LIBPQ
        unique_ptr<PostgreSQLConnection> conn(new PostgreSQLConnection);
        conn->open_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("PostgreSQL support is not available");
#endif
    }
    if (strncmp(url, "mysql:", 6) == 0)
    {
#ifdef HAVE_MYSQL
        unique_ptr<MySQLConnection> conn(new MySQLConnection);
        conn->open_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("MySQL support is not available");
#endif
    }
    if (strncmp(url, "odbc://", 7) == 0)
    {
#ifdef HAVE_ODBC
        unique_ptr<ODBCConnection> conn(new ODBCConnection);
        conn->connect_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("ODBC support is not available");
#endif
    }
    if (strncmp(url, "test:", 5) == 0)
    {
        const char* envurl = getenv("DBA_DB");
        if (envurl != NULL)
            return create_from_url(envurl);
        else
            return create_from_url("sqlite://test.sqlite");
    }
    error_consistency::throwf("unsupported url \"%s\"", url);
}

}
}
