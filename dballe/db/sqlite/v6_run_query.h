/*
 * db/odbc/v6_run_query - run a prebuilt query on the SQLite connector
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#ifndef DBALLE_DB_SQLITE_V6_RUN_QUERY_H
#define DBALLE_DB_SQLITE_V6_RUN_QUERY_H

#include <dballe/db/sqlite/internals.h>
#include <dballe/db/v6/internals.h>
#include <functional>

namespace dballe {
namespace db {
namespace v6 {
struct QueryBuilder;

// SQLite implementation for dballe::db::v6::run_built_query
void sqlite_run_built_query(SQLiteConnection& conn, const QueryBuilder& qb, std::function<void(SQLRecord& rec)> dest);

// SQLite implementation for dballe::db::v6::run_delete_query
void sqlite_run_delete_query(SQLiteConnection& conn, const QueryBuilder& qb);

}
}
}

#endif
