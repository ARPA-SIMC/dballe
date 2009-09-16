/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup db
 *
 * Functions used to manage a general DB-ALLe query
 */

#ifndef DBA_DB_CURSOR_H
#define DBA_DB_CURSOR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/record.h>
#include <dballe/db/querybuf.h>

/**
 * Constants used to define what values we should retrieve from a query
 */
/** Retrieve latitude and longitude */
#define DBA_DB_WANT_COORDS		(1 << 0)
/** Retrieve the mobile station identifier */
#define DBA_DB_WANT_IDENT		(1 << 1)
/** Retrieve the level information */
#define DBA_DB_WANT_LEVEL		(1 << 2)
/** Retrieve the time range information */
#define DBA_DB_WANT_TIMERANGE	(1 << 3)
/** Retrieve the date and time information */
#define DBA_DB_WANT_DATETIME	(1 << 4)
/** Retrieve the variable name */
#define DBA_DB_WANT_VAR_NAME	(1 << 5)
/** Retrieve the variable value */
#define DBA_DB_WANT_VAR_VALUE	(1 << 6)
/** Retrieve the report code */
#define DBA_DB_WANT_REPCOD		(1 << 7)
/** Retrieve the station ID */
#define DBA_DB_WANT_ANA_ID		(1 << 8)
/** Retrieve the context ID */
#define DBA_DB_WANT_CONTEXT_ID	(1 << 9)

/**
 * Constants used to define what is needed from the FROM part of the query
 */
/** Add pseudoana to the FROM part of the query */
#define DBA_DB_FROM_PA			(1 << 0)
/** Add context to the FROM part of the query */
#define DBA_DB_FROM_C			(1 << 1)
/** Add data to the FROM part of the query */
#define DBA_DB_FROM_D			(1 << 2)
/** Add repinfo to the FROM part of the query */
#define DBA_DB_FROM_RI			(1 << 3)
/** Add the pseudoana context as 'cbs' to the FROM part of the query */
#define DBA_DB_FROM_CBS			(1 << 4)
/** Add the the block variables as 'dblo' to the FROM part of the query */
#define DBA_DB_FROM_DBLO		(1 << 5)
/** Add the the station variables as 'dsta' to the FROM part of the query */
#define DBA_DB_FROM_DSTA		(1 << 6)
/** Add the the pseudoana variables as 'dana' to the FROM part of the query */
#define DBA_DB_FROM_DANA		(1 << 7)
/** Add an extra data table as 'ddf' to the FROM part of the query, to restrict
 * the query on variable values */
#define DBA_DB_FROM_DDF			(1 << 8)
/** Add an extra attr table as 'adf' to the FROM part of the query, to restrict
 * the query on variable attributes */
#define DBA_DB_FROM_ADF			(1 << 9)

/**
 * Values for query modifier flags
 */
/** When values from different reports exist on the same point, only report the
 * one from the report with the highest priority */
#define DBA_DB_MODIFIER_BEST		(1 << 0)
/** Tell the database optimizer that this is a query on a database with a big
 * pseudoana table (this serves to hint the MySQL optimizer, which would not
 * otherwise apply the correct strategy */
#define DBA_DB_MODIFIER_BIGANA		(1 << 1)
/** Remove duplicates in the results */
#define DBA_DB_MODIFIER_DISTINCT	(1 << 2)
/** Include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_ANAEXTRA	(1 << 3)
/** Do not include the extra anagraphical data in the results */
#define DBA_DB_MODIFIER_NOANAEXTRA	(1 << 4)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED	(1 << 5)
/** Start geting the results as soon as they are available, without waiting for
 * the database to finish building the result set.  As a side effect, it is
 * impossible to know in advance the number of results.  Currently, it does not
 * work with the MySQL ODBC driver */
#define DBA_DB_MODIFIER_STREAM		(1 << 6)
/** Sort by rep_cod after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT	(1 << 7)

#ifndef DBA_DB_DEFINED
#define DBA_DB_DEFINED
struct _dba_db;
/** @copydoc _dba_db */
typedef struct _dba_db* dba_db;
#endif

struct _dba_db_cursor;
/** @copydoc _dba_db_cursor */
typedef struct _dba_db_cursor* dba_db_cursor;

/**
 * Create a new dba_cursor
 *
 * @param db
 *   Database that will be queried
 * @retval cur
 *   The newly created cursor.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_cursor_create(dba_db db, dba_db_cursor* cur);

/**
 * Delete a dba_db_cursor
 *
 * @param cur
 *   The cursor to delete
 */
void dba_db_cursor_delete(dba_db_cursor cur);

/**
 * Create and execute a database query.
 *
 * The results are retrieved by iterating the cursor.
 *
 * @param cur
 *   The dballe cursor to use for the query
 * @param query
 *   The record with the query data (see technical specifications, par. 1.6.4
 *   "parameter output/input"
 * @param wanted
 *   The values wanted in output
 * @param modifiers
 *   Optional modifiers to ask for special query behaviours
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_db_cursor_query(dba_db_cursor cur, dba_record query, unsigned int wanted, unsigned int modifiers);

/**
 * Get the number of rows still to be fetched
 *
 * @param cur
 *   The dballe cursor to query.
 * @return
 *   The number of rows still to be queried.  The value is undefined if no
 *   query has been successfully peformed yet using this cursor.
 */
int dba_db_cursor_remaining(dba_db_cursor cur);

/**
 * Get a new item from the results of a query
 *
 * @param cur
 *   The cursor to use to iterate the results
 * @retval has_data
 *   True if a new record has been read, false if there is no more data to read
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_cursor_next(dba_db_cursor cur, int* has_data);

/**
 * Fill in a record with the contents of a dba_db_cursor
 *
 * @param cur
 *   The cursor to use to iterate the results
 * @param rec
 *   The record where to store the values
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_cursor_to_record(dba_db_cursor cur, dba_record rec);


#if 0
/**
 * Get a new item from the results of an anagraphic query
 *
 * @param cur
 *   The cursor returned by dba_ana_query
 * @param rec
 *   The record where to store the values
 * @param is_last
 *   Variable that will be set to true if the element returned is the last one
 *   in the sequence, else to false.
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_ana_cursor_next(dba_db_cursor cur, dba_record rec, int* is_last);

/**
 * Get a new item from the results of a query
 *
 * @param cur
 *   The cursor returned by dba_query
 * @param rec
 *   The record where to store the values
 * @retval var
 *   The variable read by this fetch
 * @retval context_id
 *   The context id for this data
 * @retval is_last
 *   Variable that will be set to true if the element returned is the last one
 *   in the sequence, else to false.
 * @return
 *   The error indicator for the function.  The error code DBA_ERR_NOTFOUND is
 *   used when there are no more results to get.
 *
 * @note
 *   Do not forget to call dba_db_cursor_delete after you have finished retrieving
 *   the query data.
 */
dba_err dba_db_cursor_next(dba_db_cursor cur, dba_record rec, dba_varcode* var, int* context_id, int* is_last);
#endif

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
