/*
 * db/v6/attr - attr table management
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

#ifndef DBALLE_DB_V6_ATTR_H
#define DBALLE_DB_V6_ATTR_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <wreport/var.h>
#include <memory>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct Connection;

namespace v6 {

/**
 * Precompiled queries to manipulate the attr table
 */
struct Attr
{
    static std::unique_ptr<Attr> create(Connection& conn);
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
    virtual void read(int id_data, wreport::Var& var) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

}
}
}

#endif
