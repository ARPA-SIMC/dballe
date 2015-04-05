/*
 * db/v5/repinfo - repinfo table management
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

#ifndef DBALLE_DB_SQLITE_V5_REPINFO_H
#define DBALLE_DB_SQLITE_V5_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <dballe/db/sql/repinfo.h>
#include <vector>
#include <string>
#include <map>

namespace dballe {
struct Record;

namespace db {
struct SQLiteConnection;

namespace v5 {

/**
 * Fast cached access to the repinfo table
 */
struct SQLiteRepinfo : public sql::Repinfo
{
    /**
     * DB connection. The pointer is assumed always valid during the
     * lifetime of the object
     */
    SQLiteConnection& conn;

    SQLiteRepinfo(SQLiteConnection& conn);
    SQLiteRepinfo(const SQLiteRepinfo&) = delete;
    SQLiteRepinfo(const SQLiteRepinfo&&) = delete;
    virtual ~SQLiteRepinfo();
    SQLiteRepinfo& operator=(const SQLiteRepinfo&) = delete;

    void update(const char* deffile, int* added, int* deleted, int* updated) override;
    void dump(FILE* out) override;

protected:
    /// Return how many time this ID is used in the database
    virtual int id_use_count(unsigned id, const char* name);

    void read_cache() override;
    void insert_auto_entry(const char* memo) override;
};

}

namespace v6 {

struct SQLiteRepinfo : public v5::SQLiteRepinfo
{
    SQLiteRepinfo(SQLiteConnection& conn);

protected:
    int id_use_count(unsigned id, const char* name) override;
};

}
}
}
#endif

