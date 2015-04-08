/*
 * db/v6/attr - attr table management
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

#include "config.h"
#include "internals.h"
#include "db.h"
#include "dballe/db/sqlite/internals.h"
#include "dballe/db/sqlite/repinfo.h"
#include "dballe/db/sqlite/station.h"
#include "dballe/db/sqlite/levtr.h"
#include "dballe/db/sqlite/datav6.h"
#include "dballe/db/sqlite/attrv6.h"
#include "dballe/db/sqlite/v6_run_query.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/postgresql/internals.h"
#include "dballe/db/postgresql/repinfo.h"
#include "dballe/db/postgresql/station.h"
#include "dballe/db/postgresql/levtr.h"
#include "dballe/db/postgresql/datav6.h"
#include "dballe/db/postgresql/attrv6.h"
#endif
#ifdef HAVE_ODBC
#include "dballe/db/odbc/internals.h"
#include "dballe/db/odbc/repinfo.h"
#include "dballe/db/odbc/station.h"
#include "dballe/db/odbc/levtr.h"
#include "dballe/db/odbc/datav6.h"
#include "dballe/db/odbc/attrv6.h"
#include "dballe/db/odbc/v6_run_query.h"
#endif
#include "dballe/core/record.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v6 {

std::unique_ptr<sql::Repinfo> create_repinfo(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::SQLiteRepinfo(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::ODBCRepinfo(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::PostgreSQLRepinfo(*c));
#endif
    else
        throw error_unimplemented("v6 DB repinfo only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

std::unique_ptr<sql::Station> create_station(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::SQLiteStation(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::ODBCStation(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::PostgreSQLStation(*c));
#endif
    else
        throw error_unimplemented("v6 DB station only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

unique_ptr<sql::LevTr> create_levtr(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::SQLiteLevTr(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::ODBCLevTr(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::PostgreSQLLevTr(*c));
#endif
    else
        throw error_unimplemented("v6 DB levtr only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

unique_ptr<sql::DataV6> create_datav6(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::DataV6>(new SQLiteDataV6(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::DataV6>(new ODBCDataV6(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::DataV6>(new PostgreSQLDataV6(*c));
#endif
    else
        throw error_unimplemented("DB datav6 only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

unique_ptr<sql::AttrV6> create_attrv6(Connection& conn)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::AttrV6>(new SQLiteAttrV6(*c));
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::AttrV6>(new ODBCAttrV6(*c));
#endif
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::AttrV6>(new PostgreSQLAttrV6(*c));
#endif
    else
        throw error_unimplemented("DB attrv6 only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}


bool SQLRecord::querybest_fields_are_the_same(const SQLRecord& r)
{
    if (out_ana_id != r.out_ana_id) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (out_datetime != r.out_datetime) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

void run_built_query(Connection& conn, const QueryBuilder& qb, std::function<void(SQLRecord& rec)> dest)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        sqlite_run_built_query(*c, qb, dest);
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        odbc_run_built_query(*c, qb, dest);
#endif
    else
        throw error_unimplemented("DB run_built_query_v6 only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

void run_delete_query(Connection& conn, const QueryBuilder& qb)
{
    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        sqlite_run_delete_query(*c, qb);
#ifdef HAVE_ODBC
    else if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        odbc_run_delete_query(*c, qb);
#endif
    else
        throw error_unimplemented("DB run_delete_query_v6 only implemented for ODBC, SQLite and PostgreSQL connectors, when available");
}

}
}
}

