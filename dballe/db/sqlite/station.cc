/*
 * db/station - station table management
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

#include "station.h"
#include "dballe/db/sqlite/internals.h"
#include "dballe/core/var.h"
#include "dballe/core/record.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteStationV5::SQLiteStationV5(SQLiteConnection& conn)
    : conn(conn)
{
    const char* select_fixed_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident IS NULL";
    const char* select_mobile_query =
        "SELECT id FROM station WHERE lat=? AND lon=? AND ident=?";
    const char* insert_query =
        "INSERT INTO station (lat, lon, ident)"
        " VALUES (?, ?, ?);";

    // Create the statement for select fixed
    sfstm = conn.sqlitestatement(select_fixed_query).release();

    // Create the statement for select mobile
    smstm = conn.sqlitestatement(select_mobile_query).release();

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();
}

SQLiteStationV5::~SQLiteStationV5()
{
    delete sfstm;
    delete smstm;
    delete istm;
}

bool SQLiteStationV5::maybe_get_id(int lat, int lon, const char* ident, int* id)
{
    SQLiteStatement* s;
    if (ident)
    {
        smstm->bind_val(1, lat);
        smstm->bind_val(2, lon);
        smstm->bind_val(3, ident);
        s = smstm;
    } else {
        sfstm->bind_val(1, lat);
        sfstm->bind_val(2, lon);
        s = sfstm;
    }
    bool found = false;
    s->execute([&]() {
        found = true;
        *id = s->column_int(0);
    });
    return found;
}

int SQLiteStationV5::get_id(int lat, int lon, const char* ident)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
        return id;
    throw error_notfound("station not found in the database");
}

int SQLiteStationV5::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
    {
        if (inserted) *inserted = false;
        return id;
    }

    // If no station was found, insert a new one
    istm->bind_val(1, lat);
    istm->bind_val(2, lon);
    if (ident)
        istm->bind_val(3, ident);
    else
        istm->bind_null_val(3);
    istm->execute();
    if (inserted) *inserted = true;
    return conn.get_last_insert_id();
}

void SQLiteStationV5::read_station_vars(SQLiteStatement& stm, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;

    stm.execute([&]() {
        Varcode code = stm.column_int(0);
        TRACE("fill_ana_layer Got B%02ld%03ld %s\n", WR_VAR_X(code), WR_VAR_Y(code), out_value);

        // First process the variable, possibly inserting the old one in the message
        if (last_varcode != code)
        {
            TRACE("fill_ana_layer new var\n");
            if (var.get())
            {
                TRACE("fill_ana_layer inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                dest(move(var));
            }
            var = newvar(code, stm.column_string(1).c_str());
            last_varcode = code;
        }

        if (!stm.column_isnull(2))
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(ap_newvar(stm.column_int(2), stm.column_string(3).c_str()));
        }
    });

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void SQLiteStationV5::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] =
        "SELECT d.id_var, d.value, a.type, a.value"
        "  FROM context c, data d"
        "  LEFT JOIN attr a ON a.id_context = d.id_context AND a.id_var = d.id_var"
        " WHERE d.id_context = c.id AND c.id_ana = ? AND c.id_report = ?"
        "   AND c.datetime = {ts '1000-01-01 00:00:00.000'}"
        " ORDER BY d.id_var, a.type";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station, id_report);
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);

    read_station_vars(*stm, dest);
}

void SQLiteStationV5::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    auto stm = conn.sqlitestatement("SELECT id, lat, lon, ident FROM station");
    stm->execute([&]() {
        fprintf(out, " %d, %.5f, %.5f",
                stm->column_int(0),
                stm->column_int(1) / 100000.0,
                stm->column_int(2) / 100000.0);
        if (stm->column_isnull(3))
            putc('\n', out);
        else
        {
            string ident = stm->column_string(3);
            fprintf(out, ", %.*s\n", ident.size(), ident.data());
        }
        ++count;
    });
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

void SQLiteStationV5::add_station_vars(int id_station, Record& rec)
{
    /* Extra variables to add:
     *
     * HEIGHT,      B07001  1793
     * HEIGHT_BARO, B07031  1823
     * ST_NAME,     B01019   275
     * BLOCK,       B01001   257
     * STATION,     B01002   258
    */
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM context c, data d, repinfo ri
         WHERE c.id = d.id_context AND ri.id = c.id_report AND c.id_ana=?
           AND c.datetime='1000-01-01 00:00:00.000'
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN context sc ON sri.id=sc.id_report
            JOIN data sd ON sc.id=sd.id_context
          WHERE sc.id_ana=c.id_ana
            AND sc.ltype1=c.ltype1 AND sc.l1=c.l1 AND sc.ltype2=c.ltype2 AND sc.l2=c.l2
            AND sc.ptype=c.ptype AND sc.p1=c.p1 AND sc.p2=c.p2
            AND sc.datetime=c.datetime AND sd.id_var=d.id_var)
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    stm->execute([&]() {
        rec.var(stm->column_int(0)).setc(stm->column_string(1).c_str());
    });
}

SQLiteStationV6::SQLiteStationV6(SQLiteConnection& conn)
    : SQLiteStationV5(conn) {}

void SQLiteStationV6::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    static const char query[] = R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM data d
          LEFT JOIN attr a ON a.id_data = d.id
         WHERE d.id_station=? AND d.id_report=?
           AND d.id_lev_tr = -1
         ORDER BY d.id_var, a.type
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station, id_report);
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);

    read_station_vars(*stm, dest);
}

void SQLiteStationV6::add_station_vars(int id_station, Record& rec)
{
    const char* query = R"(
        SELECT d.id_var, d.value
          FROM data d, repinfo ri
         WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = ?
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN data sd ON sri.id=sd.id_report
          WHERE sd.id_station=d.id_station AND sd.id_lev_tr = -1
            AND sd.id_var=d.id_var)
    )";

    auto stm = conn.sqlitestatement(query);
    stm->bind(id_station);
    stm->execute([&]() {
        rec.var(stm->column_int(0)).setc(stm->column_string(1).c_str());
    });
}

}
}
}
