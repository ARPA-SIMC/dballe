/*
 * db/sqlite/attr - attribute table management
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
namespace sqlite {

/**
 * Precompiled queries to manipulate the attr table
 */
class SQLiteAttrV6 : public sql::AttrV6
{
protected:
    /** DB connection. */
    SQLiteConnection& conn;

    /// Precompiled select statement
    SQLiteStatement* sstm = nullptr;
    /// Precompiled insert statement
    SQLiteStatement* istm = nullptr;
    /// Precompiled update statement
    SQLiteStatement* ustm = nullptr;

public:
    SQLiteAttrV6(SQLiteConnection& conn);
    SQLiteAttrV6(const SQLiteAttrV6&) = delete;
    SQLiteAttrV6(const SQLiteAttrV6&&) = delete;
    SQLiteAttrV6& operator=(const SQLiteAttrV6&) = delete;
    ~SQLiteAttrV6();

    void insert(Transaction& t, sql::bulk::InsertAttrsV6& vars, UpdateMode update_mode=UPDATE) override;
    void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void dump(FILE* out) override;
};

}
}
}
#endif
