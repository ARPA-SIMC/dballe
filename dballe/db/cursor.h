/*
 * db/cursor - manage select queries
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

/** @file
 * @ingroup db
 *
 * Functions used to manage a general DB-ALLe query
 */

#ifndef DBA_DB_CURSOR_H
#define DBA_DB_CURSOR_H

#include <dballe/db/odbcworkarounds.h>
#include <dballe/db/querybuf.h>
#include <dballe/core/record.h>
#include <sqltypes.h>

namespace dballe {
struct DB;
struct Record;

namespace db {
struct Statement;

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

/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
struct Cursor
{
    /** Database to operate on */
    DB& db;
    /** ODBC statement to use for the query */
    db::Statement* stm;

    /** Dynamically generated SQL query */
    Querybuf sql_query;

    /** WHERE subquery */
    Querybuf sql_where;

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

    /// true if we have already appended the "ORDER BY" clause to the query
    bool has_orderby;

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
    DBALLE_SQL_C_SINT_TYPE count;

    Cursor(DB& db);
    ~Cursor();

    /**
     * Create and execute a database query.
     *
     * The results are retrieved by iterating the cursor.
     *
     * @param query
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input"
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    void query(const Record& query, unsigned int wanted, unsigned int modifiers);

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    int remaining() const;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    bool next();

    /**
     * Fill in a record with the contents of a dba_db_cursor
     *
     * @param rec
     *   The record where to store the values
     */
    void to_record(Record& rec);

protected:
    /// Reset the cursor at the beginning of a query
    void reset();

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec);

    /// Initialise query modifiers from the 'query' parameter in \a rec
    void init_modifiers(const Record& rec);

    /**
     * Add one or more fields to the ORDER BY part of sql_query.
     */
    void add_to_orderby(const char* fields);

    /**
     * Add extra JOIN clauses to sql_query according to what is wanted.
     *
     * @param base
     *   The first table mentioned in the query, to which the other tables are
     *   joined
     */
    void add_other_froms(unsigned int base);

    /// Resolve table/field dependencies adding the missing bits to from_wanted
    void resolve_dependencies();

    /// Prepare SELECT Part and see what needs to be available in the FROM part
    void make_select();

    /**
     * Return the number of results for a query.
     *
     * This is the same as Cursor::query, but it does a SELECT COUNT(*) only.
     *
     * @warning: do not use it except to get an approximate row count:
     * insert/delete/update queries run between the count and the select will
     * change the size of the result set.
     */
    int getcount(const Record& query, unsigned int wanted, unsigned int modifiers);

    /// Add an int field to the WHERE part of the query, binding it as an input parameter
    void add_int(const Record& rec, DBALLE_SQL_C_SINT_TYPE& in, dba_keyword key, const char* sql, int needed_from);

    /// Build the WHERE part of the query, and bind the input parameters
    void make_where(const Record& rec);

    /// Add repinfo-related WHERE clauses on column \a colname to \a buf from \a query
    void add_repinfo_where(Querybuf& buf, const Record& query, const char* colname);
};

#if 0


#endif


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

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
