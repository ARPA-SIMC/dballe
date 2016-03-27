#include "station.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/core/var.h"
#include "dballe/record.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v6 {
namespace mysql {

MySQLStationBase::MySQLStationBase(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLStationBase::~MySQLStationBase()
{
}

bool MySQLStationBase::maybe_get_id(int lat, int lon, const char* ident, int* id)
{
    Querybuf qb;
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

int MySQLStationBase::get_id(int lat, int lon, const char* ident)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
        return id;
    throw error_notfound("station not found in the database");
}

int MySQLStationBase::obtain_id(int lat, int lon, const char* ident, bool* inserted)
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

void MySQLStationBase::read_station_vars(const std::string& query, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;
    auto res = conn.exec_store(query);
    while (auto row = res.fetch())
    {
        Varcode code = row.as_int(0);
        TRACE("fill_ana_layer Got %01d%02ld%03ld %s\n", WR_VAR_FXY(code), row.as_cstring(1));

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
            var->seta(newvar(row.as_int(2), row.as_cstring(3)));
        }
    }

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void MySQLStationBase::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
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

void MySQLStationBase::dump(FILE* out)
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

void MySQLStationBase::add_station_vars(int id_station, Record& rec)
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
        rec.set(newvar((Varcode)row.as_int(0), row.as_cstring(1)));
}

MySQLStationV6::MySQLStationV6(MySQLConnection& conn)
    : MySQLStationBase(conn) {}

}
}
}
}
