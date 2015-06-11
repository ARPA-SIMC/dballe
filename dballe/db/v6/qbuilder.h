#ifndef DBA_DB_V6_QBUILDER_H
#define DBA_DB_V6_QBUILDER_H

#include <dballe/db/querybuf.h>
#include <dballe/db/sql.h>
#include <dballe/db/v6/db.h>
#include <dballe/db/v6/cursor.h>
#include <regex.h>

namespace dballe {
namespace db {
namespace v6 {

/// Build SQL queries for V6 databases
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
    const core::Query& query;

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


    QueryBuilder(DB& db, const core::Query& query, unsigned int modifiers, bool query_station_vars);
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
    StationQueryBuilder(DB& db, const core::Query& query, unsigned int modifiers)
        : QueryBuilder(db, query, modifiers, false) {}

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct DataQueryBuilder : public QueryBuilder
{
    int query_data_id;

    DataQueryBuilder(DB& db, const core::Query& query, unsigned int modifiers, bool query_station_vars);

    virtual void build_select();
    virtual bool build_where();
    virtual void build_order_by();
};

struct IdQueryBuilder : public DataQueryBuilder
{
    IdQueryBuilder(DB& db, const core::Query& query, unsigned int modifiers, bool query_station_vars)
        : DataQueryBuilder(db, query, modifiers, query_station_vars) {}

    virtual void build_select();
    virtual void build_order_by();
};

struct SummaryQueryBuilder : public DataQueryBuilder
{
    SummaryQueryBuilder(DB& db, const core::Query& query, unsigned int modifiers, bool query_station_vars)
        : DataQueryBuilder(db, query, modifiers, query_station_vars) {}

    virtual void build_select();
    virtual void build_order_by();
};

}
}
}

#endif
