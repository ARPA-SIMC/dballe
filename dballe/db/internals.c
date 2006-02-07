#define _GNU_SOURCE
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

dba_err dba_db_rep_cod_from_memo(dba_db db, const char* memo, int* rep_cod)
{
	const char* query = "SELECT id FROM repinfo WHERE memo = ?";
	dba_err err = DBA_OK;
	SQLHSTMT stm;
	SQLINTEGER rep_cod_ind;
	int res;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind input parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)memo, 0, 0);

	/* Bind variable and indicator for SELECT results */
	SQLBindCol(stm, 1, SQL_C_SLONG, rep_cod, sizeof(int), &rep_cod_ind);

	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "looking for report informations");
		goto cleanup;
	}

	/* Get one result */
	if (SQLFetch(stm) == SQL_NO_DATA)
	{
		err = dba_error_notfound("looking for report informations for '%s'", memo);
		goto cleanup;
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}

/* Get the report id from this record.  If rep_memo is specified instead, the
 * corresponding report id is queried in the database and set as "rep_cod" in
 * the record. */
dba_err dba_db_get_rep_cod(dba_db db, dba_record rec, int* id)
{
	const char* rep;
	if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_COD)) != NULL)
		*id = strtol(rep, 0, 10);
	else if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_MEMO)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_db_rep_cod_from_memo(db, rep, id));
		/* DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_REP_COD, *id)); */
	}
	else
		return dba_error_notfound("looking for report type in rep_cod or rep_memo");
	return dba_error_ok();
}		

#define PARM_INT(field, key, sql) do {\
	if ((val = dba_record_key_peek_value(rec, key)) != NULL) { \
		db->sel_##field = strtol(val, 0, 10); \
		TRACE("found " #field ": adding " sql ". val is %d\n", db->sel_##field); \
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, sql)); \
		SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &db->sel_##field, 0, 0); \
	} } while (0)

static dba_err dba_prepare_select_context(dba_db db, dba_record rec, SQLHSTMT stm, int* pseq)
{
	const char* val;

	/* Bind select fields */

	/* Set the time extremes */
	{
		int minvalues[6], maxvalues[6];
		DBA_RUN_OR_RETURN(dba_record_parse_date_extremes(rec, minvalues, maxvalues));
		
		if (minvalues[0] != -1)
		{
			/* Add constraint on the minimum date interval */
			snprintf(db->sel_dtmin, 25, "%04d-%02d-%02d %02d:%02d:%02d",
					minvalues[0], minvalues[1], minvalues[2],
					minvalues[3], minvalues[4], minvalues[5]);
			DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND c.datetime >= ?"));
			TRACE("found min time interval: adding AND c.datetime >= ?.  val is %s\n", db->sel_dtmin);
			SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_dtmin, 0, 0);
		}
		
		if (maxvalues[0] != -1)
		{
			snprintf(db->sel_dtmax, 25, "%04d-%02d-%02d %02d:%02d:%02d",
					maxvalues[0], maxvalues[1], maxvalues[2],
					maxvalues[3], maxvalues[4], maxvalues[5]);
			DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND c.datetime <= ?"));
			TRACE("found max time interval: adding AND c.datetime <= ?.  val is %s\n", db->sel_dtmax);
			SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_dtmax, 0, 0);
		}
	}

	PARM_INT(ana_id, DBA_KEY_ANA_ID, " AND pa.id = ?");
	PARM_INT(latmin, DBA_KEY_LAT, " AND pa.lat = ?");
	PARM_INT(latmin, DBA_KEY_LATMIN, " AND pa.lat > ?");
	PARM_INT(latmax, DBA_KEY_LATMAX, " AND pa.lat < ?");
	PARM_INT(lonmin, DBA_KEY_LON, " AND pa.lon = ?");
	PARM_INT(lonmin, DBA_KEY_LONMIN, " AND pa.lon > ?");
	PARM_INT(lonmax, DBA_KEY_LONMAX, " AND pa.lon < ?");

	{
		const char* mobile_sel = dba_record_key_peek_value(rec, DBA_KEY_MOBILE);

		if (mobile_sel != NULL)
		{
			if (mobile_sel[0] == '0')
			{
				DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND pa.ident IS NULL"));
				TRACE("found fixed/mobile: adding AND pa.ident IS NULL.\n");
			} else {
				DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND NOT pa.ident IS NULL"));
				TRACE("found fixed/mobile: adding AND NOT pa.ident IS NULL\n");
			}
		}
	}

	if ((db->sel_ident = dba_record_key_peek_value(rec, DBA_KEY_IDENT)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND pa.ident = ?"));
		TRACE("found ident: adding AND pa.ident = ?.  val is %s\n", db->sel_ident);
		SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_ident, 0, 0);
	}

	PARM_INT(pindicator, DBA_KEY_PINDICATOR, " AND c.ptype = ?");
	PARM_INT(p1, DBA_KEY_P1, " AND c.p1 = ?");
	PARM_INT(p2, DBA_KEY_P2, " AND c.p2 = ?");
	PARM_INT(leveltype, DBA_KEY_LEVELTYPE, " AND c.ltype = ?");
	PARM_INT(l1, DBA_KEY_L1, " AND c.l1 = ?");
	PARM_INT(l2, DBA_KEY_L2, " AND c.l2 = ?");

	PARM_INT(block, DBA_KEY_BLOCK, " AND pa.block = ?");
	PARM_INT(station, DBA_KEY_STATION, " AND pa.station = ?");

	return dba_error_ok();

}

