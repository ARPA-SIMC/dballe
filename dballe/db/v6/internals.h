/*
 * db/v6/internals - internal interfaces for v6 db implementation
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

#ifndef DBALLE_DB_V6_INTERNALS_H
#define DBALLE_DB_V6_INTERNALS_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/core/defs.h>
#include <dballe/db/sql/repinfo.h>
#include <dballe/db/sql/station.h>
#include <wreport/var.h>
#include <memory>
#include <cstdio>

namespace dballe {
struct Record;
struct Msg;

namespace msg {
struct Context;
}

namespace db {
struct Connection;
struct Statement;

namespace v6 {
struct DB;
struct QueryBuilder;

/// Precompiled queries to manipulate the repinfo table
std::unique_ptr<sql::Repinfo> create_repinfo(Connection& conn);

/// Precompiled queries to manipulate the station table
std::unique_ptr<sql::Station> create_station(Connection& conn);

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct LevTr
{
    struct DBRow
    {
        /// lev_tr ID SQL parameter
        int id;
        /// First level type SQL parameter
        int ltype1;
        /// Level L1 SQL parameter
        int l1;
        /// Second level type SQL parameter
        int ltype2;
        /// Level L2 SQL parameter
        int l2;
        /// Time range type SQL parameter
        int pind;
        /// Time range P1 SQL parameter
        int p1;
        /// Time range P2 SQL parameter
        int p2;
    };

    static std::unique_ptr<LevTr> create(DB& db);

    virtual ~LevTr();

    /**
     * Return the ID for the given Level and Trange, adding it to the database
     * if it does not already exist
     */
    virtual int obtain_id(const Level& lev, const Trange& tr) = 0;

    /**
     * Return the ID for the given Record, adding it to the database if it does
     * not already exist
     */
    virtual int obtain_id(const Record& rec) = 0;

    /// Read the LevTr data for an id, returns nullptr if not found
    virtual const DBRow* read(int id) = 0;

    /// Read the contents of the LevTr table
    virtual void read_all(std::function<void(const DBRow&)> dest) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;
};

struct LevTrCache
{
    virtual ~LevTrCache();

    /**
     * Fill a record with level/timerange info with this id.
     *
     * @return true if found, else false
     */
    virtual bool to_rec(int id, Record& rec) = 0;

    /// Return a Level for this ID
    virtual Level to_level(int id) const = 0;

    /// Return a Trange for this ID
    virtual Trange to_trange(int id) const = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    virtual msg::Context* to_msg(int id, Msg& msg) = 0;

    /// Invalidate the cache
    virtual void invalidate() = 0;

    /// Dump cache contents to an output stream
    virtual void dump(FILE* out) const = 0;

    static std::unique_ptr<LevTrCache> create(DB& db);
};

/**
 * Precompiled query to manipulate the data table
 */
struct Data
{
    static std::unique_ptr<Data> create(DB& conn);
    virtual ~Data();

    /// Set the IDs that identify this variable
    virtual void set_context(int id_station, int id_report, int id_lev_tr) = 0;

    /// Set id_lev_tr and datetime to mean 'station information'
    virtual void set_station_info(int id_station, int id_report) = 0;

    /// Set the date from the date information in the record
    virtual void set_date(const Record& rec) = 0;

    /// Set the date from a split up date
    virtual void set_date(int ye, int mo, int da, int ho, int mi, int se) = 0;

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    virtual void insert_or_fail(const wreport::Var& var, int* res_id=nullptr) = 0;

    /**
     * Insert an entry into the data table, ignoring conflicts.
     *
     * Trying to replace an existing value will do nothing.
     *
     * @return true if it was inserted, false if it was already present
     */
    virtual bool insert_or_ignore(const wreport::Var& var, int* res_id=nullptr) = 0;

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     *
     * If id is not NULL, it stores the database id of the inserted/modified
     * data in *id.
     */
    virtual void insert_or_overwrite(const wreport::Var& var, int* res_id=nullptr) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};


/**
 * Precompiled queries to manipulate the attr table
 */
struct Attr
{
    static std::unique_ptr<Attr> create(DB& db);
    virtual ~Attr();

    /**
     * Insert an entry into the attr table
     *
     * If set to true, an existing attribute with the same context and
     * wreport::Varcode will be overwritten
     */
    virtual void write(int id_data, const wreport::Var& var) = 0;

    /**
     * Load from the database all the attributes for var
     *
     * @param var
     *   wreport::Var to which the resulting attributes will be added
     * @return
     *   The error indicator for the function (See @ref error.h)
     */
    virtual void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

/// Query results from SQL output
struct SQLRecord
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
    bool querybest_fields_are_the_same(const SQLRecord& r);
};

/**
 * Run a query on the given statement, returning results as SQLRecord objects
 *
 * SQLRecord is filled with the output variables according to which sel_* is true.
 *
 * Query will dispatch to the right connector routines for the query, based on
 * the actual implementation of stm.
 */
void run_built_query(Connection& conn, const QueryBuilder& qb, std::function<void(SQLRecord& rec)> dest);

void run_delete_query(Connection& conn, const QueryBuilder& qb);

}
}
}

#endif

