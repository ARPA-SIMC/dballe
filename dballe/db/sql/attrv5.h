/*
 * db/attr - attr table management
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

#ifndef DBALLE_DB_SQL_ATTRV5_H
#define DBALLE_DB_SQL_ATTRV5_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <wreport/var.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct ODBCConnection;
struct ODBCStatement;

namespace sql {

/**
 * Precompiled queries to manipulate the attr table
 */
struct AttrV5
{
    virtual ~AttrV5();

    /// Set the input context fields
    virtual void set_context(int id_context, wreport::Varcode id_var) = 0;

    /**
     * Set the input fields using the values in a wreport::Var
     *
     * @param var
     *   The Var with the data to copy into ins
     */
    virtual void set(const wreport::Var& var) = 0;

    /**
     * Set the value input field from a string
     *
     * @param value
     *   The value to copy into ins
     */
    virtual void set_value(const char* value) = 0;

    /**
     * Insert an entry into the attr table
     *
     * @param replace
     *   If set to true, an existing attribute with the same context and
     *   wreport::Varcode will be overwritten; else, trying to replace an
     *   existing attribute will result in an error.
     */
    virtual void insert() = 0;

    /**
     * Load from the database all the attributes for var
     *
     * @param var
     *   wreport::Var to which the resulting attributes will be added
     * @return
     *   The error indicator for the function (See @ref error.h)
     */
    virtual void load(int id_context, wreport::Var& var) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

}
}
}
#endif
