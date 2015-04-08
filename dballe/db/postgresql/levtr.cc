/*
 * db/v6/lev_tr - lev_tr table management
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

#include "levtr.h"
#include "dballe/db/postgresql/internals.h"
#include "dballe/core/defs.h"
#include "dballe/core/record.h"
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
namespace postgresql {

namespace {

Level to_level(const postgresql::Result& res, unsigned row, int first_id)
{
    return Level(
            res.get_int4(row, first_id),
            res.get_int4(row, first_id + 1),
            res.get_int4(row, first_id + 2),
            res.get_int4(row, first_id + 3));
}

Trange to_trange(const postgresql::Result& res, unsigned row, int first_id)
{
    return Trange(
            res.get_int4(row, first_id),
            res.get_int4(row, first_id + 1),
            res.get_int4(row, first_id + 2));
}

}

PostgreSQLLevTrV6::PostgreSQLLevTrV6(PostgreSQLConnection& conn)
    : conn(conn)
{
    conn.prepare("v6_levtr_select_id", R"(
        SELECT id FROM lev_tr WHERE
             ltype1=$1::int4 AND l1=$2::int4 AND ltype2=$3::int4 AND l2=$4::int4
         AND ptype=$5::int4 AND p1=$6::int4 AND p2=$7::int4
    )");
    conn.prepare("v6_levtr_select_data", "SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=$1::int4");
    conn.prepare("v6_levtr_insert", R"(
        INSERT INTO lev_tr (id, ltype1, l1, ltype2, l2, ptype, p1, p2)
             VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::int4, $5::int4, $6::int4, $7::int4)
          RETURNING id
    )");
}

PostgreSQLLevTrV6::~PostgreSQLLevTrV6()
{
}

int PostgreSQLLevTrV6::obtain_id(const Level& lev, const Trange& tr)
{
    using namespace postgresql;
    Result res = conn.exec_prepared("v6_levtr_select_id",
            lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2);
    switch (res.rowcount())
    {
        case 0: break;
        case 1:
            // If there is an existing record, use its ID and don't do an INSERT
            return res.get_int4(0, 0);
        default: error_consistency::throwf("select levtr ID query returned %u results", res.rowcount());

    }

    return conn.exec_prepared_one_row("v6_levtr_insert",
            lev.ltype1, lev.l1, lev.ltype2, lev.l2, tr.pind, tr.p1, tr.p2).get_int4(0, 0);
}

int PostgreSQLLevTrV6::obtain_id(const Record& rec)
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

const sql::LevTr::DBRow* PostgreSQLLevTrV6::read(int id)
{
    auto res = conn.exec_prepared("v6_levtr_select_data", id);
    switch (res.rowcount())
    {
        case 0: return nullptr;
        case 1:
            working_row.id = id;
            working_row.ltype1 = res.get_int4(0, 0);
            working_row.l1 = res.get_int4(0, 1);
            working_row.ltype2 = res.get_int4(0, 2);
            working_row.l2 = res.get_int4(0, 3);
            working_row.pind = res.get_int4(0, 4);
            working_row.p1 = res.get_int4(0, 5);
            working_row.p2 = res.get_int4(0, 6);
            return &working_row;
        default: error_consistency::throwf("select levtr data query returned %u results", res.rowcount());
    }
}

void PostgreSQLLevTrV6::read_all(std::function<void(const LevTr::DBRow&)> dest)
{
    auto res = conn.exec("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        working_row.id = res.get_int4(row, 0);
        working_row.ltype1 = res.get_int4(row, 1);
        working_row.l1 = res.get_int4(row, 2);
        working_row.ltype2 = res.get_int4(row, 3);
        working_row.l2 = res.get_int4(row, 4);
        working_row.pind = res.get_int4(row, 5);
        working_row.p1 = res.get_int4(row, 6);
        working_row.p2 = res.get_int4(row, 7);
        dest(working_row);
    }
}

void PostgreSQLLevTrV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    auto res = conn.exec("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY ID");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        fprintf(out, " %4d ", res.get_int4(row, 0));
        {
            stringstream str;
            str << to_level(res, row, 1);
            fprintf(out, "%-20s ", str.str().c_str());
        }
        {
            stringstream str;
            str << to_trange(res, row, 5);
            fprintf(out, "%-10s\n", str.str().c_str());
        }
        ++count;
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
