/*
 * db/v6/data - data table management
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

#ifndef DBALLE_DB_V6_DATA_H
#define DBALLE_DB_V6_DATA_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <wreport/var.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
struct Statement;

namespace v6 {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
struct Data
{
    /** DB connection. */
    v6::DB& db;

    /** Precompiled insert statement */
    db::Statement* istm;
    /** Precompiled update statement */
    db::Statement* ustm;
    /** Precompiled insert or ignore statement */
    db::Statement* iistm;
    /** Precompiled select ID statement */
    db::Statement* sidstm;

    /** data ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id;

    /** Station ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id_station;
    /** Report ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id_report;
    /** Context ID SQL parameter */
    DBALLE_SQL_C_SINT_TYPE id_lev_tr;
    /// NULL indicator for id_lev_tr
    SQLLEN id_lev_tr_ind;
    /** Date SQL parameter */
    SQL_TIMESTAMP_STRUCT date;
    /** Variable type SQL parameter */
    wreport::Varcode id_var;
    /** Variable value SQL parameter */
    char value[255];
    /** Variable value indicator */
    SQLLEN value_ind;

    Data(v6::DB& conn);
    ~Data();

    /**
     * Set the date from the date information in the record
     */
    void set_date(const Record& rec);

    /**
     * Set id_lev_tr and datetime to mean 'station information'
     */
    void set_station_info();

    /**
     * Set id_lev_tr to a valid value
     */
    void set_id_lev_tr(int id);

    /**
     * Set the value input fields using a wreport::Var
     */
    void set(const wreport::Var& var);

    /**
     * Set the value input fields using a string value
     */
    void set_value(const char* value);

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    void insert_or_fail(bool want_id=false);

    /**
     * Insert an entry into the data table, ignoring conflicts.
     *
     * Trying to replace an existing value will do nothing.
     *
     * @return true if it was inserted, false if it was already present
     */
    bool insert_or_ignore(bool want_id=false);

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     *
     * If id is not NULL, it stores the database id of the inserted/modified
     * data in *id.
     */
    void insert_or_overwrite(bool want_id=false);

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out);

private:
    // disallow copy
    Data(const Data&);
    Data& operator=(const Data&);
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
