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

#ifndef DBALLE_DB_V5_REPINFO_H
#define DBALLE_DB_V5_REPINFO_H

/** @file
 * @ingroup db
 *
 * Repinfo table management used by the db module.
 */

#include <memory>
#include <map>
#include <string>
#include <vector>

namespace dballe {
struct Record;

namespace db {
struct Connection;

namespace v5 {

/// Fast cached access to the repinfo table
struct Repinfo
{
    Connection& conn;

    Repinfo(Connection& conn);
    virtual ~Repinfo() {}

    static std::unique_ptr<Repinfo> create(Connection& conn);

    /**
     * Fill repinfo information in a Record based on the repinfo entry with the
     * given ID
     */
    virtual void to_record(int id, Record& rec) = 0;

    /// Get the rep_memo for a given ID; throws if id is not valud
    virtual const char* get_rep_memo(int id) = 0;

    /// Get the ID for a given rep_memo; throws if rep_memo is not valid
    virtual int get_id(const char* rep_memo) = 0;

    /// Get the priority for a given ID; returns INT_MAX if id is not valid
    virtual int get_priority(int id) = 0;

    /**
     * Update the report type information in the database using the data from the
     * given file.
     *
     * @param ri
     *   dba_db_repinfo used to update the database
     * @param deffile
     *   Pathname of the file to use for the update.  The NULL value is accepted
     *   and means to use the default configure repinfo.csv file.
     * @retval added
     *   Number of entries that have been added during the update.
     * @retval deleted
     *   Number of entries that have been deleted during the update.
     * @retval updated
     *   Number of entries that have been updated during the update.
     */
    virtual void update(const char* deffile, int* added, int* deleted, int* updated) = 0;

    /**
     * Get a mapping between rep_memo and their priorities
     */
    virtual std::map<std::string, int> get_priorities() = 0;

    /**
     * Return a vector of IDs matching the priority constraints in the given record.
     */
    virtual std::vector<int> ids_by_prio(const Record& rec) = 0;

    /**
     * Get the id of a repinfo entry given its name.
     *
     * It creates a new entry if the memo is missing from the database.
     *
     * @param memo
     *   The name to query
     * @return
     *   The resulting id.
     */
    virtual int obtain_id(const char* memo) = 0;

    /// Dump the entire contents of the database to an output stream
    virtual void dump(FILE* out) = 0;
};

}
}
}
#endif
