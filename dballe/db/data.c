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
#include <dballe/db/data.h>
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

dba_err dba_db_data_create(dba_db db, dba_db_data* ins)
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
	 * several examples with it */
	const char* insert_ignore_query_oracle =
		"MERGE INTO data USING"
		" (SELECT ? as cnt, ? as var, ? as val FROM dual)"
		" ON (id_context=cnt AND id_var=var)"
		" WHEN MATCHED THEN UPDATE SET value=value"
		" WHEN NOT MATCHED THEN"
		"  INSERT (id_context, id_var, value) VALUES (cnt, var, val)";

	dba_err err = DBA_OK;
	dba_db_data res = NULL;
	int r;

	if ((res = (dba_db_data)malloc(sizeof(struct _dba_db_data))) == NULL)
		return dba_error_alloc("creating a new dba_db_data");
	res->db = db;
	res->istm = NULL;
	res->ustm = NULL;

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'data'");
		goto cleanup;
	}

	/* Create the statement for replace */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->ustm)));
	if (db->server_type == POSTGRES)
	{
		SQLBindParameter(res->ustm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
		SQLBindParameter(res->ustm, 2, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
		SQLBindParameter(res->ustm, 3, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
	} else {
		SQLBindParameter(res->ustm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
		SQLBindParameter(res->ustm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
		SQLBindParameter(res->ustm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	}
	switch (db->server_type)
	{
		case MYSQL: 
			r = SQLPrepare(res->ustm, (unsigned char*)replace_query_mysql, SQL_NTS);
			break;
		case SQLITE: 
			r = SQLPrepare(res->ustm, (unsigned char*)replace_query_sqlite, SQL_NTS);
			break;
		case ORACLE: 
			r = SQLPrepare(res->ustm, (unsigned char*)replace_query_oracle, SQL_NTS);
			break;
		case POSTGRES: 
			r = SQLPrepare(res->ustm, (unsigned char*)replace_query_postgres, SQL_NTS);
			break;
		default:
			r = SQLPrepare(res->ustm, (unsigned char*)replace_query_postgres, SQL_NTS);
			break;
	}
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->ustm, "compiling query to update in 'data'");
		goto cleanup;
	}

	/* Create the statement for insert ignore */
	if (db->server_type != POSTGRES && db->server_type != ORACLE)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->iistm)));
		SQLBindParameter(res->iistm, 1, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
		SQLBindParameter(res->iistm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
		SQLBindParameter(res->iistm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
		switch (db->server_type)
		{
			case MYSQL: 
				r = SQLPrepare(res->iistm, (unsigned char*)insert_ignore_query_mysql, SQL_NTS);
				break;
			case SQLITE: 
				r = SQLPrepare(res->iistm, (unsigned char*)insert_ignore_query_sqlite, SQL_NTS);
				break;
			case ORACLE: 
				r = SQLPrepare(res->iistm, (unsigned char*)insert_ignore_query_oracle, SQL_NTS);
				break;
			default:
				r = SQLPrepare(res->iistm, (unsigned char*)insert_ignore_query_sqlite, SQL_NTS);
				break;
		}
		if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, res->iistm, "compiling query to insert/ignore in 'data'");
			goto cleanup;
		}
	}

	*ins = res;
	res = NULL;
	
cleanup:
	if (res != NULL)
		dba_db_data_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
};

void dba_db_data_delete(dba_db_data ins)
{
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	if (ins->ustm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->ustm);
	free(ins);
}

void dba_db_data_set(dba_db_data ins, dba_var var)
{
	ins->id_var = dba_var_code(var);
	dba_db_data_set_value(ins, dba_var_value(var));
}

void dba_db_data_set_value(dba_db_data ins, const char* value)
{
	int len = strlen(value);
	if (len > 255) len = 255;
	memcpy(ins->value, value, len);
	ins->value[len] = 0;
	ins->value_ind = len;
}

dba_err dba_db_data_insert_or_fail(dba_db_data ins)
{
	SQLHSTMT stm = ins->istm;
	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting data into table 'data'");
	return dba_error_ok();
}

dba_err dba_db_data_insert_or_ignore(dba_db_data ins, int* inserted)
{
	if (ins->db->server_type == POSTGRES || ins->db->server_type == ORACLE)
	{
		/* Try to insert, but ignore error results */
		SQLHSTMT stm = ins->istm;
		int res = SQLExecute(stm);
		/* TODO: when testing on postgres, find out the proper error code to
		 * ignore and still act on the others */
		*inserted = ((res == SQL_SUCCESS) || (res == SQL_SUCCESS_WITH_INFO));
		if (!*inserted)
		{
			const char* code;

			switch (ins->db->server_type)
			{
				/* Code for "Unique constraint violated" */
				case ORACLE: code = "23000"; break;
				case POSTGRES: code = "FIXME"; break;
				default: code = "FIXME"; break;
			}

			DBA_RUN_OR_RETURN(dba_db_error_odbc_except(code,
						SQL_HANDLE_STMT, stm,
						"inserting data into table 'data'"));
		}
		return dba_error_ok();
	} else {
		SQLLEN rowcount;
		SQLHSTMT stm = ins->iistm;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting/ignoring data into table 'data'");
        res = SQLRowCount(stm, &rowcount);
        if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
            return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
		*inserted = (rowcount != 0);
	}
	return dba_error_ok();
}

dba_err dba_db_data_insert_or_overwrite(dba_db_data ins)
{
	if (ins->db->server_type == POSTGRES)
	{
		/* TODO for postgres: do the insert or replace with:
		 * 1. lock table (or do this somehow atomically)
		 * 2. update
		 * 3. if it says not found, then insert
		 * 4. unlock table
		 */
		SQLHSTMT stm = ins->ustm;
		SQLLEN rowcount;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO) && (res != SQL_NO_DATA))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, 
						"updating data into table 'data'");
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
							  "inserting data into table 'data'");
		}
	} else {
		SQLHSTMT stm = ins->ustm;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting/rewriting data into table 'data'");
	}
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
