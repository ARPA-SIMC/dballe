#include "station.h"
#include "dballe/sql/postgresql.h"
#include "dballe/record.h"
#include "dballe/core/var.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::PostgreSQLConnection;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

PostgreSQLStation::PostgreSQLStation(PostgreSQLConnection& conn)
    : conn(conn)
{
    // Precompile our statements
    conn.prepare("v7_station_select_fixed", "SELECT id FROM station WHERE rep=$1::int4 AND lat=$2::int4 AND lon=$3::int4 AND ident IS NULL");
    conn.prepare("v7_station_select_mobile", "SELECT id FROM station WHERE rep=$1::int4 AND lat=$2::int4 AND lon=$3::int4 AND ident=$4::text");
    conn.prepare("v7_station_lookup_id", "SELECT rep, lat, lon, ident FROM station WHERE id=$1::int4");
    conn.prepare("v7_station_insert", "INSERT INTO station (id, rep, lat, lon, ident) VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::text) RETURNING id");
    conn.prepare("v7_station_get_station_vars", R"(
        SELECT d.code, d.value, a.code, a.value
          FROM station_data d
          LEFT JOIN station_attr a ON a.id_data = d.id
         WHERE d.id_station=$1::int4
         ORDER BY d.code, a.code
    )");
    conn.prepare("v7_station_add_station_vars", R"(
        SELECT d.code, d.value
          FROM station_data d
         WHERE d.id_station = $1::int4
    )");
}

PostgreSQLStation::~PostgreSQLStation()
{
}

bool PostgreSQLStation::maybe_get_id(const StationDesc& st, int* id)
{
    using namespace dballe::sql::postgresql;

    Result res;
    if (st.ident.get())
        res = move(conn.exec_prepared("v7_station_select_mobile", st.rep, st.coords.lat, st.coords.lon, st.ident.get()));
    else
        res = move(conn.exec_prepared("v7_station_select_fixed", st.rep, st.coords.lat, st.coords.lon));

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

stations_t::iterator PostgreSQLStation::lookup_id(State& st, int id)
{
    using namespace dballe::sql::postgresql;

    // First look it up in the transaction cache
    for (auto i = st.stations.begin(); i != st.stations.end(); ++i)
        if (i->second.id == id)
            return i;

    Result res = conn.exec_prepared("v7_station_lookup_id", id);

    unsigned rows = res.rowcount();
    switch (rows)
    {
        case 0: error_notfound::throwf("station with id %d not found in the database", id);
        case 1:
        {
            StationDesc desc;
            desc.rep = res.get_int4(0, 0);
            desc.coords.lat = res.get_int4(0, 1);
            desc.coords.lon = res.get_int4(0, 2);
            if (!res.is_null(0, 3))
                desc.ident = res.get_string(0, 3);
            StationState sst;
            sst.id = id;
            sst.is_new = false;
            return st.add_station(desc, sst);
        }
        default: error_consistency::throwf("select station ID query returned %u results", rows);
    }
}

stations_t::iterator PostgreSQLStation::get_id(State& st, const StationDesc& desc)
{
    auto res = st.stations.find(desc);
    if (res != st.stations.end())
        return res;

    StationState state;
    if (maybe_get_id(desc, &state.id))
    {
        state.is_new = false;
        return st.add_station(desc, state);
    }
    throw error_notfound("station not found in the database");
}

stations_t::iterator PostgreSQLStation::obtain_id(State& st, const StationDesc& desc)
{
    using namespace dballe::sql::postgresql;

    auto res = st.stations.find(desc);
    if (res != st.stations.end())
        return res;

    StationState state;
    if (maybe_get_id(desc, &state.id))
    {
        state.is_new = false;
        return st.add_station(desc, state);
    }

    // If no station was found, insert a new one
    state.id = conn.exec_prepared_one_row("v7_station_insert", desc.rep, desc.coords.lat, desc.coords.lon, desc.ident.get()).get_int4(0, 0);
    state.is_new = true;
    return st.add_station(desc, state);
}

void PostgreSQLStation::get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    using namespace dballe::sql::postgresql;

    TRACE("fill_ana_layer Performing query v7_station_get_station_vars with idst %d\n", id_station);
    Result res(conn.exec_prepared("v7_station_get_station_vars", id_station));

    // Retrieve results
    Varcode last_varcode = 0;
    unique_ptr<Var> var;

    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        Varcode code = res.get_int4(row, 0);
        TRACE("fill_ana_layer Got %01d%02ld%03ld %s\n", WR_VAR_FXY(code), res.get_string(row, 1));

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

void PostgreSQLStation::add_station_vars(int id_station, Record& rec)
{
    using namespace dballe::sql::postgresql;
    Result res(conn.exec_prepared("v7_station_add_station_vars", id_station));
    for (unsigned row = 0; row < res.rowcount(); ++row)
        rec.set(newvar((Varcode)res.get_int4(row, 0), res.get_string(row, 1)));
}

void PostgreSQLStation::_dump(std::function<void(int, int, const Coords& coords, const char* ident)> out)
{
    auto res = conn.exec("SELECT id, rep, lat, lon, ident FROM station");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* ident = res.is_null(row, 4) ? nullptr : res.get_string(row, 4);
        out(res.get_int4(row, 0), res.get_int4(row, 1), Coords((int)res.get_int4(row, 2), (int)res.get_int4(row, 3)), ident);
    }
}

}
}
}
}
