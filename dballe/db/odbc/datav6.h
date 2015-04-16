/*
 * db/odbc/datav6 - data table management
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

#ifndef DBALLE_DB_ODBC_DATAV6_H
#define DBALLE_DB_ODBC_DATAV6_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/sql/datav6.h>
#include <dballe/db/odbc/internals.h>

namespace dballe {
struct Record;

namespace db {
namespace odbc {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class ODBCDataV6 : public sql::DataV6
{
protected:
    /** DB connection. */
    ODBCConnection& conn;

    /// data ID sequence, for databases that need it
    db::Sequence* seq_data = nullptr;

    /** Precompiled insert statement */
    ODBCStatement* istm;
    /** Precompiled update statement */
    ODBCStatement* ustm;
    /** Precompiled insert or update statement, for DBs where it is available */
    ODBCStatement* ioustm;
    /** Precompiled select ID statement */
    ODBCStatement* sidstm;

    /** data ID SQL parameter */
    int id;

    /** Date SQL parameter */
    SQL_TIMESTAMP_STRUCT date;
    /** Variable type SQL parameter */
    wreport::Varcode id_var;
    /** Variable value SQL parameter */
    char value[255];
    /** Variable value indicator */
    SQLLEN value_ind;

    /**
     * Set the value input fields using a string value
     */
    void set_value(const char* value);

    /**
     * Set the value input fields using a wreport::Var
     */
    void set(const wreport::Var& var);

public:
    ODBCDataV6(ODBCConnection& conn);
    ODBCDataV6(const ODBCDataV6&) = delete;
    ODBCDataV6(const ODBCDataV6&&) = delete;
    ODBCDataV6& operator=(const ODBCDataV6&) = delete;
    ~ODBCDataV6();

    void insert(Transaction& t, sql::bulk::InsertV6& vars, bool update_existing) override;
    void remove(const v6::QueryBuilder& qb) override;

    /// Set id_lev_tr and datetime to mean 'station information'
    void set_station_info(int id_station, int id_report) override;

    /// Set the date from the date information in the record
    void set_date(const Record& rec) override;

    /// Set the date from a split up date
    void set_date(int ye, int mo, int da, int ho, int mi, int se) override;

    /**
     * Insert an entry into the data table, failing on conflicts.
     *
     * Trying to replace an existing value will result in an error.
     */
    void insert_or_fail(const wreport::Var& var, int* res_id=nullptr) override;

    /**
     * Insert an entry into the data table, overwriting on conflicts.
     *
     * An existing data with the same context and ::dba_varcode will be
     * overwritten.
     *
     * If id is not NULL, it stores the database id of the inserted/modified
     * data in *id.
     */
    void insert_or_overwrite(const wreport::Var& var, int* res_id=nullptr) override;

    /**
     * Dump the entire contents of the table to an output stream
     */
    void dump(FILE* out) override;
};

}
}
}
#endif
