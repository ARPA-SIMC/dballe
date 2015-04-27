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

#include <dballe/db/querybuf.h>
#include <dballe/db/sql.h>
#include <dballe/db/v6/db.h>
#include <dballe/db/v6/cursor.h>
#include <dballe/core/record.h>
#include <regex.h>

namespace dballe {
namespace db {
namespace v6 {

struct QueryBuilder
{
    Connection& conn;

    /** Database to operate on */
    DB& db;

    /**
     * If defined, it need to point to the identifier to be used as the only
     * bound input parameter.
     *
     * If not defined, there are no bound input parameters in this query
     */
    const char* bind_in_ident = nullptr;

    bool select_station = false; // ana_id, lat, lon, ident

    bool select_varinfo = false; // rep_cod, id_ltr, varcode

    // IdQuery
    bool select_data_id = false; // id_data

    // DataQuery
    bool select_data = false; // datetime, value

    // SummaryQuery
    bool select_summary_details = false; // id_data, datetime, datetimemax

    /// Query object
    const Query& query;

    /** Dynamically generated SQL query */
    Querybuf sql_query;

    /// FROM part of the SQL query
    Querybuf sql_from;

    /// WHERE part of the SQL query
    Querybuf sql_where;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    /// True if we are querying station information, rather than measured data
    bool query_station_vars;


    QueryBuilder(DB& db, const Query& query, unsigned int modifiers);
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
    StationQueryBuilder(DB& db, const Query& query, unsigned int modifiers)
        : QueryBuilder(db, query, modifiers) {}

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct DataQueryBuilder : public QueryBuilder
{
    int query_data_id;

    DataQueryBuilder(DB& db, const Query& query, unsigned int modifiers);

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct IdQueryBuilder : public DataQueryBuilder
{
    IdQueryBuilder(DB& db, const Query& query, unsigned int modifiers)
        : DataQueryBuilder(db, query, modifiers) {}

    virtual void build_select();
    virtual void build_order_by();
};

struct SummaryQueryBuilder : public DataQueryBuilder
{
    SummaryQueryBuilder(DB& db, const Query& query, unsigned int modifiers)
        : DataQueryBuilder(db, query, modifiers) {}

    virtual void build_select();
    virtual void build_order_by();
};

}
}
}

#endif
