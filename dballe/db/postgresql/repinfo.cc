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
namespace postgresql {

PostgreSQLRepinfoV5::PostgreSQLRepinfoV5(PostgreSQLConnection& conn)
    : Repinfo(conn), conn(conn)
{
    read_cache();
}

PostgreSQLRepinfoV5::~PostgreSQLRepinfoV5()
{
}

void PostgreSQLRepinfoV5::read_cache()
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

void PostgreSQLRepinfoV5::insert_auto_entry(const char* memo)
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

int PostgreSQLRepinfoV5::id_use_count(unsigned id, const char* name)
{
    Querybuf query(500);
    query.appendf("SELECT COUNT(1) FROM context WHERE id_report=%u", id);
    return conn.exec_one_row(query).get_int4(0, 0);
}

void PostgreSQLRepinfoV5::delete_entry(unsigned id)
{
    conn.exec_no_data("DELETE FROM repinfo WHERE id=$1::int4", (int32_t)id);
}

void PostgreSQLRepinfoV5::update_entry(const sql::repinfo::Cache& entry)
{
    conn.exec_no_data(R"(
        UPDATE repinfo SET memo=$2::text, description=$3::text, prio=$4::int4, descriptor=$5::text, tablea=$6::int4
         WHERE id=$1::int4
    )", (int32_t)entry.id,
        entry.new_memo,
        entry.new_desc,
        (int32_t)entry.new_prio,
        entry.new_descriptor,
        (int32_t)entry.new_tablea);
}

void PostgreSQLRepinfoV5::insert_entry(const sql::repinfo::Cache& entry)
{
    conn.exec_no_data(R"(
        INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)
             VALUES ($1::int4, $2::text, $3::text, $4::int4, $5::text, $6::int4)
    )", (int32_t)entry.id,
        entry.new_memo,
        entry.new_desc,
        (int32_t)entry.new_prio,
        entry.new_descriptor,
        (int32_t)entry.new_tablea);
}

void PostgreSQLRepinfoV5::dump(FILE* out)
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

PostgreSQLRepinfoV6::PostgreSQLRepinfoV6(PostgreSQLConnection& conn) : PostgreSQLRepinfoV5(conn) {}

int PostgreSQLRepinfoV6::id_use_count(unsigned id, const char* name)
{
    Querybuf query(500);
    query.appendf("SELECT COUNT(1) FROM data WHERE id_report=%u", id);
    return conn.exec_one_row(query).get_int4(0, 0);
}

}

}
}
