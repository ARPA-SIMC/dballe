/*
 * db/internals - Internal support infrastructure for the DB
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup db
 *
 * Database functions and data structures used by the db module, but not
 * exported as official API.
 */

#include <dballe/db/querybuf.h>
#include <dballe/db/odbcworkarounds.h>
#include <wreport/error.h>

#include <sqltypes.h>

/*
 * Define to true to enable the use of transactions during writes
 */
#define DBA_USE_TRANSACTIONS

/* Define this to enable referential integrity */
#undef USE_REF_INT

namespace dballe {
namespace db {

/** Trace macros internally used for debugging
 * @{
 */

/* #define TRACE_DB */

#ifdef TRACE_DB
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
/** Ouput a trace message */
#define TRACE(...) do { } while (0)
/** Prefix a block of code to compile only if trace is enabled */
#define IFTRACE if (0)
#endif

/** @} */

/**
 * Report an ODBC error, using informations from the ODBC diagnostic record
 */
struct error_odbc : public wreport::error 
{
	std::string msg;

	/**
	 * Copy informations from the ODBC diagnostic record to the dba error
	 * report
	 */
	error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& msg);
	~error_odbc() throw () {}

	wreport::ErrorCode code() const throw () { return wreport::WR_ERR_ODBC; }

	virtual const char* what() const throw () { return msg.c_str(); }

	static void throwf(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...) WREPORT_THROWF_ATTRS(3, 4);
};

/**
 * Supported SQL servers.
 */
enum ServerType
{
	MYSQL,
	SQLITE,
	ORACLE,
	POSTGRES,
};

/// ODBC environment
struct Environment
{
	SQLHENV od_env;

	Environment();
	~Environment();

	static Environment& get();
};

/// Database connection
struct Connection
{
	/** ODBC database connection */
	SQLHDBC	od_conn;
	/** True if the connection is open */
	bool connected;
	/** Type of SQL server we are connected to */
	enum ServerType server_type;

	Connection();
	~Connection();

	void connect(const char* dsn, const char* user, const char* password);
	void driver_connect(const char* config);
	std::string driver_name();
	void set_autocommit(bool val);

	/// Commit a transaction
	void commit();

	/// Rollback a transaction
	void rollback();

protected:
	void init_after_connect();
};

/// ODBC statement
struct Statement
{
	SQLHSTMT stm;
	/// If non-NULL, ignore all errors with this code
	const char* ignore_error;

	Statement(Connection& conn);
	~Statement();

	void bind_in(int idx, const DBALLE_SQL_C_SINT_TYPE& val);
	void bind_in(int idx, const DBALLE_SQL_C_UINT_TYPE& val);
	void bind_in(int idx, const char* val);

	void bind_out(int idx, DBALLE_SQL_C_SINT_TYPE& val, SQLLEN* ind = 0);
	void bind_out(int idx, DBALLE_SQL_C_UINT_TYPE& val, SQLLEN* ind = 0);
	void bind_out(int idx, char* val, SQLLEN buflen, SQLLEN* ind = 0);

	void prepare(const char* query);
	void prepare(const char* query, int qlen);

	void exec_direct(const char* query);
	void exec_direct(const char* query, int qlen);

	void execute();
	bool fetch();
	void close_cursor();

protected:
	bool error_is_ignored();
	bool is_error(int sqlres);
};

/// ODBC statement to read a sequence
struct Sequence : public Statement
{
	DBALLE_SQL_C_SINT_TYPE out;

	Sequence(Connection& conn, const char* name);
	~Sequence();

	/// Read the current value of the sequence
	const DBALLE_SQL_C_SINT_TYPE& read();
};

/// Return the default repinfo file pathname
const char* default_repinfo_file();

#if 0

struct _dba_db_repinfo;
struct _dba_db_pseudoana;
struct _dba_db_context;
struct _dba_db_data;
struct _dba_db_attr;
struct _dba_db_seq;

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

	/** What is needed from the SELECT part of the query */
	unsigned int select_wanted;

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
 	SQL_TIMESTAMP_STRUCT	sel_dtmin;
 	SQL_TIMESTAMP_STRUCT	sel_dtmax;
	DBALLE_SQL_C_SINT_TYPE	sel_latmin;
	DBALLE_SQL_C_SINT_TYPE	sel_latmax;
	DBALLE_SQL_C_SINT_TYPE	sel_lonmin;
	DBALLE_SQL_C_SINT_TYPE	sel_lonmax;
	char	sel_ident[64];
	DBALLE_SQL_C_SINT_TYPE	sel_ltype1;
	DBALLE_SQL_C_SINT_TYPE	sel_l1;
	DBALLE_SQL_C_SINT_TYPE	sel_ltype2;
	DBALLE_SQL_C_SINT_TYPE	sel_l2;
	DBALLE_SQL_C_SINT_TYPE	sel_pind;
	DBALLE_SQL_C_SINT_TYPE	sel_p1;
	DBALLE_SQL_C_SINT_TYPE	sel_p2;
	DBALLE_SQL_C_SINT_TYPE	sel_b;
	DBALLE_SQL_C_SINT_TYPE	sel_rep_cod;
	DBALLE_SQL_C_SINT_TYPE	sel_ana_id;
	DBALLE_SQL_C_SINT_TYPE	sel_context_id;
	/** @} */

	/** Query results
	 * @{
	 */
	DBALLE_SQL_C_SINT_TYPE	out_lat;
	DBALLE_SQL_C_SINT_TYPE	out_lon;
	char	out_ident[64];		SQLLEN out_ident_ind;
	DBALLE_SQL_C_SINT_TYPE	out_ltype1;
	DBALLE_SQL_C_SINT_TYPE	out_l1;
	DBALLE_SQL_C_SINT_TYPE	out_ltype2;
	DBALLE_SQL_C_SINT_TYPE	out_l2;
	DBALLE_SQL_C_SINT_TYPE	out_pind;
	DBALLE_SQL_C_SINT_TYPE	out_p1;
	DBALLE_SQL_C_SINT_TYPE	out_p2;
	DBALLE_SQL_C_SINT_TYPE	out_idvar;
	SQL_TIMESTAMP_STRUCT	out_datetime;
	char	out_value[255];
	DBALLE_SQL_C_SINT_TYPE	out_rep_cod;
	DBALLE_SQL_C_SINT_TYPE	out_ana_id;
	DBALLE_SQL_C_SINT_TYPE	out_context_id;
	DBALLE_SQL_C_SINT_TYPE	out_priority;
	/** @} */

	/** Number of results still to be fetched */
	int count;
};

/**
 * Report an ODBC error, using informations from the ODBC diagnostic record,
 * and ignoring one kind of SQL error.
 *
 * For an (incomplete) list of SQL error codes, see
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/odbc/htm/odbcodbc_error_codes.asp
 */
dba_err dba_db_error_odbc_except(const char* error_to_ignore, SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...);

/**
 * Report the last ID auto-generated by an insert
 */
dba_err dba_db_last_insert_id(dba_db db, int* id);

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

#endif

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
