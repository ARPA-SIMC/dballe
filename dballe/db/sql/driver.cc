/*
 * db/sql/runqueryv6 - db-independent support for specific v6 queries
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

#include "driver.h"
#include "config.h"
#include "dballe/db/sqlite/driver.h"
#include "dballe/db/sqlite/internals.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/postgresql/driver.h"
#include "dballe/db/postgresql/internals.h"
#endif
#ifdef HAVE_ODBC
#include "dballe/db/odbc/driver.h"
#include "dballe/db/odbc/internals.h"
#endif

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

bool SQLRecordV6::querybest_fields_are_the_same(const SQLRecordV6& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (out_datetime != r.out_datetime) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

Driver::~Driver()
{
}

std::unique_ptr<Driver> Driver::create(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<Driver>(new sqlite::Driver(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Driver>(new odbc::Driver(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<Driver>(new postgresql::Driver(*c));
#endif
    else
        throw error_unimplemented("DB drivers only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

}
}
}
