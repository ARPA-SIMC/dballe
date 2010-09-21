/*
 * db/data - data table management
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "data.h"
#include "internals.h"

#include <sql.h>
#include <cstring>
#if 0
#define _GNU_SOURCE
#include <sqlext.h>

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <assert.h>
#endif

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {

Data::Data(Connection& conn)
    : conn(conn), istm(0), ustm(0), iistm(0)
{
	const char* insert_query =
		"INSERT INTO data (id_context, id_var, value) VALUES(?, ?, ?)";
	const char* replace_query_mysql =
		"INSERT INTO data (id_context, id_var, value) VALUES(?, ?, ?)"
		" ON DUPLICATE KEY UPDATE value=VALUES(value)";
	const char* replace_query_sqlite =
		"INSERT OR REPLACE INTO data (id_context, id_var, value) VALUES(?, ?, ?)";
	const char* replace_query_oracle =
		"MERGE INTO data USING"
		" (SELECT ? as cnt, ? as var, ? as val FROM dual)"
		" ON (id_context=cnt AND id_var=var)"
		" WHEN MATCHED THEN UPDATE SET value=val"
		" WHEN NOT MATCHED THEN"
		"  INSERT (id_context, id_var, value) VALUES (cnt, var, val)";
	const char* replace_query_postgres =
		"UPDATE data SET value=? WHERE id_context=? AND id_var=?";
	const char* insert_ignore_query_mysql =
		"INSERT IGNORE INTO data (id_context, id_var, value) VALUES(?, ?, ?)";
	const char* insert_ignore_query_sqlite =
		"INSERT OR IGNORE INTO data (id_context, id_var, value) VALUES(?, ?, ?)";
	/* FIXME: there is a useless WHEN MATCHED, but there does not seem a way to
	 * have a MERGE with only a WHEN NOT, although on the internet one finds
	 * several examples with it * /
	const char* insert_ignore_query_oracle =
		"MERGE INTO data USING"
		" (SELECT ? as cnt, ? as var, ? as val FROM dual)"
		" ON (id_context=cnt AND id_var=var)"
		" WHEN MATCHED THEN UPDATE SET value=value"
		" WHEN NOT MATCHED THEN"
		"  INSERT (id_context, id_var, value) VALUES (cnt, var, val)";
    */

	/* Create the statement for insert */
    istm = new db::Statement(conn);
	istm->bind_in(1, id_context);
	istm->bind_in(2, id_var);
	istm->bind_in(3, value, value_ind);
    istm->prepare(insert_query);

	/* Create the statement for replace */
    ustm = new db::Statement(conn);
	if (conn.server_type == POSTGRES)
	{
		ustm->bind_in(1, value, value_ind);
		ustm->bind_in(2, id_context);
		ustm->bind_in(3, id_var);
	} else {
		ustm->bind_in(1, id_context);
		ustm->bind_in(2, id_var);
		ustm->bind_in(3, value, value_ind);
	}
	switch (conn.server_type)
	{
		case MYSQL: ustm->prepare(replace_query_mysql); break;
		case SQLITE: ustm->prepare(replace_query_sqlite); break;
		case ORACLE: ustm->prepare(replace_query_oracle); break;
		case POSTGRES: ustm->prepare(replace_query_postgres); break;
		default: ustm->prepare(replace_query_postgres); break;
	}

	/* Create the statement for insert ignore */
    iistm = new db::Statement(conn);
    iistm->bind_in(1, id_context);
    iistm->bind_in(2, id_var);
    iistm->bind_in(3, value, value_ind);
	switch (conn.server_type)
    {
        case POSTGRES: iistm->prepare(insert_query); iistm->ignore_error = "FIXME"; break;
        case ORACLE: iistm->prepare(insert_query); iistm->ignore_error = "23000"; break;
        //case ORACLE: iistm->prepare(insert_ignore_query_oracle); break;
        case MYSQL: iistm->prepare(insert_ignore_query_mysql); break;
        case SQLITE: iistm->prepare(insert_ignore_query_sqlite); break;
        default: iistm->prepare(insert_ignore_query_sqlite); break;
    }
}

Data::~Data()
{
	if (istm) delete istm;
	if (ustm) delete ustm;
	if (iistm) delete iistm;
}

void Data::set(const wreport::Var& var)
{
	id_var = var.code();
	set_value(var.value());
}

void Data::set_value(const char* qvalue)
{
    if (qvalue == NULL)
    {
        value[0] = 0;
        value_ind = SQL_NULL_DATA;
    } else {
        int len = strlen(qvalue);
        if (len > 255) len = 255;
        memcpy(value, qvalue, len);
        value[len] = 0;
        value_ind = len;
    }
}

void Data::insert_or_fail()
{
    istm->execute();
}

bool Data::insert_or_ignore()
{
    int sqlres = iistm->execute();
	if (conn.server_type == POSTGRES || conn.server_type == ORACLE)
		return ((sqlres == SQL_SUCCESS) || (sqlres == SQL_SUCCESS_WITH_INFO));
	else
        return iistm->rowcount() != 0;
}

void Data::insert_or_overwrite()
{
	if (conn.server_type == POSTGRES)
	{
        if (ustm->execute() == SQL_NO_DATA)
            istm->execute();
	} else
        ustm->execute();
}

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
