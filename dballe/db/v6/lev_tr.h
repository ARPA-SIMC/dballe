/*
 * db/v6/lev_tr - lev_tr table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_V6_LEV_TR_H
#define DBALLE_DB_V6_LEV_TR_H

/** @file
 * @ingroup db
 *
 * lev_tr table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <sqltypes.h>
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
struct LevTr
{
    /**
     * DB connection.
     */
    DB& db;

    /** Precompiled select statement */
    db::Statement* sstm;
    /** Precompiled select data statement */
    db::Statement* sdstm;
    /** Precompiled insert statement */
    db::Statement* istm;
    /** Precompiled delete statement */
    db::Statement* dstm;

    /** lev_tr ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id;

    /** First level type SQL parameter */
    DBALLE_SQL_C_SINT_TYPE ltype1;
    /** Level L1 SQL parameter */
    DBALLE_SQL_C_SINT_TYPE l1;
    /** Second level type SQL parameter */
    DBALLE_SQL_C_SINT_TYPE ltype2;
    /** Level L2 SQL parameter */
    DBALLE_SQL_C_SINT_TYPE l2;
    /** Time range type SQL parameter */
    DBALLE_SQL_C_SINT_TYPE pind;
    /** Time range P1 SQL parameter */
    DBALLE_SQL_C_SINT_TYPE p1;
    /** Time range P2 SQL parameter */
    DBALLE_SQL_C_SINT_TYPE p2;

    LevTr(DB& db);
    ~LevTr();

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
     * Insert a new lev_tr in the database
     *
     * @return
     *   The ID of the newly inserted lev_tr
     */
    int insert();

    /**
     * Remove a lev_tr record
     */
    void remove();

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

private:
    // disallow copy
    LevTr(const LevTr&);
    LevTr& operator=(const LevTr&);
};

struct LevTrCache
{
    virtual ~LevTrCache();

    /**
     * Fill a record with level/timerange info with this id.
     *
     * @return true if found, else false
     */
    virtual bool to_rec(int id, Record& rec) = 0;

    /**
     * Get/create a Context in the Msg for this level/timerange.
     *
     * @returns the context, or 0 if the id is not valid.
     */
    virtual msg::Context* to_msg(int id, Msg& msg) = 0;

    /// Invalidate the cache
    virtual void invalidate() = 0;

    /// Dump cache contents to an output stream
    virtual void dump(FILE* out) const = 0;

    static std::auto_ptr<LevTrCache> create(LevTr& lt);
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
