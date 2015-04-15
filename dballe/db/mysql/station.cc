/*
 * db/mysql/station - station table management
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

#include "station.h"
#include "dballe/db/mysql/internals.h"
#include "dballe/db/querybuf.h"
#include "dballe/core/var.h"
#include "dballe/core/record.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace mysql {

MySQLStationV5::MySQLStationV5(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLStationV5::~MySQLStationV5()
{
}

bool MySQLStationV5::maybe_get_id(int lat, int lon, const char* ident, int* id)
{
    Querybuf qb;
    MySQLStatement* s;
    if (ident)
    {
        string escaped_ident = conn.escape(ident);
        qb.appendf("SELECT id FROM station WHERE lat=%d AND lon=%d AND ident='%s'",
                lat, lon, escaped_ident.c_str());
    } else {
        qb.appendf("SELECT id FROM station WHERE lat=%d AND lon=%d AND ident IS NULL", lat, lon);
    }
    auto res = conn.exec_store(qb);
    switch (res.rowcount())
    {
        case 0:
            return false;
        case 1:
            *id = res.fetch().as_int(0);
            return true;
        default:
            error_consistency::throwf("select station ID query returned %u results", res.rowcount());
    }
}

int MySQLStationV5::get_id(int lat, int lon, const char* ident)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
        return id;
    throw error_notfound("station not found in the database");
}

int MySQLStationV5::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    // Try select first
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
    {
        if (inserted) *inserted = false;
        return id;
    }

    // Not found, do an insert
    Querybuf qb;
    if (ident)
    {
        string escaped_ident = conn.escape(ident);
        qb.appendf(R"(
            INSERT INTO station (lat, lon, ident) VALUES (%d, %d, '%s')
        )", lat, lon, escaped_ident.c_str());
    } else {
        qb.appendf(R"(
            INSERT INTO station (lat, lon, ident) VALUES (%d, %d, NULL)
        )", lat, lon);
    }
    conn.exec_no_data(qb);
    if (inserted) *inserted = true;
    return conn.get_last_insert_id();
#if 0
    // See http://mikefenwick.com/blog/insert-into-database-or-return-id-of-duplicate-row-in-mysql/
    //
    // It would be nice to use this, BUT at every failed insert the
    // auto_increment sequence is updated, so every lookup causes a hole in the
    // sequence.
    //
    // Since the auto_increment sequence blocks all inserts once it runs out of
    // numbers, we cannot afford to eat it away for every ID lookup.
    //
    // http://stackoverflow.com/questions/2615417/what-happens-when-auto-increment-on-integer-column-reaches-the-max-value-in-data
    // Just in case there's any question, the AUTO_INCREMENT field /DOES NOT
    // WRAP/. Once you hit the limit for the field size, INSERTs generate an
    // error. (As per Jeremy Cole)

    Querybuf qb;
    if (ident)
    {
        string escaped_ident = conn.escape(ident);
        qb.appendf(R"(
            INSERT INTO station (lat, lon, ident) VALUES (%d, %d, '%s')
           ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)
        )", lat, lon, escaped_ident.c_str());
    } else {
        qb.appendf(R"(
            INSERT INTO station (lat, lon, ident) VALUES (%d, %d, NULL)
           ON DUPLICATE KEY UPDATE id=LAST_INSERT_ID(id)
        )", lat, lon);
    }
    conn.exec_no_data(qb);
    if (inserted) *inserted = mysql_affected_rows(conn) > 0;
    return conn.get_last_insert_id();
#endif
}

void MySQLStationV5::read_station_vars(const std::string& query, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;
    auto res = conn.exec_store(query);
    while (auto row = res.fetch())
    {
        Varcode code = row.as_int(0);
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
            var = newvar(code, row.as_cstring(1));
            last_varcode = code;
        }

        if (!row.isnull(2))
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(ap_newvar(row.as_int(2), row.as_cstring(3)));
        }
    }

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void MySQLStationV5::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    Querybuf query;
    query.appendf(R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM context c, data d
          LEFT JOIN attr a ON a.id_context = d.id_context AND a.id_var = d.id_var
         WHERE d.id_context = c.id AND c.id_ana = %d AND c.id_report = %d
           AND c.datetime = '1000-01-01 00:00:00.000'
         ORDER BY d.id_var, a.type
    )", id_station, id_report);
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query.c_str(), id_station, id_report);
    read_station_vars(query, dest);
}

void MySQLStationV5::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");
    auto res = conn.exec_store("SELECT id, lat, lon, ident FROM station");
    while (auto row = res.fetch())
    {
        fprintf(out, " %d, %.5f, %.5f",
                row.as_int(0),
                row.as_int(1) / 100000.0,
                row.as_int(2) / 100000.0);
        if (row.isnull(3))
            putc('\n', out);
        else
            fprintf(out, ", %s\n", row.as_cstring(3));
        ++count;
    }
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}

void MySQLStationV5::add_station_vars(int id_station, Record& rec)
{
    /* Extra variables to add:
     *
     * HEIGHT,      B07001  1793
     * HEIGHT_BARO, B07031  1823
     * ST_NAME,     B01019   275
     * BLOCK,       B01001   257
     * STATION,     B01002   258
    */
    Querybuf query;
    query.appendf(R"(
        SELECT d.id_var, d.value
          FROM context c, data d, repinfo ri
         WHERE c.id = d.id_context AND ri.id = c.id_report AND c.id_ana=%d
           AND c.datetime='1000-01-01 00:00:00.000'
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN context sc ON sri.id=sc.id_report
            JOIN data sd ON sc.id=sd.id_context
          WHERE sc.id_ana=c.id_ana
            AND sc.ltype1=c.ltype1 AND sc.l1=c.l1 AND sc.ltype2=c.ltype2 AND sc.l2=c.l2
            AND sc.ptype=c.ptype AND sc.p1=c.p1 AND sc.p2=c.p2
            AND sc.datetime=c.datetime AND sd.id_var=d.id_var)
    )", id_station);
    auto res = conn.exec_store(query);
    while (auto row = res.fetch())
        rec.var(row.as_int(0)).setc(row.as_cstring(1));
}

MySQLStationV6::MySQLStationV6(MySQLConnection& conn)
    : MySQLStationV5(conn) {}

void MySQLStationV6::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    Querybuf query;
    query.appendf(R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM data d
          LEFT JOIN attr a ON a.id_data = d.id
         WHERE d.id_station=%d AND d.id_report=%d
           AND d.id_lev_tr = -1
         ORDER BY d.id_var, a.type
    )", id_station, id_report);
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query.c_str(), id_station, id_report);
    read_station_vars(query, dest);
}

void MySQLStationV6::add_station_vars(int id_station, Record& rec)
{
    Querybuf query;
    query.appendf(R"(
        SELECT d.id_var, d.value
          FROM data d, repinfo ri
         WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = %d
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN data sd ON sri.id=sd.id_report
          WHERE sd.id_station=d.id_station AND sd.id_lev_tr = -1
            AND sd.id_var=d.id_var)
    )", id_station);
    auto res = conn.exec_store(query);
    while (auto row = res.fetch())
        rec.var(row.as_int(0)).setc(row.as_cstring(1));
}

}
}
}
