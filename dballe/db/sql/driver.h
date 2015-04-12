/*
 * db/sql/driver - db-independent functions
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_SQL_DRIVER_H
#define DBALLE_DB_SQL_DRIVER_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/core/defs.h>
#include <wreport/var.h>
#include <memory>
#include <functional>
#include <vector>
#include <cstdio>

namespace dballe {
namespace db {
struct Connection;

namespace v6 {
struct QueryBuilder;
}

namespace sql {
struct Repinfo;
struct Station;
struct LevTr;
struct DataV5;
struct DataV6;
struct AttrV5;
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

namespace bulk {

/**
 * Workflow information about a variable listed for bulk insert/update
 */
struct VarV6
{
    static const unsigned FLAG_NEEDS_UPDATE = 1 << 0;
    static const unsigned FLAG_UPDATED      = 1 << 1;
    static const unsigned FLAG_NEEDS_INSERT = 1 << 2;
    static const unsigned FLAG_INSERTED     = 1 << 3;
    int id_levtr;
    int id_data;
    const wreport::Var* var;
    unsigned flags = 0;

    VarV6(const wreport::Var* var, int id_levtr=-1, int id_data=-1)
        : id_levtr(id_levtr), id_data(id_data), var(var)
    {
    }
    bool operator<(const VarV6& v) const
    {
        if (int d = id_levtr - v.id_levtr) return d < 0;
        return var->code() < v.var->code();
    }

    bool needs_update() const { return flags | FLAG_NEEDS_UPDATE; }
    bool updated() const { return flags | FLAG_UPDATED; }
    bool needs_insert() const { return flags | FLAG_NEEDS_INSERT; }
    bool inserted() const { return flags | FLAG_INSERTED; }
    void set_needs_update() { flags |= FLAG_NEEDS_UPDATE; }
    void set_updated() { flags = (flags & ~FLAG_NEEDS_UPDATE) | FLAG_UPDATED; }
    void set_needs_insert() { flags |= FLAG_NEEDS_INSERT; }
    void set_inserted() { flags = (flags & ~FLAG_NEEDS_INSERT) | FLAG_INSERTED; }

    void dump(FILE* out) const;
};


/**
 * Input for a bulk insert of a lot of variables sharing the same context
 * information.
 */
struct InsertV6 : public std::vector<VarV6>
{
    int id_station;
    int id_report;
    Datetime datetime;

    void add(const wreport::Var* var, int id_levtr)
    {
        emplace_back(var, id_levtr);
    }

    void dump(FILE* out) const;
};

/**
 * Helper class for annotating InsertV6 variables with the current status of
 * the database.
 */
struct AnnotateVarsV6
{
    InsertV6& vars;
    InsertV6::iterator iter;
    bool do_insert = false;
    bool do_update = false;

    AnnotateVarsV6(InsertV6& vars) : vars(vars), iter(vars.begin()) {}

    bool annotate(int id_data, int id_levtr, wreport::Varcode code, const char* value);
    void annotate_end();

    void dump(FILE* out) const;
};

}


struct Driver
{
    virtual ~Driver();

    /// Precompiled queries to manipulate the repinfo table
    virtual std::unique_ptr<sql::Repinfo> create_repinfov5() = 0;

    /// Precompiled queries to manipulate the repinfo table
    virtual std::unique_ptr<sql::Repinfo> create_repinfov6() = 0;

    /// Precompiled queries to manipulate the station table
    virtual std::unique_ptr<sql::Station> create_stationv5() = 0;

    /// Precompiled queries to manipulate the station table
    virtual std::unique_ptr<sql::Station> create_stationv6() = 0;

    /// Precompiled queries to manipulate the levtr table
    virtual std::unique_ptr<sql::LevTr> create_levtrv6() = 0;

    /// Precompiled queries to manipulate the data table
    virtual std::unique_ptr<sql::DataV5> create_datav5() = 0;

    /// Precompiled queries to manipulate the data table
    virtual std::unique_ptr<sql::DataV6> create_datav6() = 0;

    /// Precompiled queries to manipulate the attr table
    virtual std::unique_ptr<sql::AttrV5> create_attrv5() = 0;

    /// Precompiled queries to manipulate the attr table
    virtual std::unique_ptr<sql::AttrV6> create_attrv6() = 0;

    /// Bulk variable insert
    virtual void bulk_insert_v6(bulk::InsertV6& vars, bool update_existing=true) = 0;

    /**
     * Run a query on the given statement, returning results as SQLRecordV6 objects
     *
     * SQLRecordV6 is filled with the output variables according to which sel_* is true.
     *
     * Query will dispatch to the right connector routines for the query, based on
     * the actual implementation of stm.
     */
    virtual void run_built_query_v6(const v6::QueryBuilder& qb, std::function<void(SQLRecordV6& rec)> dest) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void run_delete_query_v6(const v6::QueryBuilder& qb) = 0;

    /// Create all missing tables for V5 databases
    virtual void create_tables_v5() = 0;

    /// Create all missing tables for V6 databases
    virtual void create_tables_v6() = 0;

    /// Delete all tables for V5 databases
    virtual void delete_tables_v5() = 0;

    /// Delete all tables for V6 databases
    virtual void delete_tables_v6() = 0;

    /// Empty all tables for V5 databases, assuming that they exist, without touching the repinfo table
    //virtual void remove_all_v5() = 0;

    /// Empty all tables for V5 databases, assuming that they exist, without touching the repinfo table
    //virtual void remove_all_v6() = 0;

    /// Create a Driver for this connection
    static std::unique_ptr<Driver> create(Connection& conn);
};

}
}
}
#endif
