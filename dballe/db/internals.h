#ifndef DBALLE_DB_INTERNALS_H
#define DBALLE_DB_INTERNALS_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 *
 * Database functions and data structures used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/querybuf.h>
#include <dballe/core/dba_record.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>


/* #define TRACE_DB */

#ifdef TRACE_DB
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif


/**
 * DB-ALLe session structure
 */
struct _dba_db
{
	SQLHDBC	od_conn;
	/*
	 * This is very conservative:
	 * The query size plus 30 possible select values, maximum of 30 characters each
	 * plus 400 characters for the various combinations of the two min and max datetimes,
	 * plus 255 for the blist
	 */
	dba_querybuf querybuf;
 	char sel_dtmin[25];
 	char sel_dtmax[25];
 	char sel_dtlike[25];
	int sel_latmin;
	int sel_lonmin;
	int sel_latmax;
	int sel_lonmax;
	const char* sel_ident;
	int sel_pindicator;
	int sel_p1;
	int sel_p2;
	int sel_leveltype;
	int sel_l1;
	int sel_l2;
	int sel_b;
	int sel_rep_cod;
	const char* sel_rep_memo;
	int sel_priority;
	int sel_priomin;
	int sel_priomax;
	int sel_ana_id;
	int sel_data_id;
	int sel_block;
	int sel_station;
};
#ifndef DBALLE_DB_H
typedef struct _dba_db* dba_db;
#endif

/**
 * Report an ODBC error, using informations from the ODBC diagnostic record
 */
dba_err dba_db_error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...);

/**
 * Create a new ODBC statement handle
 */
dba_err dba_db_statement_create(dba_db db, SQLHSTMT* stm);
		
/**
 * Report the last ID auto-generated by an insert
 */
dba_err dba_db_last_insert_id(dba_db db, int* id);

/**
 * Begin a transaction
 */
dba_err dba_db_begin(dba_db db);

/**
 * Commit a transaction
 */
dba_err dba_db_commit(dba_db db);

/**
 * Rollback a transaction
 */
void dba_db_rollback(dba_db db);

/**
 * Get the report id from this record.
 *
 * If rep_memo is specified instead, the corresponding report id is queried in
 * the database and set as "rep_cod" in the record.
 */
dba_err dba_db_get_rep_cod(dba_db db, dba_record rec, int* id);


/**
 * Add select WHERE parameters from the data in query
 */
dba_err dba_db_prepare_select(dba_db db, dba_record query, SQLHSTMT stm, int* pseq);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
