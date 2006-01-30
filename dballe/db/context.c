#define _GNU_SOURCE
#include <dballe/db/context.h>
#include <dballe/db/internals.h>
#include <dballe/core/verbose.h>

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
#ifdef USE_MYSQL4
#define DBA_USE_DELETE_USING
#define DBA_USE_TRANSACTIONS
#endif


dba_err dba_db_context_create(dba_db db, dba_db_context* ins)
{
	const char* select_query =
		"SELECT id FROM context WHERE id_ana=? AND id_report=? AND datetime=?"
		" AND ltype=? AND l1=? AND l2=?"
		" AND ptype=? AND p1=? AND p2=?";
	const char* insert_query =
		"INSERT INTO context VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	dba_err err = DBA_OK;
	dba_db_context res = NULL;
	int r;

	if ((res = (dba_db_context)malloc(sizeof(struct _dba_db_context))) == NULL)
		return dba_error_alloc("creating a new dba_db_context");
	res->db = db;
	res->sstm = NULL;
	res->istm = NULL;

	/* Create the statement for select fixed */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sstm)));
	SQLBindParameter(res->sstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_ana), 0, 0);
	SQLBindParameter(res->sstm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_report), 0, 0);
	SQLBindParameter(res->sstm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->date), 0, &(res->date_ind));
	SQLBindParameter(res->sstm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->ltype), 0, 0);
	SQLBindParameter(res->sstm, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->l1), 0, 0);
	SQLBindParameter(res->sstm, 6, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->l2), 0, 0);
	SQLBindParameter(res->sstm, 7, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->pind), 0, 0);
	SQLBindParameter(res->sstm, 8, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->p1), 0, 0);
	SQLBindParameter(res->sstm, 9, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->p2), 0, 0);

	SQLBindCol(res->sstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->sstm, (unsigned char*)select_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sstm, "compiling query to look for context IDs");
		goto cleanup;
	}

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_ana), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_report), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->date), 0, &(res->date_ind));
	SQLBindParameter(res->istm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->ltype), 0, 0);
	SQLBindParameter(res->istm, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->l1), 0, 0);
	SQLBindParameter(res->istm, 6, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->l2), 0, 0);
	SQLBindParameter(res->istm, 7, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->pind), 0, 0);
	SQLBindParameter(res->istm, 8, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->p1), 0, 0);
	SQLBindParameter(res->istm, 9, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->p2), 0, 0);
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'context'");
		goto cleanup;
	}

	*ins = res;
	res = NULL;
	
cleanup:
	if (res != NULL)
		dba_db_context_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
};

void dba_db_context_delete(dba_db_context ins)
{
	if (ins->sstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sstm);
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	free(ins);
}

dba_err dba_db_context_get_id(dba_db_context ins, int *id)
{
	SQLHSTMT stm = ins->sstm;

	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "looking for context IDs");

	/* Get the result */
	if (SQLFetch(stm) != SQL_NO_DATA)
		*id = ins->id;
	else
		*id = -1;

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_context_get_id cursor");

	return dba_error_ok();
}

dba_err dba_db_context_insert(dba_db_context ins, int *id)
{
	int res = SQLExecute(ins->istm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->istm, "inserting new data into context");

	return dba_db_last_insert_id(ins->db, id);
}

/* vim:set ts=4 sw=4: */
