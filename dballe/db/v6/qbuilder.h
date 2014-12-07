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
    // stm.bind_out(output_seq++, cur.sqlrec.out_ana_id);
    // stm.bind_out(output_seq++, cur.sqlrec.out_lat);
    // stm.bind_out(output_seq++, cur.sqlrec.out_lon);
    // stm.bind_out(output_seq++, cur.sqlrec.out_ident, sizeof(cur.sqlrec.out_ident), cur.sqlrec.out_ident_ind);

    bool select_varinfo = false; // rep_cod, id_ltr, varcode
    // stm.bind_out(output_seq++, cur.sqlrec.out_rep_cod);
    // stm.bind_out(output_seq++, cur.sqlrec.out_id_ltr);
    // stm.bind_out(output_seq++, cur.sqlrec.out_varcode);

    // IdQuery
    bool select_data_id = false; // id_data
    // stm.bind_out(output_seq++, cur.sqlrec.out_id_data);

    // DataQuery
    bool select_data = false; // datetime, value
    // stm.bind_out(output_seq++, cur.sqlrec.out_datetime);
    // stm.bind_out(output_seq++, cur.sqlrec.out_value, sizeof(cur.sqlrec.out_value));

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

    /// True if we are querying station information, rather than measured data
    bool query_station_vars;


    QueryBuilder(DB& db, const Record& rec, unsigned int modifiers);
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
    StationQueryBuilder(DB& db, const Record& rec, unsigned int modifiers)
        : QueryBuilder(db, rec, modifiers) {}

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct DataQueryBuilder : public QueryBuilder
{
    int query_data_id;

    DataQueryBuilder(DB& db, const Record& rec, unsigned int modifiers);

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct IdQueryBuilder : public DataQueryBuilder
{
    IdQueryBuilder(DB& db, const Record& rec, unsigned int modifiers)
        : DataQueryBuilder(db, rec, modifiers) {}

    virtual void build_select();
    virtual void build_order_by();
};

struct SummaryQueryBuilder : public DataQueryBuilder
{
    SummaryQueryBuilder(DB& db, const Record& rec, unsigned int modifiers)
        : DataQueryBuilder(db, rec, modifiers) {}

    virtual void build_select();
    virtual void build_order_by();
};

}
}
}

#endif
