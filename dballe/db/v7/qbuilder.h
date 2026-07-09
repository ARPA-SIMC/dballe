#ifndef DBA_DB_V7_QBUILDER_H
#define DBA_DB_V7_QBUILDER_H

#include <dballe/core/query.h>
#include <dballe/db/v7/db.h>
#include <dballe/sql/querybuf.h>
#include <regex.h>

namespace dballe {
struct Varmatch;

namespace db {
namespace v7 {

/// Build SQL queries for V7 databases
struct QueryBuilder
{
    dballe::sql::Connection& conn;

    /** Database to operate on */
    std::shared_ptr<v7::Transaction> tr;

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
    dballe::sql::Querybuf sql_query;

    /// FROM part of the SQL query
    dballe::sql::Querybuf sql_from;

    /// WHERE part of the SQL query
    dballe::sql::Querybuf sql_where;

    /// Modifier flags to enable special query behaviours
    const unsigned int modifiers;

    /// True if we are querying station information, rather than measured data
    bool query_station_vars;

    QueryBuilder(std::shared_ptr<v7::Transaction> tr, const core::Query& query,
                 unsigned int modifiers, bool query_station_vars);
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

    virtual void build_select()   = 0;
    virtual bool build_where()    = 0;
    virtual void build_order_by() = 0;
};

struct StationQueryBuilder : public QueryBuilder
{
    StationQueryBuilder(std::shared_ptr<v7::Transaction> tr,
                        const core::Query& query, unsigned int modifiers)
        : QueryBuilder(tr, query, modifiers, false)
    {
    }

    void build_select() override;
    bool build_where() override;
    void build_order_by() override;
};

struct DataQueryBuilder : public QueryBuilder
{
    /// Attribute filter, if requested
    Varmatch* attr_filter = nullptr;

    /// True if we also query attributes of data
    bool query_attrs;

    /// True if the select includes the attrs field
    bool select_attrs = false;

    DataQueryBuilder(std::shared_ptr<v7::Transaction> tr,
                     const core::Query& query, unsigned int modifiers,
                     bool query_station_vars);
    ~DataQueryBuilder();

    // bool add_attrfilter_where(const char* tbl);

    /// Match the attributes of var against attr_filter
    bool match_attrs(const wreport::Var& var) const;

    void build_select() override;
    bool build_where() override;
    void build_order_by() override;
};

struct IdQueryBuilder : public DataQueryBuilder
{
    IdQueryBuilder(std::shared_ptr<v7::Transaction> tr,
                   const core::Query& query, unsigned int modifiers,
                   bool query_station_vars)
        : DataQueryBuilder(tr, query, modifiers, query_station_vars)
    {
    }

    void build_select() override;
    void build_order_by() override;
};

struct SummaryQueryBuilder : public DataQueryBuilder
{
    SummaryQueryBuilder(std::shared_ptr<v7::Transaction> tr,
                        const core::Query& query, unsigned int modifiers,
                        bool query_station_vars)
        : DataQueryBuilder(tr, query, modifiers, query_station_vars)
    {
    }

    void build_select() override;
    void build_order_by() override;
};

} // namespace v7
} // namespace db
} // namespace dballe

#endif
