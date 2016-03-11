/*
 * db/mysql/attr - attribute table management
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

#ifndef DBALLE_DB_MYSQL_ATTRV6_H
#define DBALLE_DB_MYSQL_ATTRV6_H

#include <dballe/db/sql/attrv6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
namespace db {
namespace mysql {

/**
 * Precompiled queries to manipulate the attr table
 */
class MySQLAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    dballe::sql::MySQLConnection& conn;

public:
    MySQLAttrV6(dballe::sql::MySQLConnection& conn);
    MySQLAttrV6(const MySQLAttrV6&) = delete;
    MySQLAttrV6(const MySQLAttrV6&&) = delete;
    MySQLAttrV6& operator=(const MySQLAttrV6&) = delete;
    ~MySQLAttrV6();

    void insert(dballe::sql::Transaction& t, sql::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
#endif
