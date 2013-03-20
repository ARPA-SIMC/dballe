/*
 * db/context - context table management
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_CONTEXT_H
#define DBALLE_DB_CONTEXT_H

/** @file
 * @ingroup db
 *
 * Context table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct Connection;
struct Statement;

/**
 * Precompiled queries to manipulate the context table
 */
struct Context
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

    /** Context ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id;

    /** Station ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id_station;
    /** Report ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id_report;
    /** Date SQL parameter */
    SQL_TIMESTAMP_STRUCT date;
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

    Context(DB& db);
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

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
