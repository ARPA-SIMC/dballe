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
#include <dballe/db/pseudoana.h>
#include <dballe/db/internals.h>
#include <dballe/core/verbose.h>

#include <config.h>

#include <sql.h>
#include <sqlext.h>

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

dba_err dba_db_pseudoana_create(dba_db db, dba_db_pseudoana* ins)
{
	const char* select_fixed_query =
		"SELECT id FROM pseudoana WHERE lat=? AND lon=? AND ident IS NULL";
	const char* select_mobile_query =
		"SELECT id FROM pseudoana WHERE lat=? AND lon=? AND ident=?";
	const char* select_query =
		"SELECT lat, lon, ident FROM pseudoana WHERE id=?";
	const char* insert_query =
		"INSERT INTO pseudoana (lat, lon, ident)"
		" VALUES (?, ?, ?);";
	const char* update_query =
		"UPDATE pseudoana SET lat=?, lon=?, ident=? WHERE id=?";
	const char* remove_query =
		"DELETE FROM pseudoana WHERE id=?";
	dba_err err = DBA_OK;
	dba_db_pseudoana res = NULL;
	int r;

	if ((res = (dba_db_pseudoana)malloc(sizeof(struct _dba_db_pseudoana))) == NULL)
		return dba_error_alloc("creating a new dba_db_pseudoana");
	res->db = db;
	res->sfstm = NULL;
	res->smstm = NULL;
	res->sstm = NULL;
	res->istm = NULL;
	res->ustm = NULL;
	res->dstm = NULL;

	/* Create the statement for select fixed */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sfstm)));
	SQLBindParameter(res->sfstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->sfstm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindCol(res->sfstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->sfstm, (unsigned char*)select_fixed_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sfstm, "compiling query to look for fixed pseudoana IDs");
		goto cleanup;
	}

	/* Create the statement for select mobile */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->smstm)));
	SQLBindParameter(res->smstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->smstm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->smstm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	SQLBindCol(res->smstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->smstm, (unsigned char*)select_mobile_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->smstm, "compiling query to look for mobile pseudoana IDs");
		goto cleanup;
	}

	/* Create the statement for select station data */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sstm)));
	SQLBindParameter(res->sstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id), 0, 0);
	SQLBindCol(res->sstm, 1, SQL_C_SLONG, &(res->lat), sizeof(res->lat), 0);
	SQLBindCol(res->sstm, 2, SQL_C_SLONG, &(res->lon), sizeof(res->lon), 0);
	SQLBindCol(res->sstm, 3, SQL_C_CHAR, &(res->ident), sizeof(res->ident), &(res->ident_ind));
	r = SQLPrepare(res->sstm, (unsigned char*)select_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sstm, "compiling query to look for pseudoana data by ID");
		goto cleanup;
	}

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'pseudoana'");
		goto cleanup;
	}

	/* Create the statement for update */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->ustm)));
	SQLBindParameter(res->ustm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->ustm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->ustm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	SQLBindParameter(res->ustm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id), 0, 0);
	r = SQLPrepare(res->ustm, (unsigned char*)update_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->ustm, "compiling query to update pseudoana");
		goto cleanup;
	}

	/* Create the statement for remove */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->dstm)));
	SQLBindParameter(res->dstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id), 0, 0);
	r = SQLPrepare(res->dstm, (unsigned char*)remove_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->dstm, "compiling query to remove from pseudoana");
		goto cleanup;
	}

	*ins = res;
	res = NULL;
	
cleanup:
	if (res != NULL)
		dba_db_pseudoana_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
};

void dba_db_pseudoana_delete(dba_db_pseudoana ins)
{
	if (ins->sfstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sfstm);
	if (ins->smstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->smstm);
	if (ins->sstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sstm);
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	if (ins->ustm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->ustm);
	free(ins);
}

void dba_db_pseudoana_set_ident(dba_db_pseudoana ins, const char* ident)
{
	if (ident == NULL)
	{
		ins->ident[0] = 0;
		ins->ident_ind = SQL_NULL_DATA; 
	} else {
		int len = strlen(ident);
		if (len > 64) len = 64;
		memcpy(ins->ident, ident, len);
		ins->ident[len] = 0;
		ins->ident_ind = len; 
	}
}

dba_err dba_db_pseudoana_get_id(dba_db_pseudoana ins, int *id)
{
	SQLHSTMT stm = ins->ident_ind == SQL_NULL_DATA ? ins->sfstm : ins->smstm;

	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "looking for pseudoana IDs");

	/* Get the result */
	if (SQLFetch(stm) != SQL_NO_DATA)
		*id = ins->id;
	else
		*id = -1;

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_pseudoana_get_id cursor");

	return dba_error_ok();
}

dba_err dba_db_pseudoana_get_data(dba_db_pseudoana ins, int id)
{
	int res;
	ins->id = id;
	res = SQLExecute(ins->sstm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->sstm, "looking for pseudoana information");
	/* Get the result */
	if (SQLFetch(ins->sstm) == SQL_NO_DATA)
		return dba_error_notfound("looking for information for ana_id %d", id);
	res = SQLCloseCursor(ins->sstm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->sstm, "closing dba_db_pseudoana_get_data cursor");
	if (ins->ident_ind == SQL_NULL_DATA)
		ins->ident[0] = 0;
	return dba_error_ok();
}

dba_err dba_db_pseudoana_insert(dba_db_pseudoana ins, int *id)
{
	int res = SQLExecute(ins->istm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->istm, "inserting new data into pseudoana");

	return dba_db_last_insert_id(ins->db, id);
}

dba_err dba_db_pseudoana_update(dba_db_pseudoana ins)
{
	int res = SQLExecute(ins->ustm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->ustm, "updating pseudoana");

	return dba_error_ok();
}

dba_err dba_db_pseudoana_remove(dba_db_pseudoana ins)
{
	int res = SQLExecute(ins->dstm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->dstm, "removing a pseudoana record");

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
