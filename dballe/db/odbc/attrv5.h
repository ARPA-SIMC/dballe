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

#ifndef DBALLE_DB_ODBC_ATTRV5_H
#define DBALLE_DB_ODBC_ATTRV5_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/db/sql/attrv5.h>
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
 * Precompiled queries to manipulate the attr table
 */
struct ODBCAttrV5 : public sql::AttrV5
{
    /** DB connection. */
    ODBCConnection& conn;

    /** Precompiled select statement */
    ODBCStatement* sstm;
    /** Precompiled insert statement */
    ODBCStatement* istm;
    /** Precompiled replace statement */
    ODBCStatement* rstm;

    /** context id SQL parameter */
    int id_context;
    /** variable id SQL parameter */
    wreport::Varcode id_var;
    /** attribute id SQL parameter */
    wreport::Varcode type;
    /** attribute value SQL parameter */
    char value[255];
    /** attribute value indicator */
    SQLLEN value_ind;

    ODBCAttrV5(ODBCConnection& conn);
    ~ODBCAttrV5();

    void set_context(int id_context, wreport::Varcode id_var) override;

    /**
     * Set the input fields using the values in a wreport::Var
     *
     * @param var
     *   The Var with the data to copy into ins
     */
    void set(const wreport::Var& var) override;

    /**
     * Set the value input field from a string
     *
     * @param value
     *   The value to copy into ins
     */
    void set_value(const char* value) override;

    /**
     * Insert an entry into the attr table
     *
     * @param replace
     *   If set to true, an existing attribute with the same context and
     *   wreport::Varcode will be overwritten; else, trying to replace an
     *   existing attribute will result in an error.
     */
    void insert() override;

    /**
     * Load from the database all the attributes for var
     *
     * @param var
     *   wreport::Var to which the resulting attributes will be added
     * @return
     *   The error indicator for the function (See @ref error.h)
     */
    void load(int id_context, wreport::Var& var) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;

private:
    // disallow copy
    ODBCAttrV5(const ODBCAttrV5&);
    ODBCAttrV5& operator=(const ODBCAttrV5&);
};

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif