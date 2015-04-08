/*
 * db/v6/internals - internal interfaces for v6 db implementation
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

#ifndef DBALLE_DB_V6_INTERNALS_H
#define DBALLE_DB_V6_INTERNALS_H

/** @file
 * @ingroup db
 *
 * Attribute table management used by the db module.
 */

#include <dballe/core/defs.h>
#include <dballe/db/sql/repinfo.h>
#include <dballe/db/sql/station.h>
#include <dballe/db/sql/levtr.h>
#include <dballe/db/sql/datav6.h>
#include <dballe/db/sql/attrv6.h>
#include <wreport/var.h>
#include <memory>
#include <cstdio>

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
struct QueryBuilder;

/// Precompiled queries to manipulate the repinfo table
std::unique_ptr<sql::Repinfo> create_repinfo(Connection& conn);

/// Precompiled queries to manipulate the station table
std::unique_ptr<sql::Station> create_station(Connection& conn);

/// Precompiled queries to manipulate the levtr table
std::unique_ptr<sql::LevTr> create_levtr(Connection& conn);

/// Precompiled queries to manipulate the levtr table
std::unique_ptr<sql::DataV6> create_datav6(Connection& conn);

/// Precompiled queries to manipulate the levtr table
std::unique_ptr<sql::AttrV6> create_attrv6(Connection& conn);

/// Query results from SQL output
struct SQLRecord
{
    int out_lat;
    int out_lon;
    char out_ident[64];
    int out_ident_size; // -1 for NULL
    wreport::Varcode out_varcode;
    Datetime out_datetime;
    Datetime out_datetimemax;
    char out_value[255];
    int out_rep_cod;
    int out_ana_id;
    int out_id_ltr;
    int out_id_data;
    int priority;

    /**
     * Checks true if ana_id, id_ltr, datetime and varcode are the same in
     * both records
     *
     * @returns true if they match, false if they are different
     */
    bool querybest_fields_are_the_same(const SQLRecord& r);
};

/**
 * Run a query on the given statement, returning results as SQLRecord objects
 *
 * SQLRecord is filled with the output variables according to which sel_* is true.
 *
 * Query will dispatch to the right connector routines for the query, based on
 * the actual implementation of stm.
 */
void run_built_query(Connection& conn, const QueryBuilder& qb, std::function<void(SQLRecord& rec)> dest);

void run_delete_query(Connection& conn, const QueryBuilder& qb);

}
}
}

#endif

