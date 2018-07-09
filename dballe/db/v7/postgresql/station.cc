#include "station.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/sql/postgresql.h"
#include "dballe/record.h"
#include "dballe/core/var.h"
#include "dballe/core/values.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::PostgreSQLConnection;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

PostgreSQLStation::PostgreSQLStation(v7::Transaction& tr, PostgreSQLConnection& conn)
    : v7::Station(tr), conn(conn)
{
    // Precompile our statements
    conn.prepare("v7_station_select_fixed", "SELECT id FROM station WHERE rep=$1::int4 AND lat=$2::int4 AND lon=$3::int4 AND ident IS NULL");
    conn.prepare("v7_station_select_mobile", "SELECT id FROM station WHERE rep=$1::int4 AND lat=$2::int4 AND lon=$3::int4 AND ident=$4::text");
    conn.prepare("v7_station_insert", "INSERT INTO station (id, rep, lat, lon, ident) VALUES (DEFAULT, $1::int4, $2::int4, $3::int4, $4::text) RETURNING id");
    conn.prepare("v7_station_get_station_vars", R"(
        SELECT d.code, d.value, d.attrs
          FROM station_data d
         WHERE d.id_station=$1::int4
         ORDER BY d.code
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

int PostgreSQLStation::maybe_get_id(Tracer<>& trc, const dballe::Station& st)
{
    using namespace dballe::sql::postgresql;

    int rep = tr.repinfo().obtain_id(st.report.c_str());

    Tracer<> trc_sel;
    Result res;
    if (st.ident.get())
    {
        if (trc) trc_sel.reset(trc->trace_select("v7_station_select_mobile", res.rowcount()));
        res = move(conn.exec_prepared("v7_station_select_mobile", rep, st.coords.lat, st.coords.lon, st.ident.get()));
    }
    else
    {
        if (trc) trc_sel.reset(trc->trace_select("v7_station_select_fixed"));
        res = move(conn.exec_prepared("v7_station_select_fixed", rep, st.coords.lat, st.coords.lon));
    }
    unsigned rows = res.rowcount();
    if (trc_sel) trc_sel->add_row(rows);
    switch (rows)
    {
        case 0: return MISSING_INT;
        case 1: return res.get_int4(0, 0);
        default: error_consistency::throwf("select station ID query returned %u results", rows);
    }
}

int PostgreSQLStation::insert_new(Tracer<>& trc, const dballe::Station& desc)
{
    // If no station was found, insert a new one
    int rep = tr.repinfo().get_id(desc.report.c_str());
    Tracer<> trc_ins(trc ? trc->trace_insert("v7_station_insert", 1) : nullptr);
    return conn.exec_prepared_one_row("v7_station_insert", rep, desc.coords.lat, desc.coords.lon, desc.ident.get()).get_int4(0, 0);
}

void PostgreSQLStation::get_station_vars(Tracer<>& trc, int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    using namespace dballe::sql::postgresql;

    TRACE("get_station_vars Performing query v7_station_get_station_vars with idst %d\n", id_station);
    Tracer<> trc_sel(trc ? trc->trace_select("v7_station_get_station_vars") : nullptr);
    Result res(conn.exec_prepared("v7_station_get_station_vars", id_station));
    if (trc_sel) trc_sel->add_row(res.rowcount());

    // Retrieve results
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        Varcode code = res.get_int4(row, 0);
        TRACE("get_station_vars Got %01d%02d%03d %s\n", WR_VAR_FXY(code), res.get_string(row, 1));

        unique_ptr<Var> var = newvar(code, res.get_string(row, 1));
        if (!res.is_null(row, 2))
        {
            TRACE("get_station_vars new attribute\n");
            Values::decode(res.get_bytea(row, 2), [&](unique_ptr<wreport::Var> a) { var->seta(move(a)); });
        }

        dest(move(var));
    };
}

void PostgreSQLStation::add_station_vars(Tracer<>& trc, int id_station, Record& rec)
{
    using namespace dballe::sql::postgresql;
    Tracer<> trc_sel(trc ? trc->trace_select("v7_station_add_station_vars") : nullptr);
    Result res(conn.exec_prepared("v7_station_add_station_vars", id_station));
    if (trc_sel) trc_sel->add_row(res.rowcount());
    for (unsigned row = 0; row < res.rowcount(); ++row)
        rec.set(newvar((Varcode)res.get_int4(row, 0), res.get_string(row, 1)));
}

void PostgreSQLStation::run_station_query(Tracer<>& trc, const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station&)> dest)
{
    using namespace dballe::sql::postgresql;
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);

    // Start the query asynchronously
    int res;
    if (qb.bind_in_ident)
    {
        const char* args[1] = { qb.bind_in_ident };
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 1, nullptr, args, nullptr, nullptr, 1);
    } else {
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
    }
    if (!res)
        throw sql::error_postgresql(conn, "executing " + qb.sql_query);

    dballe::Station station;
    conn.run_single_row_mode(qb.sql_query, [&](const Result& res) {
        if (trc_sel) trc_sel->add_row(res.rowcount());
        for (unsigned row = 0; row < res.rowcount(); ++row)
        {
            station.id = res.get_int4(row, 0);
            station.report = tr.repinfo().get_rep_memo(res.get_int4(row, 1));
            station.coords.lat = res.get_int4(row, 2);
            station.coords.lon = res.get_int4(row, 3);
            if (res.is_null(row, 4))
                station.ident.clear();
            else
                station.ident = res.get_string(row, 4);
            dest(station);
        }
    });
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
