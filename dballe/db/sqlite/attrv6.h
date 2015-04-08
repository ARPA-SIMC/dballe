/*
 * db/v6/odbc/attr - attribute table management
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

#ifndef DBALLE_DB_SQLITE_ATTRV6_H
#define DBALLE_DB_SQLITE_ATTRV6_H

#include <dballe/db/sql/attrv6.h>
#include <dballe/db/sqlite/internals.h>

namespace dballe {
namespace db {
namespace v6 {

/**
 * Precompiled queries to manipulate the attr table
 */
class SQLiteAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    SQLiteConnection& conn;

    /** Precompiled select statement */
    SQLiteStatement* sstm = nullptr;
    /** Precompiled replace statement */
    SQLiteStatement* rstm = nullptr;

public:
    SQLiteAttrV6(SQLiteConnection& conn);
    SQLiteAttrV6(const SQLiteAttrV6&) = delete;
    SQLiteAttrV6(const SQLiteAttrV6&&) = delete;
    SQLiteAttrV6& operator=(const SQLiteAttrV6&) = delete;
    ~SQLiteAttrV6();

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
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

}
}
}
#endif
