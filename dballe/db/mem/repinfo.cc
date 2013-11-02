/*
 * db/mem/repinfo - repinfo table management
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

#include "repinfo.h"
#include "dballe/db/internals.h"
#include "dballe/core/csv.h"
#include <wreport/error.h>
#include <set>
#include <limits>
#include <cstdlib>
#include <cstdio>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace mem {

namespace {

struct fd_closer
{
    FILE* fd;
    fd_closer(FILE* fd) : fd(fd) {}
    ~fd_closer() { fclose(fd); }
};

static inline void inplace_tolower(std::string& buf)
{
    for (string::iterator i = buf.begin(); i != buf.end(); ++i)
        *i = tolower(*i);
}

}

void Repinfo::load(const char* repinfo_file)
{
    int added, deleted, updated;
    update(repinfo_file, &added, &deleted, &updated);
}

void Repinfo::update(const char* deffile, int* added, int* deleted, int* updated)
{
    *added = *deleted = *updated = 0;

    if (deffile == 0)
        deffile = default_repinfo_file();

    // Open the input CSV file
    FILE* in = fopen(deffile, "r");
    if (in == NULL)
        error_system::throwf("opening file %s", deffile);
    fd_closer closer(in);

    set<string> oldkeys;
    for (map<string, int>::const_iterator i = priorities.begin(); i != priorities.end(); ++i)
        oldkeys.insert(i->first);

    // Read the CSV file
    vector<string> columns;
    for (int line = 1; csv_read_next(in, columns); ++line)
    {
        if (columns.size() != 6)
            error_parse::throwf(deffile, line, "Expected 6 columns, got %zd", columns.size());

        // Lowercase all rep_memos
        inplace_tolower(columns[1]);
        string memo = columns[1];
        int prio = strtol(columns[3].c_str(), 0, 10);

        map<string, int>::iterator old = priorities.find(memo);
        if (old == priorities.end())
        {
            priorities.insert(make_pair(memo, prio));
            ++*added;
        } else {
            old->second = prio;
            ++*updated;
        }

        oldkeys.erase(memo);
    }

    for (set<string>::const_iterator i = oldkeys.begin(); i != oldkeys.end(); ++i)
    {
        priorities.erase(*i);
        ++*deleted;
    }
}

std::map<std::string, int> Repinfo::get_priorities() const
{
    return priorities;
}

int Repinfo::get_prio(const std::string& memo)
{
    string lc_memo = memo;
    inplace_tolower(lc_memo);

    std::map<std::string, int>::const_iterator i = priorities.find(lc_memo);
    if (i != priorities.end())
        return i->second;

    int max_prio = numeric_limits<int>::min();
    for (std::map<std::string, int>::const_iterator i = priorities.begin();
            i != priorities.end(); ++i)
        if (i->second > max_prio)
            max_prio = i->second;

    int new_prio = max_prio + 1;
    priorities.insert(make_pair(lc_memo, new_prio));
    return new_prio;
}

void Repinfo::dump(FILE* out) const
{
    for (std::map<std::string, int>::const_iterator i = priorities.begin();
            i != priorities.end(); ++i)
        fprintf(out, " %s: %d\n", i->first.c_str(), i->second);
}

}
}
}
