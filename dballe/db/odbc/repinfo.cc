/*
 * db/odbc/repinfo - repinfo table management
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
#include "dballe/db/odbc/internals.h"
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace odbc {

ODBCRepinfoBase::ODBCRepinfoBase(ODBCConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

ODBCRepinfoBase::~ODBCRepinfoBase()
{
}

void ODBCRepinfoBase::read_cache()
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

void ODBCRepinfoBase::insert_auto_entry(const char* memo)
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


int ODBCRepinfoBase::id_use_count(unsigned id, const char* name)
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

void ODBCRepinfoBase::delete_entry(unsigned id)
{
    auto stm = conn.odbcstatement("DELETE FROM repinfo WHERE id=?");
    stm->bind_in(1, id);
    stm->execute_and_close();
}

void ODBCRepinfoBase::update_entry(const sql::repinfo::Cache& entry)
{
    auto stm = conn.odbcstatement(R"(
        UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?
         WHERE id=?
    )");
    stm->bind_in(1, entry.new_memo.c_str());
    stm->bind_in(2, entry.new_desc.c_str());
    stm->bind_in(3, entry.new_prio);
    stm->bind_in(4, entry.new_descriptor.c_str());
    stm->bind_in(5, entry.new_tablea);
    stm->bind_in(6, entry.id);
    stm->execute_and_close();
}

void ODBCRepinfoBase::insert_entry(const sql::repinfo::Cache& entry)
{
    auto stm = conn.odbcstatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (?, ?, ?, ?, ?, ?)
    )");

    stm->bind_in(1, entry.id);
    stm->bind_in(2, entry.new_memo.c_str());
    stm->bind_in(3, entry.new_desc.c_str());
    stm->bind_in(4, entry.new_prio);
    stm->bind_in(5, entry.new_descriptor.c_str());
    stm->bind_in(6, entry.new_tablea);
    stm->execute_and_close();
}

void ODBCRepinfoBase::dump(FILE* out)
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


ODBCRepinfoV6::ODBCRepinfoV6(ODBCConnection& conn) : ODBCRepinfoBase(conn) {}

int ODBCRepinfoV6::id_use_count(unsigned id, const char* name)
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
