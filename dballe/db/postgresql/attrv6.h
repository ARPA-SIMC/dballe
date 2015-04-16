/*
 * db/postgresql/attrv6 - attribute table management
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_POSTGRESQL_ATTRV6_H
#define DBALLE_DB_POSTGRESQL_ATTRV6_H

#include <dballe/db/sql/attrv6.h>
#include <dballe/db/postgresql/internals.h>

namespace dballe {
namespace db {
namespace postgresql {

/**
 * Precompiled queries to manipulate the attr table
 */
class PostgreSQLAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    PostgreSQLConnection& conn;

    void impl_add(int id_data, sql::AttributeList& attrs) override;

public:
    PostgreSQLAttrV6(PostgreSQLConnection& conn);
    PostgreSQLAttrV6(const PostgreSQLAttrV6&) = delete;
    PostgreSQLAttrV6(const PostgreSQLAttrV6&&) = delete;
    PostgreSQLAttrV6& operator=(const PostgreSQLAttrV6&) = delete;
    ~PostgreSQLAttrV6();

    void insert(Transaction& t, sql::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;

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
