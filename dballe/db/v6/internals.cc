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

#include "internals.h"
#include "db.h"
#include "dballe/db/sqlite/internals.h"
#include "dballe/db/sqlite/repinfo.h"
#include "dballe/db/sqlite/station.h"
#include "dballe/db/sqlite/levtr.h"
#include "dballe/db/sqlite/v6_data.h"
#include "dballe/db/sqlite/v6_attr.h"
#include "dballe/db/sqlite/v6_run_query.h"
#include "dballe/db/postgresql/internals.h"
#include "dballe/db/postgresql/repinfo.h"
#include "dballe/db/postgresql/station.h"
#include "dballe/db/postgresql/levtr.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/db/odbc/repinfo.h"
#include "dballe/db/odbc/station.h"
#include "dballe/db/odbc/levtr.h"
#include "dballe/db/odbc/v6_data.h"
#include "dballe/db/odbc/v6_attr.h"
#include "dballe/db/odbc/v6_run_query.h"
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
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::ODBCRepinfo(*c));
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::SQLiteRepinfo(*c));
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::Repinfo>(new v6::PostgreSQLRepinfo(*c));
    else
        throw error_unimplemented("v6 DB repinfo only implemented for ODBC and SQLite connectors");
}

std::unique_ptr<sql::Station> create_station(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::ODBCStation(*c));
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::SQLiteStation(*c));
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::Station>(new v6::PostgreSQLStation(*c));
    else
        throw error_unimplemented("v6 DB station not yet implemented for non-ODBC connectors");
}

unique_ptr<sql::LevTr> create_levtr(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::ODBCLevTr(*c));
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::SQLiteLevTr(*c));
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<sql::LevTr>(new v6::PostgreSQLLevTr(*c));
    else
        throw error_unimplemented("v6 DB LevTr only implemented ODBC and SQLite connectors");
}


Data::~Data() {}
unique_ptr<Data> Data::create(DB& db)
{
    if (ODBCConnection* conn = dynamic_cast<ODBCConnection*>(db.conn))
        return unique_ptr<Data>(new ODBCData(*conn));
    else if (SQLiteConnection* conn = dynamic_cast<SQLiteConnection*>(db.conn))
        return unique_ptr<Data>(new SQLiteData(*conn));
    else
        throw error_unimplemented("v6 DB Data only implemented for ODBC and SQLite connectors");
}


Attr::~Attr() {}
unique_ptr<Attr> Attr::create(DB& db)
{
    if (ODBCConnection* conn = dynamic_cast<ODBCConnection*>(db.conn))
        return unique_ptr<Attr>(new ODBCAttr(*conn));
    if (SQLiteConnection* conn = dynamic_cast<SQLiteConnection*>(db.conn))
        return unique_ptr<Attr>(new SQLiteAttr(*conn));
    else
        throw error_unimplemented("v6 DB attr only implemented for ODBC and SQLite connectors");
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
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        odbc_run_built_query(*c, qb, dest);
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        sqlite_run_built_query(*c, qb, dest);
    else
        throw error_unimplemented("v6 DB run_built_query not yet implemented for non-ODBC connectors");
}

void run_delete_query(Connection& conn, const QueryBuilder& qb)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        odbc_run_delete_query(*c, qb);
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        sqlite_run_delete_query(*c, qb);
    else
        throw error_unimplemented("v6 DB run_delete_query not yet implemented for non-ODBC connectors");
}

}
}
}

