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
#include <wreport/varinfo.h>
#include <sqltypes.h>
#include <vector>

namespace dballe {
struct DB;
struct Record;

namespace db {
struct Statement;

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

    /** What values are wanted from the query */
    unsigned int wanted;

    /** Modifier flags to enable special query behaviours */
    unsigned int modifiers;

    /** What is in the FROM part of the query, used to know what output fields
     * are bound */
    unsigned int from_wanted;

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
    wreport::Varcode		out_varcode;
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
     * @return
     *   The count of items in the results
     */
    int query(const Record& query, unsigned int wanted, unsigned int modifiers);

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

    /**
     * Query attributes for the current variable
     */
    unsigned query_attrs(const std::vector<wreport::Varcode>& qcs, Record& attrs);

protected:
    /// Reset the cursor at the beginning of a query
    void reset();

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec);

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
};

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
