/*
 * db/v6/qbuilder - build SQL queries for V6 databases
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_DB_V6_QBUILDER_H
#define DBA_DB_V6_QBUILDER_H

#include "dballe/db/querybuf.h"
#include "dballe/db/internals.h"
#include "dballe/db/db.h"
#include "dballe/core/record.h"
#include "db.h"
#include "cursor.h"


/**
 * Constants used to define what is needed from the FROM part of the query
 */
/** Add pseudoana to the FROM part of the query */
#define DBA_DB_FROM_PA          (1 << 0)
/** Add lev_tr to the FROM part of the query */
#define DBA_DB_FROM_LTR         (1 << 1)
/** Add data to the FROM part of the query */
#define DBA_DB_FROM_D           (1 << 2)
/** Add repinfo to the FROM part of the query */
#define DBA_DB_FROM_RI          (1 << 3)
/** Add the the block variables as 'dblo' to the FROM part of the query */
#define DBA_DB_FROM_DBLO        (1 << 5)
/** Add the the station variables as 'dsta' to the FROM part of the query */
#define DBA_DB_FROM_DSTA        (1 << 6)
/** Add the the pseudoana variables as 'dana' to the FROM part of the query */
#define DBA_DB_FROM_DANA        (1 << 7)
/** Add an extra data table as 'ddf' to the FROM part of the query, to restrict
 * the query on variable values */
#define DBA_DB_FROM_DDF         (1 << 8)
/** Add an extra attr table as 'adf' to the FROM part of the query, to restrict
 * the query on variable attributes */
#define DBA_DB_FROM_ADF         (1 << 9)

namespace dballe {
namespace db {
namespace v6 {

struct QueryBuilder
{
    /** Database to operate on */
    DB& db;

    /** Statement to build variables to */
    Statement& stm;

    /** Cursor with the output variables */
    Cursor& cur;

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
    bool query_station_vars;

    /// true if we have already appended the "ORDER BY" clause to the query
    bool has_orderby;

    /** Selection parameters (input) for the query
     * @{
     */
    SQL_TIMESTAMP_STRUCT    sel_dtmin;
    SQL_TIMESTAMP_STRUCT    sel_dtmax;
    char    sel_ident[64];
    /** @} */

    QueryBuilder(DB& db, Statement& stm, Cursor& cur, int wanted, int modifiers)
        : db(db), stm(stm), cur(cur), sql_query(2048), sql_where(1024),
          wanted(wanted), modifiers(modifiers),
          select_wanted(0), from_wanted(0), input_seq(1), output_seq(1),
          query_station_vars(false), has_orderby(false) {}

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
    void add_other_froms(unsigned int base, const Record& rec);

    /**
     * Add an extra data table to the join, set to act as a filter on the
     * station level
     */
    void add_station_filter(Querybuf& q, const char* name, const char* extra_query=NULL);

    /// Resolve table/field dependencies adding the missing bits to from_wanted
    void resolve_dependencies();

    /// Prepare SELECT Part and see what needs to be available in the FROM part
    void make_select();

    /**
     * Prepare the extra bits of the SELECT part that need information after
     * dependency computation
     */
    void make_extra_select();

    /// Build the FROM and WHERE parts of the query
    void make_from(const Record& rec);

    /// Add an int field to the WHERE part of the query, binding it as an input parameter
    void add_int(const Record& rec, dba_keyword key, const char* sql, int needed_from);

    /// Build the WHERE part of the query, and bind the input parameters
    void make_where(const Record& rec);

    /// Add repinfo-related WHERE clauses on column \a colname to \a buf from \a query
    void add_repinfo_where(Querybuf& buf, const Record& query, const char* colname);

    /// Build the big data query
    void build_query(const Record& rec);

    /// Build the query with just SELECT COUNT(*)
    void build_count_query(const Record& rec);

    /// Build the query with just a select for date extremes
    void build_date_extremes_query(const Record& rec);
};

}
}
}

#endif
