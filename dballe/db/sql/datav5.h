/*
 * db/data - data table management
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

#ifndef DBALLE_DB_SQL_DATAV5_H
#define DBALLE_DB_SQL_DATAV5_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
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
 * Precompiled query to manipulate the data table
 */
struct DataV5
{
    virtual ~DataV5();

    /// Set the input context ID
    virtual void set_context_id(int context_id) = 0;

    /**
     * Set the value input fields using a wreport::Var
     */
    virtual void set(const wreport::Var& var) = 0;

    /**
     * Set the value input fields using a string value
     */
    virtual void set_value(const char* value) = 0;

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    virtual void insert_or_fail() = 0;

    /**
     * Insert an entry into the data table, ignoring conflicts.
     *
     * Trying to replace an existing value will do nothing.
     *
     * @return true if it was inserted, false if it was already present
     */
    virtual bool insert_or_ignore() = 0;

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     */
    virtual void insert_or_overwrite() = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

}
}
}

#endif
