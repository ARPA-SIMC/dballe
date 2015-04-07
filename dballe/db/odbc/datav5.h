/*
 * db/data - data table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_V5_DATA_H
#define DBALLE_DB_V5_DATA_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/sql/datav5.h>
#include <wreport/var.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct ODBCConnection;
struct ODBCStatement;

namespace v5 {

/**
 * Precompiled query to manipulate the data table
 */
struct ODBCDataV5 : public sql::DataV5
{
    /** DB connection. */
    ODBCConnection& conn;

    /** Precompiled insert statement */
    ODBCStatement* istm;
    /** Precompiled update statement */
    ODBCStatement* ustm;
    /** Precompiled insert or ignore statement */
    ODBCStatement* iistm;

    /// Context ID SQL parameter
    int id_context;
    /** Variable type SQL parameter */
    wreport::Varcode id_var;
    /** Variable value SQL parameter */
    char value[255];
    /** Variable value indicator */
    SQLLEN value_ind;

    ODBCDataV5(ODBCConnection& conn);
    ~ODBCDataV5();
    ODBCDataV5(const ODBCDataV5&) = delete;
    ODBCDataV5(ODBCDataV5&&) = delete;
    ODBCDataV5& operator=(const ODBCDataV5&) = delete;
    ODBCDataV5& operator=(ODBCDataV5&&) = delete;

    void set_context_id(int context_id) override;

    /**
     * Set the value input fields using a wreport::Var
     */
    void set(const wreport::Var& var) override;

    /**
     * Set the value input fields using a string value
     */
    void set_value(const char* value) override;

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    void insert_or_fail() override;

    /**
     * Insert an entry into the data table, ignoring conflicts.
     *
     * Trying to replace an existing value will do nothing.
     *
     * @return true if it was inserted, false if it was already present
     */
    bool insert_or_ignore() override;

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     */
    void insert_or_overwrite() override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

}
}
}

#endif
