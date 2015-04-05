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

#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/db/sqlite/internals.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

SQLiteRepinfo::SQLiteRepinfo(SQLiteConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

SQLiteRepinfo::~SQLiteRepinfo()
{
}

void SQLiteRepinfo::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto stm = conn.sqlitestatement("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    stm->execute([&]() {
        string memo = stm->column_string(1);
        string desc = stm->column_string(2);
        string descriptor = stm->column_string(4);
        cache_append(
            stm->column_int(0),
            memo.c_str(),
            desc.c_str(),
            stm->column_int(3),
            descriptor.c_str(),
            stm->column_int(5)
        );
    });

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void SQLiteRepinfo::insert_auto_entry(const char* memo)
{
    auto stm = conn.sqlitestatement("SELECT MAX(id) FROM repinfo");
    unsigned id;
    stm->execute_one([&]() {
        id = stm->column_int(0);
    });

    stm = conn.sqlitestatement("SELECT MAX(prio) FROM repinfo");
    unsigned prio;
    stm->execute_one([&]() {
        prio = stm->column_int(0);
    });

    ++id;
    ++prio;

    stm = conn.sqlitestatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
               VALUES (?, ?, ?, ?, '-', 255)
    )");
    stm->bind(id, memo, memo, prio);
    stm->execute();
}

int SQLiteRepinfo::id_use_count(unsigned id, const char* name)
{
    unsigned count = 0;
    auto stm = conn.sqlitestatement("SELECT COUNT(1) FROM context WHERE id_report=?");
    stm->bind(id);
    stm->execute_one([&]() {
        count = stm->column_int(0);
    });
    return count;
}

void SQLiteRepinfo::update(const char* deffile, int* added, int* deleted, int* updated)
{
    *added = *deleted = *updated = 0;

    // Read the new repinfo data from file
    vector<sql::repinfo::Cache> newitems = read_repinfo_file(deffile);

    {
        auto transaction(conn.transaction());

        // Verify that we are not trying to delete a repinfo entry that is
        // in use
        for (size_t i = 0; i < cache.size(); ++i)
        {
            /* Ensure that we are not deleting a repinfo entry that is already in use */
            if (!cache[i].memo.empty() && cache[i].new_memo.empty())
                if (id_use_count(cache[i].id, cache[i].memo.c_str()) > 0)
                    error_consistency::throwf(
                            "trying to delete repinfo entry %u,%s which is currently in use",
                            (unsigned)cache[i].id, cache[i].memo.c_str());
        }

        /* Perform the changes */

        /* Delete the items that were deleted */
        {
            auto stm = conn.sqlitestatement("DELETE FROM repinfo WHERE id=?");
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && cache[i].new_memo.empty())
                {
                    stm->bind(cache[i].id);
                    stm->execute();
                    ++*deleted;
                }
        }

        /* Update the items that were modified */
        {
            auto stm = conn.sqlitestatement(R"(
                UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?
                 WHERE id=?
            )");
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && !cache[i].new_memo.empty())
                {
                    stm->bind(
                            cache[i].new_memo,
                            cache[i].new_desc,
                            cache[i].new_prio,
                            cache[i].new_descriptor.c_str(),
                            cache[i].new_tablea,
                            cache[i].id);
                    stm->execute();
                    ++*updated;
                }
        }

        /* Insert the new items */
        if (!newitems.empty())
        {
            auto stm = conn.sqlitestatement(R"(
                INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
                     VALUES (?, ?, ?, ?, ?, ?)
            )");

            for (vector<sql::repinfo::Cache>::const_iterator i = newitems.begin();
                    i != newitems.end(); ++i)
            {
                stm->bind(
                    i->id,
                    i->new_memo,
                    i->new_desc,
                    i->new_prio,
                    i->new_descriptor,
                    i->new_tablea);
                stm->execute();
                ++*added;
            }
        }

        transaction->commit();
    }

    /* Reread the cache */
    read_cache();
}

void SQLiteRepinfo::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");

    int count = 0;
    auto stm = conn.sqlitestatement("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    stm->execute([&]() {
        string memo = stm->column_string(1);
        string desc = stm->column_string(2);
        string descriptor = stm->column_string(4);
        fprintf(out, " %4d   %s  %s  %d  %s %d\n",
                stm->column_int(0),
                memo.c_str(),
                desc.c_str(),
                stm->column_int(3),
                descriptor.c_str(),
                stm->column_int(5));

        ++count;
    });
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}


}

namespace v6 {

SQLiteRepinfo::SQLiteRepinfo(SQLiteConnection& conn) : v5::SQLiteRepinfo(conn) {}

int SQLiteRepinfo::id_use_count(unsigned id, const char* name)
{
    unsigned count = 0;
    auto stm = conn.sqlitestatement("SELECT COUNT(1) FROM data WHERE id_report=?");
    stm->bind(id);
    stm->execute_one([&]() {
        count = stm->column_int(0);
    });
    return count;
}

}

}
}
