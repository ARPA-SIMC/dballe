#define _GNU_SOURCE
#include <dballe/db/attr.h>
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


dba_err dba_db_attr_create(dba_db db, dba_db_attr* ins)
{
	const char* insert_query =
		"INSERT INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?)";
	const char* replace_query =
		"INSERT INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?) ON DUPLICATE KEY UPDATE value=VALUES(value)";
	dba_err err = DBA_OK;
	dba_db_attr res = NULL;
	int r;

	if ((res = (dba_db_attr)malloc(sizeof(struct _dba_db_attr))) == NULL)
		return dba_error_alloc("creating a new dba_db_attr");
	res->db = db;
	res->istm = NULL;
	res->rstm = NULL;

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_data), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->type), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'attr'");
		goto cleanup;
	}

	/* Create the statement for replace */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->rstm)));
	SQLBindParameter(res->rstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_data), 0, 0);
	SQLBindParameter(res->rstm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->type), 0, 0);
	SQLBindParameter(res->rstm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	r = SQLPrepare(res->rstm, (unsigned char*)replace_query, SQL_NTS);
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
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	if (ins->rstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->rstm);
	free(ins);
}

dba_err dba_db_attr_insert(dba_db_attr ins, int replace)
{
	SQLHSTMT stm = replace ? ins->rstm : ins->istm;
	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting data into attr");
	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
