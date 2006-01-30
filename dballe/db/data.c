#define _GNU_SOURCE
#include <dballe/db/data.h>
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


dba_err dba_db_data_create(dba_db db, dba_db_data* ins)
{
	const char* select_query =
		"SELECT id FROM data WHERE id_context=? AND id_var=?";
	const char* insert_query =
		"INSERT INTO data (id_context, id_var, value) VALUES(?, ?, ?)";
	const char* replace_query =
		"UPDATE data SET value=? WHERE id=?";
	dba_err err = DBA_OK;
	dba_db_data res = NULL;
	int r;

	if ((res = (dba_db_data)malloc(sizeof(struct _dba_db_data))) == NULL)
		return dba_error_alloc("creating a new dba_db_data");
	res->db = db;
	res->sstm = NULL;
	res->istm = NULL;
	res->ustm = NULL;

	/* Create the statement for select */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sstm)));
	SQLBindParameter(res->sstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
	SQLBindParameter(res->sstm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &(res->id_var), 0, 0);
	SQLBindCol(res->sstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->sstm, (unsigned char*)select_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sstm, "compiling query to look for data IDs");
		goto cleanup;
	}

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id_context), 0, 0);
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
	SQLBindParameter(res->ustm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &(res->value), 0, &(res->value_ind));
	SQLBindParameter(res->ustm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id), 0, 0);
	r = SQLPrepare(res->ustm, (unsigned char*)replace_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->ustm, "compiling query to update in 'data'");
		goto cleanup;
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
	if (ins->sstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sstm);
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

dba_err dba_db_data_get_id(dba_db_data ins, int *id)
{
	SQLHSTMT stm = ins->sstm;

	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "looking for data IDs");

	/* Get the result */
	if (SQLFetch(stm) != SQL_NO_DATA)
		*id = ins->id;
	else
		*id = -1;

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_data_get_id cursor");

	return dba_error_ok();
}

dba_err dba_db_data_insert(dba_db_data ins, int rewrite, int* id)
{
	*id = -1;

	if (rewrite)
		DBA_RUN_OR_RETURN(dba_db_data_get_id(ins, id));

	if (*id == -1)
	{
		SQLHSTMT stm = ins->istm;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting data into data");
		return dba_db_last_insert_id(ins->db, id);
	} else if (rewrite) {
		SQLHSTMT stm = ins->ustm;
		int res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "updating data into data");
		return dba_error_ok();
	} else
		return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
