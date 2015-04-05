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
#include "dballe/db/odbc/internals.h"
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

ODBCRepinfo::ODBCRepinfo(ODBCConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

ODBCRepinfo::~ODBCRepinfo()
{
}

void ODBCRepinfo::read_cache()
{
    auto stm = conn.odbcstatement("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");

    unsigned id;
    char memo[20]; SQLLEN memo_ind;
    char description[255]; SQLLEN description_ind;
    int prio;
    char descriptor[6]; SQLLEN descriptor_ind;
    unsigned tablea;

    cache.clear();
    memo_idx.clear();

    stm->bind_out(1, id);
    stm->bind_out(2, memo, sizeof(memo), memo_ind);
    stm->bind_out(3, description, sizeof(description), description_ind);
    stm->bind_out(4, prio);
    stm->bind_out(5, descriptor, sizeof(descriptor), descriptor_ind);
    stm->bind_out(6, tablea);

    stm->execute();

    /* Get the results and save them in the record */
    while (stm->fetch())
        cache_append(id, memo, description, prio, descriptor, tablea);

    /* Rebuild the memo index as well */
    rebuild_memo_idx();
}

void ODBCRepinfo::insert_auto_entry(const char* memo)
{
    auto stm = conn.odbcstatement("SELECT MAX(id) FROM repinfo");
    unsigned id;
    stm->bind_out(1, id);
    stm->execute();
    stm->fetch_expecting_one();

    stm = conn.odbcstatement("SELECT MAX(prio) FROM repinfo");
    unsigned prio;
    stm->bind_out(1, prio);
    stm->execute();
    stm->fetch_expecting_one();

    ++id;
    ++prio;

    stm = conn.odbcstatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
               VALUES (?, ?, ?, ?, '-', 255)
    )");
    stm->bind_in(1, id);
    stm->bind_in(2, memo);
    stm->bind_in(3, memo);
    stm->bind_in(4, prio);
    stm->execute_ignoring_results();
}


#if 0
/*
static void commit_cache_item(struct _dba_db_repinfo_cache* item)
{
    if (item->memo != NULL)
        free(item->memo);
    item->memo = item->new_memo;
    item->new_memo = NULL;

    if (item->desc != NULL)
        free(item->desc);
    item->desc = item->new_desc;
    item->new_desc = NULL;

    if (item->descriptor != NULL)
        free(item->descriptor);
    item->descriptor = item->new_descriptor;
    item->new_descriptor = NULL;
}
*/
#endif


int ODBCRepinfo::id_use_count(unsigned id, const char* name)
{
    unsigned dbid = id;
    unsigned count;
    auto stm = conn.odbcstatement("SELECT COUNT(1) FROM context WHERE id_report = ?");
    stm->bind_in(1, dbid);
    stm->bind_out(1, count);
    stm->execute();
    if (!stm->fetch_expecting_one())
        error_consistency::throwf("%s is in cache but not in the database (database externally modified?)", name);
    return count;
}

void ODBCRepinfo::update(const char* deffile, int* added, int* deleted, int* updated)
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
            auto stm = conn.odbcstatement("DELETE FROM repinfo WHERE id=?");
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && cache[i].new_memo.empty())
                {
                    stm->bind_in(1, cache[i].id);
                    stm->execute_and_close();

                    /* clear_cache_item(&(ri->cache[i])); */
                    ++*deleted;
                }
        }

        /* Update the items that were modified */
        {
            auto stm = conn.odbcstatement(R"(
                UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?
                 WHERE id=?
            )");
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && !cache[i].new_memo.empty())
                {
                    stm->bind_in(1, cache[i].new_memo.c_str());
                    stm->bind_in(2, cache[i].new_desc.c_str());
                    stm->bind_in(3, cache[i].new_prio);
                    stm->bind_in(4, cache[i].new_descriptor.c_str());
                    stm->bind_in(5, cache[i].new_tablea);
                    stm->bind_in(6, cache[i].id);

                    stm->execute_and_close();

                    /* commit_cache_item(&(ri->cache[i])); */
                    ++*updated;
                }
        }

        /* Insert the new items */
        if (!newitems.empty())
        {
            auto stm = conn.odbcstatement(R"(
                INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
                     VALUES (?, ?, ?, ?, ?, ?)
            )");

            for (vector<sql::repinfo::Cache>::const_iterator i = newitems.begin();
                    i != newitems.end(); ++i)
            {
                stm->bind_in(1, i->id);
                stm->bind_in(2, i->new_memo.c_str());
                stm->bind_in(3, i->new_desc.c_str());
                stm->bind_in(4, i->new_prio);
                stm->bind_in(5, i->new_descriptor.c_str());
                stm->bind_in(6, i->new_tablea);

                stm->execute_and_close();

                /* DBA_RUN_OR_GOTO(cleanup, cache_append(ri, id, memo, description, prio, descriptor, tablea)); */
                ++*added;
            }
        }

        transaction->commit();
    }

    /* Reread the cache */
    read_cache();
}

void ODBCRepinfo::dump(FILE* out)
{
    unsigned id;
    char memo[20]; SQLLEN memo_ind;
    char description[255]; SQLLEN description_ind;
    int prio;
    char descriptor[6]; SQLLEN descriptor_ind;
    unsigned tablea;

    auto stm = conn.odbcstatement("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    stm->bind_out(1, id);
    stm->bind_out(2, memo, sizeof(memo), memo_ind);
    stm->bind_out(3, description, sizeof(description), description_ind);
    stm->bind_out(4, prio);
    stm->bind_out(5, descriptor, sizeof(descriptor), descriptor_ind);
    stm->bind_out(6, tablea);
    stm->execute();

    int count;
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");
    for (count = 0; stm->fetch(); ++count)
    {
        fprintf(out, " %4d   %s  %s  %d  %s %u\n",
                (int)id,
                memo_ind == SQL_NULL_DATA ? "-" : memo,
                description_ind == SQL_NULL_DATA ? "-" : description,
                (int)prio,
                descriptor_ind == SQL_NULL_DATA ? "-" : descriptor,
                (unsigned)tablea
               );
    }
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}


}

namespace v6 {

ODBCRepinfo::ODBCRepinfo(ODBCConnection& conn) : v5::ODBCRepinfo(conn) {}

int ODBCRepinfo::id_use_count(unsigned id, const char* name)
{
    unsigned count;
    auto stm = conn.odbcstatement("SELECT COUNT(1) FROM data WHERE id_report = ?");
    stm->bind_in(1, id);
    stm->bind_out(1, count);
    stm->execute();
    if (!stm->fetch_expecting_one())
        error_consistency::throwf("%s is in cache but not in the database (database externally modified?)", name);
    return count;
}
}

}
}
