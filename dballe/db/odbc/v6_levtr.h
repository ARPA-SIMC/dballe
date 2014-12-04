/*
 * db/v6/lev_tr - lev_tr table management
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

#ifndef DBALLE_DB_ODBC_V6_LEV_TR_H
#define DBALLE_DB_ODBC_V6_LEV_TR_H

/** @file
 * @ingroup db
 *
 * lev_tr table management used by the db module.
 */

#include <dballe/db/v6/internals.h>
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

namespace v6 {
struct DB;

/**
 * Precompiled queries to manipulate the lev_tr table
 */
struct ODBCLevTr : public LevTr
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

    /// lev_tr ID SQL parameter
    int id;

    /// First level type SQL parameter
    int ltype1;
    /// Level L1 SQL parameter
    int l1;
    /// Second level type SQL parameter
    int ltype2;
    /// Level L2 SQL parameter
    int l2;
    /// Time range type SQL parameter
    int pind;
    /// Time range P1 SQL parameter
    int p1;
    /// Time range P2 SQL parameter
    int p2;

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
     * Get lev_tr information given a lev_tr ID
     *
     * @param id
     *   ID of the lev_tr to query
     */
    void get_data(int id);

    /**
     * Remove a lev_tr record
     */
    void remove();

public:
    ODBCLevTr(ODBCConnection& conn);
    ODBCLevTr(const LevTr&) = delete;
    ODBCLevTr(const LevTr&&) = delete;
    ODBCLevTr& operator=(const ODBCLevTr&) = delete;
    ~ODBCLevTr();

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

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};


}
}
}
#endif

