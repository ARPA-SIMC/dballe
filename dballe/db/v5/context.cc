/*
 * db/context - context table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "context.h"
#include "dballe/db/internals.h"
#include "db.h"
#include <dballe/core/defs.h>

#include <sstream>
#include <cstring>
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

Context::Context(DB& db)
    : db(db), sstm(0), sdstm(0), istm(0), dstm(0)
{
    const char* select_query =
        "SELECT id FROM context WHERE id_ana=? AND id_report=? AND datetime=?"
        " AND ltype1=? AND l1=? AND ltype2=? AND l2=?"
        " AND ptype=? AND p1=? AND p2=?";
    const char* select_data_query =
        "SELECT id_ana, id_report, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM context WHERE id=?";
    const char* insert_query =
        "INSERT INTO context VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    const char* remove_query =
        "DELETE FROM context WHERE id=?";

    /* Override queries for some databases */
    switch (db.conn->server_type)
    {
        case ORACLE:
            insert_query = "INSERT INTO context VALUES (seq_context.NextVal, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            break;
        case POSTGRES:
            insert_query = "INSERT INTO context VALUES (nextval('seq_context'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            break;
        default: break;
    }

    date.fraction = 0;

    /* Create the statement for select fixed */
    sstm = new Statement(*db.conn);
    sstm->bind_in(1, id_station);
    sstm->bind_in(2, id_report);
    sstm->bind_in(3, date);
    sstm->bind_in(4, ltype1);
    sstm->bind_in(5, l1);
    sstm->bind_in(6, ltype2);
    sstm->bind_in(7, l2);
    sstm->bind_in(8, pind);
    sstm->bind_in(9, p1);
    sstm->bind_in(10, p2);
    sstm->bind_out(1, id);
    sstm->prepare(select_query);

    /* Create the statement for select data */
    sdstm = new Statement(*db.conn);
    sdstm->bind_in(1, id);
    sdstm->bind_out(1, id_station);
    sdstm->bind_out(2, id_report);
    sdstm->bind_out(3, date);
    sdstm->bind_out(4, ltype1);
    sdstm->bind_out(5, l1);
    sdstm->bind_out(6, ltype2);
    sdstm->bind_out(7, l2);
    sdstm->bind_out(8, pind);
    sdstm->bind_out(9, p1);
    sdstm->bind_out(10, p2);
    sdstm->prepare(select_data_query);

    /* Create the statement for insert */
    istm = new Statement(*db.conn);
    istm->bind_in(1, id_station);
    istm->bind_in(2, id_report);
    istm->bind_in(3, date);
    istm->bind_in(4, ltype1);
    istm->bind_in(5, l1);
    istm->bind_in(6, ltype2);
    istm->bind_in(7, l2);
    istm->bind_in(8, pind);
    istm->bind_in(9, p1);
    istm->bind_in(10, p2);
    istm->prepare(insert_query);

    /* Create the statement for remove */
    dstm = new Statement(*db.conn);
    dstm->bind_in(1, id);
    dstm->prepare(remove_query);
}

Context::~Context()
{
    if (sstm) delete sstm;
    if (sdstm) delete sdstm;
    if (istm) delete istm;
    if (dstm) delete dstm;
}

int Context::get_id()
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

void Context::get_data(int qid)
{
    id = qid;
    sdstm->execute();
    if (!sdstm->fetch_expecting_one())
        error_notfound::throwf("no data found for context id %d", qid);
}

int Context::obtain_station_info()
{
    /* Fill up the query parameters with the data for the anagraphical context */
    date.year = 1000;
    date.month = 1;
    date.day = 1;
    date.hour = date.minute = date.second = 0;
    ltype1 = 257;
    l1 = ltype2 = l2 = MISSING_INT;
    pind = p1 = p2 = MISSING_INT;

    /* See if the context entry already exists */
    int res = get_id();

    /* If it doesn't exist yet, we create it */
    if (res == -1)
        res = insert();

    return res;
}

int Context::insert()
{
    istm->execute_and_close();
    return db.last_context_insert_id();
}

void Context::remove()
{
    dstm->execute_and_close();
}

void Context::dump(FILE* out)
{
    DBALLE_SQL_C_SINT_TYPE id;
    DBALLE_SQL_C_SINT_TYPE id_station;
    DBALLE_SQL_C_SINT_TYPE id_report;
    SQL_TIMESTAMP_STRUCT date;
    DBALLE_SQL_C_SINT_TYPE ltype1;
    DBALLE_SQL_C_SINT_TYPE l1;
    DBALLE_SQL_C_SINT_TYPE ltype2;
    DBALLE_SQL_C_SINT_TYPE l2;
    DBALLE_SQL_C_SINT_TYPE pind;
    DBALLE_SQL_C_SINT_TYPE p1;
    DBALLE_SQL_C_SINT_TYPE p2;

    Statement stm(*db.conn);
    stm.bind_out(1, id);
    stm.bind_out(2, id_station);
    stm.bind_out(3, id_report);
    stm.bind_out(4, date);
    stm.bind_out(5, ltype1);
    stm.bind_out(6, l1);
    stm.bind_out(7, ltype2);
    stm.bind_out(8, l2);
    stm.bind_out(9, pind);
    stm.bind_out(10, p1);
    stm.bind_out(11, p2);
    stm.exec_direct("SELECT id, id_ana, id_report, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM context ORDER BY id");
    int count;
    fprintf(out, "   id   st  rep date        lev              tr\n");
    for (count = 0; stm.fetch(); ++count)
    {
        fprintf(out, " %4d %4d %4d %04d-%02d-%02d %02d:%02d:%02d ",
            (int)id, (int)id_station, (int)id_report,
            (int)date.year, (int)date.month, (int)date.day,
            (int)date.hour, (int)date.minute, (int)date.second);
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
    fprintf(out, "%d element%s in table context\n", count, count != 1 ? "s" : "");
}

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
