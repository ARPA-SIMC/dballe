/*
 * db/station - station table management
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

#include "station.h"
#include "dballe/db/internals.h"
#include "db.h"

#include <cstring>
#include <sql.h>

using namespace wreport;

namespace dballe {
namespace db {

Station::Station(DB& db)
    : db(db), sfstm(0), smstm(0), sstm(0), istm(0), ustm(0), dstm(0)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident=?";
    const char* select_query =
        "SELECT lat, lon, ident FROM station WHERE id=?";
    const char* insert_query =
        "INSERT INTO station (lat, lon, ident)"
        " VALUES (?, ?, ?);";
    const char* update_query =
        "UPDATE station SET lat=?, lon=?, ident=? WHERE id=?";
    const char* remove_query =
        "DELETE FROM station WHERE id=?";

    /* Override queries for some databases */
    switch (db.conn->server_type)
    {
        case ORACLE:
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (seq_station.NextVal, ?, ?, ?)";
            break;
        case POSTGRES:
            insert_query = "INSERT INTO station (id, lat, lon, ident) VALUES (nextval('seq_station'), ?, ?, ?)";
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sfstm = new Statement(*db.conn);
    sfstm->bind_in(1, lat);
    sfstm->bind_in(2, lon);
    sfstm->bind_out(1, id);
    sfstm->prepare(select_fixed_query);

    /* Create the statement for select mobile */
    smstm = new Statement(*db.conn);
    smstm->bind_in(1, lat);
    smstm->bind_in(2, lon);
    smstm->bind_in(3, ident, ident_ind);
    smstm->bind_out(1, id);
    smstm->prepare(select_mobile_query);

    /* Create the statement for select station data */
    sstm = new Statement(*db.conn);
    sstm->bind_in(1, id);
    sstm->bind_out(1, lat);
    sstm->bind_out(2, lon);
    sstm->bind_out(3, ident, sizeof(ident), ident_ind);
    sstm->prepare(select_query);

    /* Create the statement for insert */
    istm = new Statement(*db.conn);
    istm->bind_in(1, lat);
    istm->bind_in(2, lon);
    istm->bind_in(3, ident, ident_ind);
    istm->prepare(insert_query);

    /* Create the statement for update */
    ustm = new Statement(*db.conn);
    ustm->bind_in(1, lat);
    ustm->bind_in(2, lon);
    ustm->bind_in(3, ident, ident_ind);
    ustm->bind_in(4, id);
    ustm->prepare(update_query);

    /* Create the statement for remove */
    dstm = new Statement(*db.conn);
    dstm->bind_in(1, id);
    dstm->prepare(remove_query);
}

Station::~Station()
{
    if (sfstm) delete sfstm;
    if (smstm) delete smstm;
    if (sstm) delete sstm;
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (dstm) delete dstm;
}

void Station::set_ident(const char* val)
{
    if (val)
    {
        int len = strlen(val);
        if (len > 64) len = 64;
        memcpy(ident, val, len);
        ident[len] = 0;
        ident_ind = len; 
    } else {
        ident[0] = 0;
        ident_ind = SQL_NULL_DATA; 
    }
}

int Station::get_id()
{
    db::Statement* stm = ident_ind == SQL_NULL_DATA ? sfstm : smstm;
    stm->execute();
    int res;
    if (stm->fetch_expecting_one())
        res = id;
    else
        res = -1;
    return res;
}

void Station::get_data(int qid)
{
    id = qid;
    sstm->execute();
    if (!sstm->fetch_expecting_one())
        error_notfound::throwf("looking for information for station id %d", qid);
    if (ident_ind == SQL_NULL_DATA)
        ident[0] = 0;
}

int Station::insert()
{
    istm->execute_and_close();
    return db.last_station_insert_id();
}

void Station::update()
{
    ustm->execute_and_close();
}

void Station::remove()
{
    dstm->execute_and_close();
}

void Station::dump(FILE* out)
{
    DBALLE_SQL_C_SINT_TYPE id;
    DBALLE_SQL_C_SINT_TYPE lat;
    DBALLE_SQL_C_SINT_TYPE lon;
    char ident[64];
    SQLLEN ident_ind;

    Statement stm(*db.conn);
    stm.bind_out(1, id);
    stm.bind_out(2, lat);
    stm.bind_out(3, lon);
    stm.bind_out(4, ident, 64, ident_ind);
    stm.exec_direct("SELECT id, lat, lon, ident FROM station");
    int count;
    fprintf(out, "dump of table station:\n");
    for (count = 0; stm.fetch(); ++count)
        if (ident_ind == SQL_NULL_DATA)
            fprintf(out, " %d, %.5f, %.5f\n", (int)id, lat/100000.0, lon/100000.0);
        else
            fprintf(out, " %d, %.5f, %.5f, %.*s\n", (int)id, lat/10000.0, lon/10000.0, (int)ident_ind, ident);
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
    stm.close_cursor();
}

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
