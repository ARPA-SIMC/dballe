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

#include <dballe/core/verbose.h>
#include <dballe/cmdline.h>
#include <dballe/db/db.h>
#include <dballe/db/internals.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#define TIMER_DEF() struct timeval lasttime; gettimeofday(&lasttime, NULL)
#define PARTIAL(...) { \
	struct timeval nowtime; \
	int elapsed; \
	gettimeofday(&nowtime, NULL); \
	elapsed = (nowtime.tv_sec - lasttime.tv_sec) * 1000 + (nowtime.tv_usec - lasttime.tv_usec) / 1000; \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, " -- %d:%d.%d\n", elapsed / 60000, (elapsed / 1000) % 60, elapsed % 1000); \
}
#define TIME(...) { \
	struct timeval nowtime; \
	int elapsed; \
	gettimeofday(&nowtime, NULL); \
	elapsed = (nowtime.tv_sec - lasttime.tv_sec) * 1000 + (nowtime.tv_usec - lasttime.tv_usec) / 1000; \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, " -- %d:%d.%d\n", elapsed / 60000, (elapsed / 1000) % 60, elapsed % 1000); \
	lasttime.tv_sec = nowtime.tv_sec; \
	lasttime.tv_usec = nowtime.tv_usec; \
}

int main(int argc, char* argv[])
{
	dba_err err = DBA_OK;
	int res;
	dba_db db = NULL;
	SQLHSTMT stm = NULL;
	TIMER_DEF();

	dba_verbose_init();
	TIME("dba_init");
	DBA_RUN_OR_GOTO(cleanup, dba_db_create("big", "enrico", "", &db));
	TIME("dba_db_create");
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));
	TIME("dba_db_statement_create");

	res = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_SCROLLABLE, 
			(SQLPOINTER)SQL_NONSCROLLABLE, SQL_IS_INTEGER);
//	res = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_TYPE, 
//			(SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "setting SQL_CURSOR_FORWARD_ONLY");
		goto cleanup;
	}

	res = SQLExecDirect(stm, (unsigned char*)argv[1], SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Executing query %s", argv[1]);
		goto cleanup;
	}
	TIME("SQLExecDirect");

	{
		SQLINTEGER rowcount;
		res = SQLRowCount(stm, &rowcount);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
			goto cleanup;
		}
		res = rowcount;
	}
	TIME("SQLRowCount (%d)", res);

	res = 0;
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		if (res % 100000 == 0)
			PARTIAL("%d rows read so far", res);
		++res;
	}

	TIME("%d SQLFetch", res);

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	if (db != NULL)
		dba_db_delete(db);

	if (err == DBA_OK)
		return 0;
	else
	{
		dba_cmdline_print_dba_error();
		return 1;
	}
}

/* vim:set ts=4 sw=4: */
