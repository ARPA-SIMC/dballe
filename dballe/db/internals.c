/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#define DBA_USE_TRANSACTIONS


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

dba_err dba_db_seq_create(dba_db db, const char* name, dba_db_seq* seq)
{
	dba_err err = DBA_OK;
	dba_db_seq res = NULL;
	char qbuf[100];
	int qlen, r;

	if ((res = (dba_db_seq)malloc(sizeof(struct _dba_db_pseudoana))) == NULL)
		return dba_error_alloc("creating a new dba_db_pseudoana");
	res->stm = NULL;

	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->stm)));
	SQLBindCol(res->stm, 1, DBALLE_SQL_C_SINT, &(res->out), sizeof(res->out), 0);
	if (db->server_type == ORACLE)
		qlen = snprintf(qbuf, 100, "SELECT %s.CurrVal FROM dual", name);	
	else
		qlen = snprintf(qbuf, 100, "SELECT last_value FROM %s", name);	
	r = SQLPrepare(res->stm, (unsigned char*)qbuf, qlen);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->stm, "compiling query to read the current %s sequence value", name);
		goto cleanup;
	}

	*seq = res;
	res = NULL;

cleanup:
	if (res != NULL)
		dba_db_seq_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_seq_read(dba_db_seq seq)
{
	int res;
	res = SQLExecute(seq->stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, seq->stm, "reading sequence value");
	/* Get the result */
	if (SQLFetch(seq->stm) == SQL_NO_DATA)
		return dba_error_notfound("fetching results of sequence value reads");
	res = SQLCloseCursor(seq->stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, seq->stm, "closing dba_db_seq_read cursor");
	return dba_error_ok();
}

void dba_db_seq_delete(dba_db_seq seq)
{
	if (seq->stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, seq->stm);
	free(seq);
}

#define DBA_ODBC_MISSING_TABLE_POSTGRES "42P01"
#define DBA_ODBC_MISSING_TABLE_MYSQL "42S01"
#define DBA_ODBC_MISSING_TABLE_SQLITE "HY000"
#define DBA_ODBC_MISSING_TABLE_ORACLE "42S02"

dba_err dba_db_drop_table_if_exists(dba_db db, const char* name)
{
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	char buf[100];
	int len, res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	if (db->server_type == MYSQL)
	{
		len = snprintf(buf, 100, "DROP TABLE IF EXISTS %s", name);
		res = SQLExecDirect(stm, (unsigned char*)buf, len);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
					"Removing old table %s", name);
			goto cleanup;
		}
	} else {
		const char* code = DBA_ODBC_MISSING_TABLE_POSTGRES;

		switch (db->server_type)
		{
			case MYSQL: code = DBA_ODBC_MISSING_TABLE_MYSQL; break;
			case SQLITE: code = DBA_ODBC_MISSING_TABLE_SQLITE; break;
			case ORACLE: code = DBA_ODBC_MISSING_TABLE_ORACLE; break;
			case POSTGRES: code = DBA_ODBC_MISSING_TABLE_POSTGRES; break;
			default: code = DBA_ODBC_MISSING_TABLE_POSTGRES; break;
		}

		len = snprintf(buf, 100, "DROP TABLE %s", name);
		res = SQLExecDirect(stm, (unsigned char*)buf, len);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			DBA_RUN_OR_GOTO(cleanup, dba_db_error_odbc_except(code,
					SQL_HANDLE_STMT, stm,
					"Removing old table %s", name));
		DBA_RUN_OR_GOTO(cleanup, dba_db_commit(db));
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_run_sql(dba_db db, const char* query)
{
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	int res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	res = SQLExecDirect(stm, query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Running query '%s'", query);
		goto cleanup;
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

#ifdef DBA_USE_TRANSACTIONS
dba_err dba_db_begin(dba_db db)
{
	// TODO: set manual transaction and get rid of this method
	if (db->stm_begin == NULL) return dba_error_ok();
	int res = SQLExecute(db->stm_begin);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_begin, "Beginning a transaction");
	return dba_error_ok();
}

dba_err dba_db_commit(dba_db db)
{
	int res = SQLEndTran(SQL_HANDLE_DBC, db->od_conn, SQL_COMMIT);
	/*int res = SQLExecute(db->stm_commit);*/
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		/*return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_commit, "Committing a transaction");*/
		return dba_db_error_odbc(SQL_HANDLE_DBC, db->od_conn, "Committing a transaction");
	return dba_error_ok();
}

/* Run unchecked to avoid altering the error status */
void dba_db_rollback(dba_db db)
{
	/* int res = SQLExecute(db->stm_rollback); */
	int res = SQLEndTran(SQL_HANDLE_DBC, db->od_conn, SQL_ROLLBACK);
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
