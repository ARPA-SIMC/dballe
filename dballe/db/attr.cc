/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#define _GNU_SOURCE
#include <dballe/db/attr.h>
#include <dballe/db/internals.h>
#include <dballe/core/verbose.h>

#include <sql.h>
#include <sqlext.h>

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

/*
 * Define to true to enable the use of transactions during writes
 */
/*
*/

dba_err dba_db_attr_create(dba_db db, dba_db_attr* ins)
{
	const char* select_query =
		"SELECT type, value FROM attr WHERE id_context=? and id_var=?";
	const char* insert_query =
		"INSERT INTO attr (id_context, id_var, type, value)"
		" VALUES(?, ?, ?, ?)";
	const char* replace_query_mysql =
		"INSERT INTO attr (id_context, id_var, type, value)"
		" VALUES(?, ?, ?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)";
	const char* replace_query_sqlite =
		"INSERT OR REPLACE INTO attr (id_context, id_var, type, value)"
		" VALUES(?, ?, ?, ?)";
	const char* replace_query_oracle =
		"MERGE INTO attr USING"
		" (SELECT ? as cnt, ? as var, ? as t, ? as val FROM dual)"
		" ON (id_context=cnt AND id_var=var AND type=t)"
		" WHEN MATCHED THEN UPDATE SET value=val"
		" WHEN NOT MATCHED THEN"
		"  INSERT (id_context, id_var, type, value) VALUES (cnt, var, t, val)";
	const char* replace_query_postgres =
		"UPDATE attr SET value=? WHERE id_context=? AND id_var=? AND type=?";
	dba_err err = DBA_OK;
	dba_db_attr res = NULL;
	int r;

	if ((res = (dba_db_attr)malloc(sizeof(struct _dba_db_attr))) == NULL)
		return dba_error_alloc("creating a new dba_db_attr");
	res->db = db;
	res->sstm = NULL;
	res->istm = NULL;
	res->rstm = NULL;

	/* Create the statement for select */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sstm)));
	SQLBindParameter(res->sstm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
	SQLBindParameter(res->sstm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
	SQLBindCol(res->sstm, 1, SQL_C_USHORT, &(res->type), sizeof(res->type), NULL);
	SQLBindCol(res->sstm, 2, SQL_C_CHAR, &(res->value), sizeof(res->value), NULL);
	r = SQLPrepare(res->sstm, (unsigned char*)select_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sstm, "compiling query to get data from 'attr'");
		goto cleanup;
	}

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->type), 0, 0);
	SQLBindParameter(res->istm, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'attr'");
		goto cleanup;
	}

	/* Create the statement for replace */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->rstm)));
	if (db->server_type == POSTGRES)
	{
		SQLBindParameter(res->rstm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
		SQLBindParameter(res->rstm, 2, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
		SQLBindParameter(res->rstm, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
		SQLBindParameter(res->rstm, 4, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->type), 0, 0);
	} else {
		SQLBindParameter(res->rstm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
		SQLBindParameter(res->rstm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
		SQLBindParameter(res->rstm, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->type), 0, 0);
		SQLBindParameter(res->rstm, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	}
	switch (db->server_type)
	{
		case MYSQL:
			r = SQLPrepare(res->rstm, (unsigned char*)replace_query_mysql, SQL_NTS);
			break;
		case SQLITE:
			r = SQLPrepare(res->rstm, (unsigned char*)replace_query_sqlite, SQL_NTS);
			break;
		case ORACLE:
			r = SQLPrepare(res->rstm, (unsigned char*)replace_query_oracle, SQL_NTS);
			break;
		case POSTGRES:
			r = SQLPrepare(res->rstm, (unsigned char*)replace_query_postgres, SQL_NTS);
			break;
		default:
			r = SQLPrepare(res->rstm, (unsigned char*)replace_query_mysql, SQL_NTS);
			break;
	}
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->rstm, "compiling query to replace in 'attr'");
		goto cleanup;
	}

	*ins = res;
	res = NULL;
	
cleanup:
	if (res != NULL)
		dba_db_attr_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
};

void dba_db_attr_delete(dba_db_attr ins)
{
	if (ins->sstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sstm);
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	if (ins->rstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->rstm);
	free(ins);
}

void dba_db_attr_set(dba_db_attr ins, dba_var var)
{
	ins->type = dba_var_code(var);
	dba_db_attr_set_value(ins, dba_var_value(var));
}

void dba_db_attr_set_value(dba_db_attr ins, const char* value)
{
	int len = strlen(value);
	if (len > 255) len = 255;
	memcpy(ins->value, value, len);
	ins->value[len] = 0;
	ins->value_ind = len;
}

dba_err dba_db_attr_insert(dba_db_attr ins, int replace)
{
	if (ins->db->server_type == POSTGRES && replace)
	{
		/* TODO for postgres: do the insert or replace with:
		 * 1. lock table (or do this somehow atomically)
		 * 2. update
		 * 3. if it says not found, then insert
		 * 4. unlock table
		 */
		SQLHSTMT stm = ins->rstm;
		SQLLEN rowcount;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO) && (res != SQL_NO_DATA))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, 
						"updating data into table 'attr'");
        res = SQLRowCount(stm, &rowcount);
        if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
            return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
		if (rowcount == 0)
		{
			/* Update failed, the element did not exist.  Create it */
			stm = ins->istm;
			int res = SQLExecute(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
				return dba_db_error_odbc(SQL_HANDLE_STMT, stm, 
							  "inserting data into table 'attr'");
		}
	} else {
		SQLHSTMT stm = replace ? ins->rstm : ins->istm;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting data into attr");
	}
	return dba_error_ok();
}

dba_err dba_db_attr_load(dba_db_attr ins, dba_var var)
{
	dba_err err = DBA_OK;
	dba_var attr = NULL;

	ins->id_var = dba_var_code(var);

	int res = SQLExecute(ins->sstm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->sstm, "looking for attributes");

	/* Make attribues from the result, and add them to var */
	while (SQLFetch(ins->sstm) != SQL_NO_DATA)
	{
		DBA_RUN_OR_GOTO(fail, dba_var_create_local(ins->type, &attr));
		DBA_RUN_OR_GOTO(fail, dba_var_setc(attr, ins->value));
		DBA_RUN_OR_GOTO(fail, dba_var_seta(var, attr));
		attr = NULL;
	}

	res = SQLCloseCursor(ins->sstm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->sstm, "closing dba_db_attr_load cursor");
	return dba_error_ok();

fail:
	res = SQLCloseCursor(ins->sstm);
	if (attr != NULL)
		dba_var_delete(attr);
	return err;
}

/* vim:set ts=4 sw=4: */
