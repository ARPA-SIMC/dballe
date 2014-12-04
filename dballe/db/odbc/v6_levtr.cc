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

#include "v6_levtr.h"
#include "dballe/db/odbc/internals.h"
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
namespace v6 {

ODBCLevTr::ODBCLevTr(ODBCConnection& conn)
    : conn(conn)
{
    const char* select_query =
        "SELECT id FROM lev_tr WHERE"
        "     ltype1=? AND l1=? AND ltype2=? AND l2=?"
        " AND ptype=? AND p1=? AND p2=?";
    const char* select_data_query =
        "SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=?";
    const char* insert_query =
        "INSERT INTO lev_tr (ltype1, l1, ltype2, l2, ptype, p1, p2) VALUES (?, ?, ?, ?, ?, ?, ?)";
    const char* remove_query =
        "DELETE FROM lev_tr WHERE id=?";

    /* Override queries for some databases */
    switch (conn.server_type)
    {
        case ServerType::ORACLE:
            insert_query = "INSERT INTO lev_tr VALUES (seq_lev_tr.NextVal, ?, ?, ?, ?, ?, ?, ?)";
            seq_lev_tr = new Sequence(conn, "seq_lev_tr");
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sstm = conn.odbcstatement().release();
    sstm->bind_in(1, ltype1);
    sstm->bind_in(2, l1);
    sstm->bind_in(3, ltype2);
    sstm->bind_in(4, l2);
    sstm->bind_in(5, pind);
    sstm->bind_in(6, p1);
    sstm->bind_in(7, p2);
    sstm->bind_out(1, id);
    sstm->prepare(select_query);

    /* Create the statement for select data */
    sdstm = conn.odbcstatement().release();
    sdstm->bind_in(1, id);
    sdstm->bind_out(1, ltype1);
    sdstm->bind_out(2, l1);
    sdstm->bind_out(3, ltype2);
    sdstm->bind_out(4, l2);
    sdstm->bind_out(5, pind);
    sdstm->bind_out(6, p1);
    sdstm->bind_out(7, p2);
    sdstm->prepare(select_data_query);

    /* Create the statement for insert */
    istm = conn.odbcstatement().release();
    istm->bind_in(1, ltype1);
    istm->bind_in(2, l1);
    istm->bind_in(3, ltype2);
    istm->bind_in(4, l2);
    istm->bind_in(5, pind);
    istm->bind_in(6, p1);
    istm->bind_in(7, p2);
    istm->prepare(insert_query);

    /* Create the statement for remove */
    dstm = conn.odbcstatement().release();
    dstm->bind_in(1, id);
    dstm->prepare(remove_query);
}

ODBCLevTr::~ODBCLevTr()
{
    delete seq_lev_tr;
    if (sstm) delete sstm;
    if (sdstm) delete sdstm;
    if (istm) delete istm;
    if (dstm) delete dstm;
}

int ODBCLevTr::get_id()
{
    sstm->execute();

    /* Get the result */
    int res;
    if (sstm->fetch_expecting_one())
        res = id;
    else
        res = -1;

    return res;
}

void ODBCLevTr::get_data(int qid)
{
    id = qid;
    sdstm->execute();
    if (!sdstm->fetch_expecting_one())
        error_notfound::throwf("no data found for lev_tr id %d", qid);
}

int ODBCLevTr::obtain_id(const Level& lev, const Trange& tr)
{
    ltype1 = lev.ltype1;
    l1 = lev.l1;
    ltype2 = lev.ltype2;
    l2 = lev.l2;
    pind = tr.pind;
    p1 = tr.p1;
    p2 = tr.p2;

    // Check for an existing lev_tr with these data
    int id = get_id();
    // If there is an existing record, use its ID and don't do an INSERT
    if (id != -1) return id;
    return insert();
}

int ODBCLevTr::obtain_id(const Record& rec)
{
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE1))
        ltype1 = var->enqi();
    else
        ltype1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L1))
        l1 = var->enqi();
    else
        l1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE2))
        ltype2 = var->enqi();
    else
        ltype2 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L2))
        l2 = var->enqi();
    else
        l2 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_PINDICATOR))
        pind = var->enqi();
    else
        pind = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P1))
        p1 = var->enqi();
    else
        p1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P2))
        p2 = var->enqi();
    else
        p2 = MISSING_INT;

    // Check for an existing lev_tr with these data
    int id = get_id();
    // If there is an existing record, use its ID and don't do an INSERT
    if (id != -1) return id;
    return insert();
}

int ODBCLevTr::insert()
{
    istm->execute_and_close();

    if (seq_lev_tr)
        return seq_lev_tr->read();
    else
        return conn.get_last_insert_id();
}

void ODBCLevTr::remove()
{
    dstm->execute_and_close();
}

void ODBCLevTr::dump(FILE* out)
{
    int id;
    int ltype1;
    int l1;
    int ltype2;
    int l2;
    int pind;
    int p1;
    int p2;

    auto stm = conn.odbcstatement();
    stm->bind_out(1, id);
    stm->bind_out(2, ltype1);
    stm->bind_out(3, l1);
    stm->bind_out(4, ltype2);
    stm->bind_out(5, l2);
    stm->bind_out(6, pind);
    stm->bind_out(7, p1);
    stm->bind_out(8, p2);
    stm->exec_direct("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY id");
    int count;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    for (count = 0; stm->fetch(); ++count)
    {
        fprintf(out, " %4d ", (int)id);
        {
            stringstream str;
            str << Level(ltype1, l1, ltype2, l2);
            fprintf(out, "%-20s ", str.str().c_str());
        }
        {
            stringstream str;
            str << Trange(pind, p1, p2);
            fprintf(out, "%-10s\n", str.str().c_str());
        }
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}

}
}
}
