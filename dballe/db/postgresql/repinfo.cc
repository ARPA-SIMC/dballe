/*
 * db/postgresql/repinfo - repinfo table management
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

#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/db/postgresql/internals.h"
#include "dballe/db/querybuf.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

PostgreSQLRepinfo::PostgreSQLRepinfo(PostgreSQLConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

PostgreSQLRepinfo::~PostgreSQLRepinfo()
{
}

void PostgreSQLRepinfo::read_cache()
{
    cache.clear();
    memo_idx.clear();

    auto stm = conn.exec("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    for (unsigned row = 0; row < stm.rowcount(); ++row)
    {
        cache_append(
            stm.get_int4(row, 0),
            stm.get_string(row, 1),
            stm.get_string(row, 2),
            stm.get_int4(row, 3),
            stm.get_string(row, 4),
            stm.get_int4(row, 5)
        );
    }

    // Rebuild the memo index as well
    rebuild_memo_idx();
}

void PostgreSQLRepinfo::insert_auto_entry(const char* memo)
{
    unsigned id = conn.exec_one_row("SELECT MAX(id) FROM repinfo").get_int4(0, 0);
    int prio = conn.exec_one_row("SELECT MAX(prio) FROM repinfo").get_int4(0, 0);

    ++id;
    ++prio;

    Querybuf query(500);
    query.appendf(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
               VALUES (%u, $1::text, $1::text, %d, '-', 255)
    )", id, prio);
    conn.exec_no_data(query, memo);
}

int PostgreSQLRepinfo::id_use_count(unsigned id, const char* name)
{
    Querybuf query(500);
    query.appendf("SELECT COUNT(1) FROM context WHERE id_report=%u", id);
    return conn.exec_one_row(query).get_int4(0, 0);
}

void PostgreSQLRepinfo::update(const char* deffile, int* added, int* deleted, int* updated)
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
            Querybuf query(1024);
            query.append("DELETE FROM repinfo WHERE id IN (");
            query.start_list(",");
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && cache[i].new_memo.empty())
                {
                    query.append_listf("%u", cache[i].id);
                    ++*deleted;
                }
            query.append(")");
            if (*deleted)
                conn.exec_no_data(query);
        }

        /* Update the items that were modified */
        {
            for (size_t i = 0; i < cache.size(); ++i)
                if (!cache[i].memo.empty() && !cache[i].new_memo.empty())
                {
                    Querybuf query(512);
                    query.appendf(R"(
                        UPDATE repinfo set memo=$1::text, description=$2::text, prio=%d, descriptor=$3::text, tablea=%u
                         WHERE id=%u
                    )", cache[i].new_prio, cache[i].new_tablea, cache[i].id);
                    conn.exec_no_data(query, cache[i].new_memo, cache[i].new_desc, cache[i].new_descriptor);
                    ++*updated;
                }
        }

        /* Insert the new items */
        if (!newitems.empty())
        {
            for (vector<sql::repinfo::Cache>::const_iterator i = newitems.begin();
                    i != newitems.end(); ++i)
            {
                Querybuf query(512);
                query.appendf(R"(
                    INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
                         VALUES (%u, $1::text, $2::text, %d, $3::text, %u)
                )", i->id, i->new_prio, i->new_tablea);
                conn.exec_no_data(query, i->new_memo, i->new_desc, i->new_descriptor);
                ++*added;
            }
        }

        transaction->commit();
    }

    /* Reread the cache */
    read_cache();
}

void PostgreSQLRepinfo::dump(FILE* out)
{
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");

    int count = 0;
    auto stm = conn.exec("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");
    for (unsigned row = 0; row < stm.rowcount(); ++row)
    {
        string memo = stm.get_string(row, 1);
        string desc = stm.get_string(row, 2);
        string descriptor = stm.get_string(row, 4);
        fprintf(out, " %4d   %s  %s  %d  %s %d\n",
                stm.get_int4(row, 0),
                memo.c_str(),
                desc.c_str(),
                stm.get_int4(row, 3),
                descriptor.c_str(),
                stm.get_int4(row, 5));
        ++count;
    };
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}


}

namespace v6 {

PostgreSQLRepinfo::PostgreSQLRepinfo(PostgreSQLConnection& conn) : v5::PostgreSQLRepinfo(conn) {}

int PostgreSQLRepinfo::id_use_count(unsigned id, const char* name)
{
    Querybuf query(500);
    query.appendf("SELECT COUNT(1) FROM data WHERE id_report=%u", id);
    return conn.exec_one_row(query).get_int4(0, 0);
}

}

}
}