static dba_err dba_prepare_select_vars(dba_db db, dba_record rec, SQLHSTMT stm, int* pseq)
{
	const char* val;

	/* Bind select fields */

	PARM_INT(data_id, DBA_KEY_DATA_ID, " AND d.id = ?");

	if ((val = dba_record_key_peek_value(rec, DBA_KEY_VAR)) != NULL)
	{
		db->sel_b = dba_descriptor_code(val);
		TRACE("found b: adding AND d.id_var = ?. val is %d %s\n", db->sel_b, val);
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND d.id_var = ?"));
		SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &db->sel_b, 0, 0);
	}
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_VARLIST)) != NULL)
	{
		size_t pos;
		size_t len;
		TRACE("found blist: adding AND d.id_var IN (%s)\n", val);
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND d.id_var IN ("));
		for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
		{
			dba_varcode code = DBA_STRING_TO_VAR(val + pos + 1);
			if (pos == 0)
				DBA_RUN_OR_RETURN(dba_querybuf_appendf(db->querybuf, "%d", code));
			else
				DBA_RUN_OR_RETURN(dba_querybuf_appendf(db->querybuf, ",%d", code));
		}
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, ")"));
	}

	PARM_INT(rep_cod, DBA_KEY_REP_COD, " AND ri.id = ?");

	if ((db->sel_rep_memo = dba_record_key_peek_value(rec, DBA_KEY_REP_MEMO)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, " AND ri.memo = ?"));
		TRACE("found rep_memo: adding AND ri.memo = ?.  val is %s\n", db->sel_rep_memo);
		SQLBindParameter(stm, (*pseq)++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_rep_memo, 0, 0);
	}

	PARM_INT(priority, DBA_KEY_PRIORITY, " AND ri.prio = ?");
	PARM_INT(priomin, DBA_KEY_PRIOMIN, " AND ri.prio >= ?");
	PARM_INT(priomax, DBA_KEY_PRIOMAX, " AND ri.prio <= ?");

	return dba_error_ok();

}
dba_err dba_db_prepare_select(dba_db db, dba_record rec, SQLHSTMT stm, int* pseq)
{
	DBA_RUN_OR_RETURN(dba_prepare_select_context(db, rec, stm, pseq));
	DBA_RUN_OR_RETURN(dba_prepare_select_vars(db, rec, stm, pseq));

	return dba_error_ok();

}
#undef PARM_INT

/* vim:set ts=4 sw=4: */
