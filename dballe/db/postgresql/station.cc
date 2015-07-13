#include "station.h"
#include "dballe/db/postgresql/internals.h"
#include "dballe/core/var.h"
#include "dballe/record.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;

namespace dballe {
namespace db {
namespace postgresql {

StationBase::StationBase(PostgreSQLConnection& conn)
    : conn(conn)
{
    // Precompile our statements
    conn.prepare("v6_station_select_fixed", "SELECT id FROM station WHERE lat=$1::int4 AND lon=$2::int4 AND ident IS NULL");
    conn.prepare("v6_station_select_mobile", "SELECT id FROM station WHERE lat=$1::int4 AND lon=$2::int4 AND ident=$3::text");
    conn.prepare("v6_station_insert", "INSERT INTO station (id, lat, lon, ident) VALUES (DEFAULT, $1::int4, $2::int4, $3::text) RETURNING id");
}

StationBase::~StationBase()
{
}

bool StationBase::maybe_get_id(int lat, int lon, const char* ident, int* id)
{
    using namespace postgresql;

    Result res;
    if (ident)
        res = move(conn.exec_prepared("v6_station_select_mobile", lat, lon, ident));
    else
        res = move(conn.exec_prepared("v6_station_select_fixed", lat, lon));

    unsigned rows = res.rowcount();
    switch (rows)
    {
        case 0: return false;
        case 1:
            *id = res.get_int4(0, 0);
            return true;
        default: error_consistency::throwf("select station ID query returned %u results", rows);
    }
}

int StationBase::get_id(int lat, int lon, const char* ident)
{
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
        return id;
    throw error_notfound("station not found in the database");
}

int StationBase::obtain_id(int lat, int lon, const char* ident, bool* inserted)
{
    // TODO: lock table
    using namespace postgresql;
    int id;
    if (maybe_get_id(lat, lon, ident, &id))
    {
        if (inserted) *inserted = false;
        return id;
    }

    // If no station was found, insert a new one
    Result res(conn.exec_prepared_one_row("v6_station_insert", lat, lon, ident));
    if (inserted) *inserted = true;
    return res.get_int4(0, 0);
}

void StationBase::get_station_vars(int id_station, int id_report, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    using namespace postgresql;
    TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);
    Result res(conn.exec_prepared("v6_station_get_station_vars", id_station, id_report));

    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;

    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        Varcode code = res.get_int4(row, 0);
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
            var = newvar(code, res.get_string(row, 1));
            last_varcode = code;
        }

        if (!res.is_null(row, 2))
        {
            TRACE("fill_ana_layer new attribute\n");
            var->seta(newvar(res.get_int4(row, 2), res.get_string(row, 3)));
        }
    };

    if (var.get())
    {
        TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        dest(move(var));
    }
}

void StationBase::add_station_vars(int id_station, Record& rec)
{
    using namespace postgresql;
    Result res(conn.exec_prepared("v6_station_add_station_vars", id_station));
    for (unsigned row = 0; row < res.rowcount(); ++row)
        rec.set(newvar((Varcode)res.get_int4(row, 0), res.get_string(row, 1)));
}

void StationBase::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station:\n");

    auto res = conn.exec("SELECT id, lat, lon, ident FROM station");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        fprintf(out, " %d, %.5f, %.5f",
                res.get_int4(row, 0),
                res.get_int4(row, 1) / 100000.0,
                res.get_int4(row, 2) / 100000.0);
        if (res.is_null(row, 3))
            putc('\n', out);
        else
            fprintf(out, ", %s\n", res.get_string(row, 3));
        ++count;
    }
    fprintf(out, "%d element%s in table station\n", count, count != 1 ? "s" : "");
}


PostgreSQLStationV6::PostgreSQLStationV6(PostgreSQLConnection& conn)
    : StationBase(conn)
{
    conn.prepare("v6_station_get_station_vars", R"(
        SELECT d.id_var, d.value, a.type, a.value
          FROM data d
          LEFT JOIN attr a ON a.id_data = d.id
         WHERE d.id_station=$1::int4 AND d.id_report=$2::int4
           AND d.id_lev_tr = -1
         ORDER BY d.id_var, a.type
    )");
    conn.prepare("v6_station_add_station_vars", R"(
        SELECT d.id_var, d.value
          FROM data d, repinfo ri
         WHERE d.id_lev_tr = -1 AND ri.id = d.id_report AND d.id_station = $1::int4
         AND ri.prio=(
          SELECT MAX(sri.prio) FROM repinfo sri
            JOIN data sd ON sri.id=sd.id_report
          WHERE sd.id_station=d.id_station AND sd.id_lev_tr = -1
            AND sd.id_var=d.id_var)
    )");
}
PostgreSQLStationV6::~PostgreSQLStationV6()
{
}

}

}
}
