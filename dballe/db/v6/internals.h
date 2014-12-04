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

#include <wreport/var.h>
#include <memory>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
struct Connection;

namespace v6 {
struct DB;

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

}
}
}

#endif

