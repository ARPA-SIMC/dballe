/*
 * db/data - data table management
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_DATA_H
#define DBALLE_DB_DATA_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <wreport/var.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct Connection;
struct Statement;

/**
 * Precompiled query to manipulate the data table
 */
struct Data
{
	/** DB connection. */
    db::Connection& conn;

	/** Precompiled insert statement */
    db::Statement* istm;
	/** Precompiled update statement */
    db::Statement* ustm;
	/** Precompiled insert or ignore statement */
    db::Statement* iistm;

	/** Context ID SQL parameter */
	DBALLE_SQL_C_SINT_TYPE id_context;
	/** Variable type SQL parameter */
    wreport::Varcode id_var;
	/** Variable value SQL parameter */
	char value[255];
	/** Variable value indicator */
	SQLLEN value_ind;

    Data(Connection& conn);
    ~Data();

    /**
     * Set the value input fields using a wreport::Var
     */
    void set(const wreport::Var& var);

    /**
     * Set the value input fields using a string value
     */
    void set_value(const char* value);

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    void insert_or_fail();

    /**
     * Insert an entry into the data table, ignoring conflicts.
     *
     * Trying to replace an existing value will do nothing.
     *
     * @return true if it was inserted, false if it was already present
     */
    bool insert_or_ignore();

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     */
    void insert_or_overwrite();

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

private:
	// disallow copy
	Data(const Data&);
	Data& operator=(const Data&);
};

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
