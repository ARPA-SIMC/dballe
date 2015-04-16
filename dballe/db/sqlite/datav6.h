/*
 * db/v6/data - data table management
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

#ifndef DBALLE_DB_SQLITE_V6_DATA_H
#define DBALLE_DB_SQLITE_V6_DATA_H

/** @file
 * @ingroup db
 *
 * Data table management used by the db module.
 */

#include <dballe/db/sql/datav6.h>
#include <dballe/db/sqlite/internals.h>

namespace dballe {
struct Record;

namespace db {
namespace sqlite {
struct DB;

/**
 * Precompiled query to manipulate the data table
 */
class SQLiteDataV6 : public sql::DataV6
{
protected:
    /** DB connection. */
    SQLiteConnection& conn;

    /** Precompiled insert statement */
    SQLiteStatement* istm = nullptr;
    /** Precompiled update statement */
    SQLiteStatement* ustm = nullptr;
    /** Precompiled select ID statement */
    SQLiteStatement* sidstm = nullptr;

    /// Date SQL parameter
    Datetime date;

public:
    SQLiteDataV6(SQLiteConnection& conn);
    SQLiteDataV6(const SQLiteDataV6&) = delete;
    SQLiteDataV6(const SQLiteDataV6&&) = delete;
    SQLiteDataV6& operator=(const SQLiteDataV6&) = delete;
    ~SQLiteDataV6();

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
