/*
 * db/odbc/lev_tr - lev_tr table management
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

#ifndef DBALLE_DB_ODBC_LEV_TR_H
#define DBALLE_DB_ODBC_LEV_TR_H

/** @file
 * @ingroup db
 *
 * lev_tr table management used by the db module.
 */

#include <dballe/db/db.h>
#include <dballe/db/sql/levtr.h>
#include <dballe/db/odbc/internals.h>
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

namespace odbc {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct ODBCLevTrV6 : public sql::LevTr
{
protected:
    /**
     * DB connection.
     */
    ODBCConnection& conn;

    /// lev_tr ID sequence, for databases that need it
    db::Sequence* seq_lev_tr = nullptr;

    /** Precompiled select statement */
    ODBCStatement* sstm = nullptr;
    /** Precompiled select data statement */
    ODBCStatement* sdstm = nullptr;
    /** Precompiled insert statement */
    ODBCStatement* istm = nullptr;
    /** Precompiled delete statement */
    ODBCStatement* dstm = nullptr;

    DBRow working_row;

    /**
     * Insert a new lev_tr in the database
     *
     * @return
     *   The ID of the newly inserted lev_tr
     */
    int insert();

    /**
     * Get the lev_tr id for the current lev_tr data.
     *
     * @return
     *   The database ID, or -1 if no existing lev_tr entry matches the given values
     */
    int get_id();

    /**
     * Remove a lev_tr record
     */
    void remove();

public:
    ODBCLevTrV6(ODBCConnection& conn);
    ODBCLevTrV6(const LevTr&) = delete;
    ODBCLevTrV6(const LevTr&&) = delete;
    ODBCLevTrV6& operator=(const ODBCLevTrV6&) = delete;
    ~ODBCLevTrV6();

    /**
     * Return the ID for the given Level and Trange, adding it to the database
     * if it does not already exist
     */
    int obtain_id(const Level& lev, const Trange& tr) override;

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

