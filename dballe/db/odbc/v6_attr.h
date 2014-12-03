/*
 * db/v6/odbc/attr - attribute table management
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

#ifndef DBALLE_DB_ODBC_V6_ATTR_H
#define DBALLE_DB_ODBC_V6_ATTR_H

#include <dballe/db/v6/internals.h>
#include <dballe/db/odbc/internals.h>

namespace dballe {
namespace db {
namespace v6 {

/**
 * Precompiled queries to manipulate the attr table
 */
class ODBCAttr : public Attr
{
protected:
    /** DB connection. */
    ODBCConnection& conn;

    /** Precompiled select statement */
    ODBCStatement* sstm;
    /** Precompiled insert statement */
    ODBCStatement* istm;
    /** Precompiled replace statement */
    ODBCStatement* rstm;

    /** id_data SQL parameter */
    int id_data;
    /** attribute id SQL parameter */
    wreport::Varcode type;
    /** attribute value SQL parameter */
    char value[255];
    /** attribute value indicator */
    SQLLEN value_ind;

    /**
     * Set the value input field from a string
     *
     * @param value
     *   The value to copy into ins
     */
    void set_value(const char* value);

public:
    ODBCAttr(ODBCConnection& conn);
    ~ODBCAttr();

    /**
     * Insert an entry into the attr table
     *
     * If set to true, an existing attribute with the same context and
     * wreport::Varcode will be overwritten
     */
    void write(int id_data, const wreport::Var& var) override;

    /**
     * Load from the database all the attributes for var
     *
     * @param var
     *   wreport::Var to which the resulting attributes will be added
     * @return
     *   The error indicator for the function (See @ref error.h)
     */
    void read(int id_data, wreport::Var& var) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;

private:
    // disallow copy
    ODBCAttr(const ODBCAttr&);
    ODBCAttr& operator=(const ODBCAttr&);
};

}
}
}
#endif
