/*
 * db/mysql/data - data table management
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

#ifndef DBALLE_DB_MYSQL_V6_DATA_H
#define DBALLE_DB_MYSQL_V6_DATA_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/sql/datav6.h>
#include <dballe/sql/fwd.h>

namespace dballe {
struct Record;

namespace db {
namespace mysql {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class MySQLDataV6 : public sql::DataV6
{
protected:
    /** DB connection. */
    dballe::sql::MySQLConnection& conn;

public:
    MySQLDataV6(dballe::sql::MySQLConnection& conn);
    MySQLDataV6(const MySQLDataV6&) = delete;
    MySQLDataV6(const MySQLDataV6&&) = delete;
    MySQLDataV6& operator=(const MySQLDataV6&) = delete;
    ~MySQLDataV6();

    void insert(dballe::sql::Transaction& t, sql::bulk::InsertV6& vars, UpdateMode update_mode=UPDATE) override;
    void remove(const v6::QueryBuilder& qb) override;
    void dump(FILE* out) override;
};

}
}
}
#endif
