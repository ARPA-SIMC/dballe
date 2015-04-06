/*
 * db/lev_tr - lev_tr table management
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

#ifndef DBALLE_DB_POSTGRESQL_LEV_TR_H
#define DBALLE_DB_POSTGRESQL_LEV_TR_H

#include <dballe/db/db.h>
#include <dballe/db/sql/levtr.h>
#include <dballe/db/postgresql/internals.h>
#include <cstdio>
#include <memory>

namespace dballe {
struct Record;
struct Msg;

namespace msg {
struct Context;
}

namespace db {
struct Connection;
struct Statement;

namespace v6 {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct PostgreSQLLevTr : public sql::LevTr
{
protected:
    /**
     * DB connection.
     */
    PostgreSQLConnection& conn;

    DBRow working_row;

public:
    PostgreSQLLevTr(PostgreSQLConnection& conn);
    PostgreSQLLevTr(const LevTr&) = delete;
    PostgreSQLLevTr(const LevTr&&) = delete;
    PostgreSQLLevTr& operator=(const PostgreSQLLevTr&) = delete;
    ~PostgreSQLLevTr();

    /**
     * Return the ID for the given Level and Trange, adding it to the database
     * if it does not already exist
     */
    int obtain_id(const Level& lev, const Trange& tr) override;

    /**
     * Return the ID for the given Record, adding it to the database if it does
     * not already exist
     */
    int obtain_id(const Record& rec) override;

    const DBRow* read(int id) override;
    void read_all(std::function<void(const DBRow&)> dest) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};


}
}
}
#endif
