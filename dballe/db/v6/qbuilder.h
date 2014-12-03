/*
 * db/v6/qbuilder - build SQL queries for V6 databases
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/db/odbc/internals.h"
#include "dballe/db/db.h"
#include "dballe/core/record.h"
#include "db.h"
#include "cursor.h"
#include <regex.h>

#if 0
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
#endif

namespace dballe {
namespace db {
namespace v6 {

/**
 * Copies of bind values that cannot be bound to data inside the query
 * Record
 */
struct ExtraQueryArgs
{
    /// Positional sequence number to use to bind ODBC input parameters
    unsigned int input_seq;

    ExtraQueryArgs() : input_seq(1) {};
};

struct QueryBuilder
{
    Connection& conn;

    /** Database to operate on */
    DB& db;

    /** Statement to bind variables to */
    ODBCStatement& stm;

    /** Cursor with the output variables */
    Cursor& cur;

    /// Record with the query
    const Record& rec;

    /** Dynamically generated SQL query */
    Querybuf sql_query;

    /// FROM part of the SQL query
    Querybuf sql_from;

    /// WHERE part of the SQL query
    Querybuf sql_where;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    ExtraQueryArgs qargs;

    /// Sequence number to use to bind ODBC output parameters
    unsigned int output_seq;

    /// True if we are querying station information, rather than measured data
    bool query_station_vars;


    QueryBuilder(DB& db, ODBCStatement& stm, Cursor& cur, const Record& rec, unsigned int modifiers);
    virtual ~QueryBuilder() {}

    void build();

protected:
    // Add WHERE conditions
    bool add_pa_where(const char* tbl);
    bool add_dt_where(const char* tbl);
    bool add_ltr_where(const char* tbl);
    bool add_varcode_where(const char* tbl);
    bool add_repinfo_where(const char* tbl);
    bool add_datafilter_where(const char* tbl);
    bool add_attrfilter_where(const char* tbl);

    virtual void build_select() = 0;
    virtual bool build_where() = 0;
    virtual void build_order_by() = 0;
};

struct StationQueryBuilder : public QueryBuilder
{
    StationQueryBuilder(DB& db, ODBCStatement& stm, Cursor& cur, const Record& rec, unsigned int modifiers)
        : QueryBuilder(db, stm, cur, rec, modifiers) {}

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct DataQueryBuilder : public QueryBuilder
{
    int query_data_id;

    DataQueryBuilder(DB& db, ODBCStatement& stm, Cursor& cur, const Record& rec, unsigned int modifiers);

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct IdQueryBuilder : public DataQueryBuilder
{
    IdQueryBuilder(DB& db, ODBCStatement& stm, Cursor& cur, const Record& rec, unsigned int modifiers)
        : DataQueryBuilder(db, stm, cur, rec, modifiers) {}

    virtual void build_select();
    virtual void build_order_by();
};

struct SummaryQueryBuilder : public DataQueryBuilder
{
    CursorSummary& cur_s;

    SummaryQueryBuilder(DB& db, ODBCStatement& stm, CursorSummary& cur, const Record& rec, unsigned int modifiers)
        : DataQueryBuilder(db, stm, cur, rec, modifiers), cur_s(cur) {}

    virtual void build_select();
    virtual void build_order_by();
};

}
}
}

#endif
