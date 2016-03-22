#ifndef DBALLE_DB_V6_DRIVER_H
#define DBALLE_DB_V6_DRIVER_H

#include <dballe/core/defs.h>
#include <dballe/db/defs.h>
#include <dballe/sql/fwd.h>
#include <wreport/var.h>
#include <memory>
#include <functional>
#include <vector>
#include <cstdio>

namespace dballe {
namespace db {
namespace v6 {
struct QueryBuilder;
struct Repinfo;
struct Station;
struct LevTr;
struct DataV6;
struct AttrV6;

/// Query results from SQL output
struct SQLRecordV6
{
    int out_lat;
    int out_lon;
    char out_ident[64];
    int out_ident_size; // -1 for NULL
    wreport::Varcode out_varcode;
    Datetime out_datetime;
    Datetime out_datetimemax;
    char out_value[255];
    int out_rep_cod;
    int out_ana_id;
    int out_id_ltr;
    int out_id_data;
    int priority;

    /**
     * Checks true if ana_id, id_ltr, datetime and varcode are the same in
     * both records
     *
     * @returns true if they match, false if they are different
     */
    bool querybest_fields_are_the_same(const SQLRecordV6& r);

    /// Dump the record as a single line to the given output stream
    void dump(FILE* out);
};

struct Driver
{
public:
    virtual ~Driver();

    /// Run a SQL query that is expected to return no data
    virtual void exec_no_data(const std::string& query) = 0;

    /// Precompiled queries to manipulate the repinfo table
    virtual std::unique_ptr<v6::Repinfo> create_repinfov6() = 0;

    /// Precompiled queries to manipulate the station table
    virtual std::unique_ptr<v6::Station> create_stationv6() = 0;

    /// Precompiled queries to manipulate the levtr table
    virtual std::unique_ptr<v6::LevTr> create_levtrv6() = 0;

    /// Precompiled queries to manipulate the data table
    virtual std::unique_ptr<v6::DataV6> create_datav6() = 0;

    /// Precompiled queries to manipulate the attr table
    virtual std::unique_ptr<v6::AttrV6> create_attrv6() = 0;

    /**
     * Run a query on the given statement, returning results as SQLRecordV6 objects
     *
     * SQLRecordV6 is filled with the output variables according to which sel_* is true.
     *
     * Query will dispatch to the right connector routines for the query, based on
     * the actual implementation of stm.
     */
    virtual void run_built_query_v6(const v6::QueryBuilder& qb, std::function<void(SQLRecordV6& rec)> dest) = 0;

    /// Create all missing tables for a DB with the given format
    void create_tables(db::Format format);

    /// Create all missing tables for V6 databases
    virtual void create_tables_v6() = 0;

    /// Delete all existing tables for a DB with the given format
    void delete_tables(db::Format format);

    /// Delete all existing tables for V6 databases
    virtual void delete_tables_v6() = 0;

    /// Empty all tables for a DB with the given format
    void remove_all(db::Format format);

    /// Empty all tables for V6 databases, assuming that they exist, without touching the repinfo table
    virtual void remove_all_v6();

    /// Perform database cleanup/maintenance on v6 databases
    virtual void vacuum_v6() = 0;

    /// Outputs to stderr an explanation of the given query
    virtual void explain(const std::string& query);

    /// Create a Driver for this connection
    static std::unique_ptr<Driver> create(dballe::sql::Connection& conn);
};

}
}
}
#endif
