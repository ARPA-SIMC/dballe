/*
 * db/context - context table management
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

#ifndef DBALLE_DB_V5_CONTEXT_H
#define DBALLE_DB_V5_CONTEXT_H

/** @file
 * @ingroup db
 *
 * Context table management used by the db module.
 */

#include <sqltypes.h>
#include <cstdio>

namespace dballe {
namespace db {
struct ODBCConnection;
struct ODBCStatement;
struct Sequence;

namespace v5 {
struct DB;

/**
 * Precompiled queries to manipulate the context table
 */
struct Context
{
    /**
     * DB connection.
     */
    ODBCConnection& conn;

    /// Context ID sequence
    Sequence* seq_context = nullptr;

    /** Precompiled select statement */
    ODBCStatement* sstm = nullptr;
    /** Precompiled select data statement */
    ODBCStatement* sdstm = nullptr;
    /** Precompiled insert statement */
    ODBCStatement* istm = nullptr;
    /** Precompiled delete statement */
    ODBCStatement* dstm = nullptr;

    /// Context ID SQL parameter
    int id;

    /// Station ID SQL parameter
    int id_station;
    /// Report ID SQL parameter
    int id_report;
    /** Date SQL parameter */
    SQL_TIMESTAMP_STRUCT date;
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

    Context(ODBCConnection& conn);
    ~Context();

    /**
     * Get the context id for the current context data.
     *
     * @return
     *   The database ID, or -1 if no existing context entry matches the given values
     */
    int get_id();

    /**
     * Get context information given a context ID
     *
     * @param id
     *   ID of the context to query
     */
    void get_data(int id);

    /**
     * Get the context id for a station info context.
     *
     * id_station and id_report must be filled in for the query.
     *
     * @return
     *   The database ID
     */
    int obtain_station_info();

    /**
     * Insert a new context in the database
     *
     * @return
     *   The ID of the newly inserted context
     */
    int insert();

    /**
     * Remove a context record
     */
    void remove();

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

private:
    // disallow copy
    Context(const Context&);
    Context& operator=(const Context&);
};

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
