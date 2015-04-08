/*
 * db/sql/attrv6 - v6 implementation of attr table
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

#ifndef DBALLE_DB_SQL_ATTRV6_H
#define DBALLE_DB_SQL_ATTRV6_H

#include <wreport/var.h>
#include <functional>
#include <vector>
#include <memory>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
namespace sql {

struct AttributeList;

/**
 * Precompiled queries to manipulate the attr table
 */
struct AttrV6
{
protected:
    /**
     * Add all the attributes in attrs as attributes of the variable id_data.
     *
     * Existing attributes are replaced if they are also found in var, but are
     * not deleted if they are not found in var.
     */
    virtual void impl_add(int id_data, AttributeList& attrs) = 0;

public:
    virtual ~AttrV6();

    /**
     * Add all the variables in attrs as attributes of id_data.
     *
     * Existing attributes are replaced but not deleted.
     */
    void add(int id_data, const Record& attrs);

    /**
     * Add all the attributes of var as attributes of id_data.
     *
     * Existing attributes are replaced but not deleted.
     */
    void add(int id_data, const wreport::Var& var);

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

