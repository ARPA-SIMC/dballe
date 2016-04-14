#include "station.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/var.h"
#include "dballe/core/values.h"
#include <wreport/var.h>

using namespace wreport;
using namespace dballe::db;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::MySQLStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

MySQLStation::MySQLStation(MySQLConnection& conn)
    : conn(conn)
{
}

MySQLStation::~MySQLStation()
{
}

bool MySQLStation::maybe_get_id(const StationDesc& st, int* id)
{
    Querybuf qb;
    if (st.ident.get())
    {
        string escaped_ident = conn.escape(st.ident.get());
        qb.appendf("SELECT id FROM station WHERE rep=%d AND lat=%d AND lon=%d AND ident='%s'",
                st.rep, st.coords.lat, st.coords.lon, escaped_ident.c_str());
    } else {
        qb.appendf("SELECT id FROM station WHERE rep=%d AND lat=%d AND lon=%d AND ident IS NULL",
                st.rep, st.coords.lat, st.coords.lon);
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

stations_t::iterator MySQLStation::lookup_id(State& st, int id)
{
    // First look it up in the transaction cache
    for (auto i = st.stations.begin(); i != st.stations.end(); ++i)
        if (i->second.id == id)
            return i;

    Querybuf qb;
    qb.appendf("SELECT rep, lat, lon, ident FROM station WHERE id=%d", id);
    auto res = conn.exec_store(qb);
    auto row = res.expect_one_result();

    StationDesc desc;
    desc.rep = row.as_int(0);
    desc.coords.lat = row.as_int(1);
    desc.coords.lon = row.as_int(2);
    if (!row.isnull(3))
        desc.ident = row.as_string(3);
    StationState sst;
    sst.id = id;
    sst.is_new = false;
    return st.add_station(desc, sst);
}

stations_t::iterator MySQLStation::obtain_id(State& st, const StationDesc& desc)
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

    // If no station was found, insert a new one
    Querybuf qb;
    if (desc.ident.get())
    {
        string escaped_ident = conn.escape(desc.ident.get());
        qb.appendf(R"(
            INSERT INTO station (rep, lat, lon, ident) VALUES (%d, %d, %d, '%s')
        )", desc.rep, desc.coords.lat, desc.coords.lon, escaped_ident.c_str());
    } else {
        qb.appendf(R"(
            INSERT INTO station (rep, lat, lon, ident) VALUES (%d, %d, %d, NULL)
        )", desc.rep, desc.coords.lat, desc.coords.lon);
    }
    conn.exec_no_data(qb);
    state.id = conn.get_last_insert_id();
    state.is_new = true;
    return st.add_station(desc, state);
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

void MySQLStation::get_station_vars(int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Perform the query
    Querybuf qb;
    qb.appendf(R"(
        SELECT d.code, d.value, d.attrs
          FROM station_data d
         WHERE d.id_station=%d
         ORDER BY d.code
    )", id_station);
    TRACE("get_station_vars Performing query: %s\n", qb.c_str());

    auto res = conn.exec_store(qb);
    while (auto row = res.fetch())
    {
        Varcode code = row.as_int(0);
        TRACE("get_station_vars Got %d%02d%03d %s\n", WR_VAR_FXY(code), row.as_cstring(1));

        unique_ptr<Var> var = newvar(code, row.as_cstring(1));
        if (!row.isnull(2))
        {
            TRACE("get_station_vars add attributes\n");
            Values::decode(row.as_blob(2), [&](unique_ptr<wreport::Var> a) { var->seta(move(a)); });
        }

        dest(move(var));
    }
}

void MySQLStation::_dump(std::function<void(int, int, const Coords& coords, const char* ident)> out)
{
    auto res = conn.exec_store("SELECT id, rep, lat, lon, ident FROM station");
    while (auto row = res.fetch())
    {
        const char* ident = row.isnull(4) ? nullptr : row.as_cstring(4);
        out(row.as_int(0), row.as_int(1), Coords(row.as_int(2), row.as_int(3)), ident);
    }
}

void MySQLStation::add_station_vars(int id_station, Record& rec)
{
    Querybuf qb;
    qb.appendf(R"(
        SELECT d.code, d.value
          FROM station_data d
         WHERE d.id_station=%d
    )", id_station);

    auto res = conn.exec_store(qb);
    while (auto row = res.fetch())
        rec.set(newvar((wreport::Varcode)row.as_int(0), row.as_cstring(1)));
}

}
}
}
}
