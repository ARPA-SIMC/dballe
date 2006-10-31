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
#include <dballe/db/internals.h>
#include <dballe/db/repinfo.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>
#include <dballe/db/data.h>
#include <dballe/db/attr.h>
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
#ifdef USE_MYSQL4
#define DBA_USE_DELETE_USING
#define DBA_USE_TRANSACTIONS
#endif


/**
 * Copy informations from the ODBC diagnostic record to the dba error
 * report
 */
dba_err dba_db_error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...)
{
	va_list ap;
	static const int strsize = 200;
	char stat[10], msg[strsize];
	char* context;
	SQLINTEGER err;
	SQLSMALLINT mlen;

	SQLGetDiagRec(handleType, handle, 1, (unsigned char*)stat, &err, (unsigned char*)msg, strsize, &mlen);
	if (mlen > strsize) mlen = strsize;

#if 0
	fprintf(stderr, "SQL ERROR CODE: %s\n", stat);
#endif

	va_start(ap, fmt);
	vasprintf(&context, fmt, ap);
	va_end(ap);

	return dba_error_generic0(DBA_ERR_ODBC, context, strndup(msg, mlen));
}

dba_err dba_db_error_odbc_except(const char* error_to_ignore, SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...)
{
	va_list ap;
	static const int strsize = 200;
	char stat[10], msg[strsize];
	char* context;
	SQLINTEGER err;
	SQLSMALLINT mlen;

	SQLGetDiagRec(handleType, handle, 1, (unsigned char*)stat, &err, (unsigned char*)msg, strsize, &mlen);
	if (mlen > strsize) mlen = strsize;

#if 0
	fprintf(stderr, "SQL ERROR CODE: %s\n", stat);
#endif

	// Ignore the given SQL error
	if (memcmp(stat, error_to_ignore, 5) == 0)
		return DBA_OK;

	va_start(ap, fmt);
	vasprintf(&context, fmt, ap);
	va_end(ap);

	return dba_error_generic0(DBA_ERR_ODBC, context, strndup(msg, mlen));
}

dba_err dba_db_statement_create(dba_db db, SQLHSTMT* stm)
{
	int res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		dba_err err = dba_db_error_odbc(SQL_HANDLE_STMT, *stm, "Allocating new statement handle");
		*stm = NULL;
		return err;
	}
	return dba_error_ok();
}

dba_err dba_db_last_insert_id(dba_db db, int* id)
{
	int res;
	*id = -1;

	res = SQLExecute(db->stm_last_insert_id);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "querying last inserted ID");

	if (SQLFetch(db->stm_last_insert_id) == SQL_NO_DATA)
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "retrieving results of query for last inserted ID");

	res = SQLCloseCursor(db->stm_last_insert_id);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "closing dba_db_last_insert_id cursor");

	*id = db->last_insert_id;

	return dba_error_ok();
}


#ifdef DBA_USE_TRANSACTIONS
dba_err dba_db_begin(dba_db db)
{
	int res = SQLExecute(db->stm_begin);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_begin, "Beginning a transaction");
	return dba_error_ok();
}

dba_err dba_db_commit(dba_db db)
{
	int res = SQLExecute(db->stm_commit);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_commit, "Committing a transaction");
	return dba_error_ok();
}

/* Run unchecked to avoid altering the error status */
void dba_db_rollback(dba_db db)
{
	int res = SQLExecute(db->stm_rollback);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return;
}
#else
/* TODO: lock and unlock tables instead */
dba_err dba_db_begin(dba_db db) { return dba_error_ok(); }
dba_err dba_db_commit(dba_db db) { return dba_error_ok(); }
void dba_db_rollback(dba_db db) {}
#endif

dba_err dba_db_need_repinfo(dba_db db)
{
	if (db->repinfo == NULL)
		return dba_db_repinfo_create(db, &(db->repinfo));
	return dba_error_ok();
}
dba_err dba_db_need_pseudoana(dba_db db)
{
	if (db->pseudoana == NULL)
		return dba_db_pseudoana_create(db, &(db->pseudoana));
	return dba_error_ok();
}
dba_err dba_db_need_context(dba_db db)
{
	if (db->context == NULL)
		return dba_db_context_create(db, &(db->context));
	return dba_error_ok();
}
dba_err dba_db_need_data(dba_db db)
{
	if (db->data == NULL)
		return dba_db_data_create(db, &(db->data));
	return dba_error_ok();
}
dba_err dba_db_need_attr(dba_db db)
{
	if (db->attr == NULL)
		return dba_db_attr_create(db, &(db->attr));
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
