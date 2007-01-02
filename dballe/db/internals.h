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

#include <sqltypes.h>


/* #define TRACE_DB */

#ifdef TRACE_DB
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif


struct _dba_db_repinfo;
struct _dba_db_pseudoana;
struct _dba_db_context;
struct _dba_db_data;
struct _dba_db_attr;

/**
 * Supported SQL servers.
 */
enum dba_db_server_type
{
	MYSQL,
	SQLITE,
};

/**
 * DB-ALLe session structure
 */
struct _dba_db
{
	/** ODBC database connection */
	SQLHDBC	od_conn;
	/** True if the connection is open */
	int connected;
	/** Type of SQL server we are connected to */
	enum dba_db_server_type server_type;

	/**
	 * Accessors for the various parts of the database.
	 *
	 * @warning Before using these 5 pointers, ensure they are initialised
	 * using one of the dba_db_need_* functions
	 * @{
	 */
	/** Report information */
	struct _dba_db_repinfo* repinfo;
	/** Pseudoana station information */
	struct _dba_db_pseudoana* pseudoana;
	/** Variable context */
	struct _dba_db_context* context;
	/** Variable values */
	struct _dba_db_data* data;
	/** Variable attributes */
	struct _dba_db_attr* attr;
	/** @} */

	/** Precompiled BEGIN SQL statement */
	SQLHSTMT stm_begin;
	/** Precompiled COMMIT SQL statement */
	SQLHSTMT stm_commit;
	/** Precompiled ROLLBACK SQL statement */
	SQLHSTMT stm_rollback;
	/** Precompiled LAST_INSERT_ID (or equivalent) SQL statement */
	SQLHSTMT stm_last_insert_id;
	/** ID of the last autogenerated primary key */
	int last_insert_id;
};
#ifndef DBA_DB_DEFINED
#define DBA_DB_DEFINED
/** @copydoc _dba_db */
typedef struct _dba_db* dba_db;
#endif

/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
struct _dba_db_cursor
{
	/** Database to operate on */
	dba_db db;
	/** ODBC statement to use for the query */
	SQLHSTMT stm;

	/** Dynamically generated SQL query */
	dba_querybuf query;

	/** WHERE subquery */
	dba_querybuf where;

	/** What values are wanted from the query */
	unsigned int wanted;

	/** Modifier flags to enable special query behaviours */
	unsigned int modifiers;

	/** What is needed from the FROM part of the query */
	unsigned int from_wanted;

	/** Sequence number to use to bind ODBC input parameters */
	unsigned int input_seq;

	/** Sequence number to use to bind ODBC output parameters */
	unsigned int output_seq;

	/** True if we also accept results from the anagraphical context */
	int accept_from_ana_context;

	/** Selection parameters (input) for the query
	 * @{
	 */
 	char	sel_dtmin[25];
 	char	sel_dtmax[25];
	int		sel_latmin;
	int		sel_latmax;
	int		sel_lonmin;
	int		sel_lonmax;
	char	sel_ident[64];
	int		sel_ltype;
	int		sel_l1;
	int		sel_l2;
	int		sel_pind;
	int		sel_p1;
	int		sel_p2;
	int		sel_b;
	int		sel_rep_cod;
	int		sel_priority;
	int		sel_priomin;
	int		sel_priomax;
	int		sel_ana_id;
	int		sel_context_id;
	int		sel_block;
	int		sel_station;
	/** @} */

	/** Query results
	 * @{
	 */
	int		out_lat;
	int		out_lon;
	char	out_ident[64];		SQLINTEGER out_ident_ind;
	int		out_ltype;
	int		out_l1;
	int		out_l2;
	int		out_pind;
	int		out_p1;
	int		out_p2;
	int		out_idvar;
	char	out_datetime[25];
	char	out_value[255];
	int		out_rep_cod;
	int		out_ana_id;
	int		out_context_id;
	int		out_priority;
	/** @} */

	/** Number of results still to be fetched */
	int count;
};

/**
 * Report an ODBC error, using informations from the ODBC diagnostic record
 */
dba_err dba_db_error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...);

/**
 * Report an ODBC error, using informations from the ODBC diagnostic record,
 * and ignoring one kind of SQL error.
 *
 * For an (incomplete) list of SQL error codes, see
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/odbc/htm/odbcodbc_error_codes.asp
 */
dba_err dba_db_error_odbc_except(const char* error_to_ignore, SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...);

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
 * Ensure that db->repinfo is initialized
 */
dba_err dba_db_need_repinfo(dba_db db);

/**
 * Ensure that db->pseudoana is initialized
 */
dba_err dba_db_need_pseudoana(dba_db db);

/**
 * Ensure that db->context is initialized
 */
dba_err dba_db_need_context(dba_db db);

/**
 * Ensure that db->data is initialized
 */
dba_err dba_db_need_data(dba_db db);

/**
 * Ensure that db->attr is initialized
 */
dba_err dba_db_need_attr(dba_db db);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
