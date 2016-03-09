/*
 * db/mem/repinfo - repinfo priority tracking
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_MEM_REPINFO_H
#define DBALLE_DB_MEM_REPINFO_H

#include <map>
#include <string>
#include <cstddef>

namespace dballe {
namespace db {
namespace mem {

class Repinfo
{
protected:
    std::map<std::string, int> priorities;

public:
    /// Get the priority for a rep_memo
    int get_prio(const std::string& memo);

    /// Like update, but it ignores the added, delete and updated stats
    void load(const char* repinfo_file=0);

    /**
     * Update the report type information in the database using the data from the
     * given file.
     *
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
    void update(const char* deffile, int* added, int* deleted, int* updated);

    /**
     * Get a mapping between rep_memo and their priorities
     */
    std::map<std::string, int> get_priorities() const;

    /// Dump contents to a file
    void dump(FILE* out) const;
};


}
}
}
#endif
