/*
 * db/mysql/lev_tr - lev_tr table management
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "levtr.h"
#include "dballe/core/defs.h"
#include "dballe/core/record.h"
#include "dballe/db/querybuf.h"
#include "dballe/msg/msg.h"
#include <map>
#include <sstream>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace mysql {

namespace {

Level to_level(mysql::Row& row, int first_id=0)
{
    return Level(
            row.as_int(first_id),
            row.as_int(first_id + 1),
            row.as_int(first_id + 2),
            row.as_int(first_id + 3));
}

Trange to_trange(mysql::Row& row, int first_id=0)
{
    return Trange(
            row.as_int(first_id),
            row.as_int(first_id + 1),
            row.as_int(first_id + 2));
}

}

MySQLLevTrV6::MySQLLevTrV6(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLLevTrV6::~MySQLLevTrV6()
{
}

int MySQLLevTrV6::obtain_id(const Level& lev, const Trange& tr)
{
    // Try select first
    Querybuf select;
    select.appendf(R"(
        SELECT id FROM lev_tr WHERE
               ltype1=%d AND l1=%d AND ltype2=%d AND l2=%d
           AND ptype=%d AND p1=%d AND p2=%d
    )", lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2);
    auto res = conn.exec_store(select);
    switch (res.rowcount())
    {
        case 0: break;
        case 1: return res.fetch().as_int(0);
        default: error_consistency::throwf("select station ID query returned %u results", res.rowcount());
    }

    // If not found, do an update
    Querybuf insert;
    insert.appendf(
            "INSERT INTO lev_tr (ltype1, l1, ltype2, l2, ptype, p1, p2) VALUES (%d, %d, %d, %d, %d, %d, %d)",
            lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2);
    conn.exec_no_data(insert);
    return conn.get_last_insert_id();
}

int MySQLLevTrV6::obtain_id(const Record& rec)
{
    Level lev;
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE1))
        lev.ltype1 = var->enqi();
    else
        lev.ltype1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L1))
        lev.l1 = var->enqi();
    else
        lev.l1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE2))
        lev.ltype2 = var->enqi();
    else
        lev.ltype2 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L2))
        lev.l2 = var->enqi();
    else
        lev.l2 = MISSING_INT;

    Trange tr;
    if (const Var* var = rec.key_peek(DBA_KEY_PINDICATOR))
        tr.pind = var->enqi();
    else
        tr.pind = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P1))
        tr.p1 = var->enqi();
    else
        tr.p1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P2))
        tr.p2 = var->enqi();
    else
        tr.p2 = MISSING_INT;

    return obtain_id(lev, tr);
}

const sql::LevTr::DBRow* MySQLLevTrV6::read(int id)
{
    Querybuf select;
    select.appendf("SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=%d", id);
    auto res = conn.exec_store(select);
    bool found = false;
    while (auto row = res.fetch())
    {
        working_row.id = id;
        working_row.ltype1 = row.as_int(0);
        working_row.l1 = row.as_int(1);
        working_row.ltype2 = row.as_int(2);
        working_row.l2 = row.as_int(3);
        working_row.pind = row.as_int(4);
        working_row.p1 = row.as_int(5);
        working_row.p2 = row.as_int(6);
        found = true;
    }
    if (!found) return nullptr;
    return &working_row;
}

void MySQLLevTrV6::read_all(std::function<void(const LevTr::DBRow&)> dest)
{
    auto res = conn.exec_store("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
    while (auto row = res.fetch())
    {
        working_row.id = row.as_int(0);
        working_row.ltype1 = row.as_int(1);
        working_row.l1 = row.as_int(2);
        working_row.ltype2 = row.as_int(3);
        working_row.l2 = row.as_int(4);
        working_row.pind = row.as_int(5);
        working_row.p1 = row.as_int(6);
        working_row.p2 = row.as_int(7);
        dest(working_row);
    }
}

void MySQLLevTrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    auto res = conn.exec_store("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY ID");
    while (auto row = res.fetch())
    {
        fprintf(out, " %4d ", row.as_int(0));
        {
            stringstream str;
            str << to_level(row, 1);
            fprintf(out, "%-20s ", str.str().c_str());
        }
        {
            stringstream str;
            str << to_trange(row, 5);
            fprintf(out, "%-10s\n", str.str().c_str());
        }
        ++count;
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
