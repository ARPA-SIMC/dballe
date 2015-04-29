/*
 * db/sqlite/repinfo - repinfo table management
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
#include "dballe/db/sqlite/internals.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteRepinfoBase::SQLiteRepinfoBase(SQLiteConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

SQLiteRepinfoBase::~SQLiteRepinfoBase()
{
}

void SQLiteRepinfoBase::read_cache()
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

void SQLiteRepinfoBase::insert_auto_entry(const char* memo)
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

int SQLiteRepinfoBase::id_use_count(unsigned id, const char* name)
{
    unsigned count = 0;
    auto stm = conn.sqlitestatement("SELECT COUNT(1) FROM context WHERE id_report=?");
    stm->bind(id);
    stm->execute_one([&]() {
        count = stm->column_int(0);
    });
    return count;
}

void SQLiteRepinfoBase::delete_entry(unsigned id)
{
    auto stm = conn.sqlitestatement("DELETE FROM repinfo WHERE id=?");
    stm->bind(id);
    stm->execute();
}

void SQLiteRepinfoBase::update_entry(const sql::repinfo::Cache& entry)
{
    auto stm = conn.sqlitestatement(R"(
        UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?
         WHERE id=?
    )");
    stm->bind(
            entry.new_memo,
            entry.new_desc,
            entry.new_prio,
            entry.new_descriptor.c_str(),
            entry.new_tablea,
            entry.id);
    stm->execute();
}

void SQLiteRepinfoBase::insert_entry(const sql::repinfo::Cache& entry)
{
    auto stm = conn.sqlitestatement(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES (?, ?, ?, ?, ?, ?)
    )");
    stm->bind(
        entry.id,
        entry.new_memo,
        entry.new_desc,
        entry.new_prio,
        entry.new_descriptor,
        entry.new_tablea);
    stm->execute();
}

void SQLiteRepinfoBase::dump(FILE* out)
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

SQLiteRepinfoV6::SQLiteRepinfoV6(SQLiteConnection& conn) : SQLiteRepinfoBase(conn) {}

int SQLiteRepinfoV6::id_use_count(unsigned id, const char* name)
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
