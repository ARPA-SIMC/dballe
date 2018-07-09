#include "station.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/qbuilder.h"
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

MySQLStation::MySQLStation(v7::Transaction& tr, MySQLConnection& conn)
    : v7::Station(tr), conn(conn)
{
}

MySQLStation::~MySQLStation()
{
}

int MySQLStation::maybe_get_id(Tracer<>& trc, const dballe::DBStation& st)
{
    int rep = tr.repinfo().obtain_id(st.report.c_str());

    Querybuf qb;
    if (st.ident.get())
    {
        string escaped_ident = conn.escape(st.ident.get());
        qb.appendf("SELECT id FROM station WHERE rep=%d AND lat=%d AND lon=%d AND ident='%s'",
                rep, st.coords.lat, st.coords.lon, escaped_ident.c_str());
    } else {
        qb.appendf("SELECT id FROM station WHERE rep=%d AND lat=%d AND lon=%d AND ident IS NULL",
                rep, st.coords.lat, st.coords.lon);
    }
    Tracer<> trc_sel(trc ? trc->trace_select(qb) : nullptr);
    auto res = conn.exec_store(qb);
    if (trc_sel) trc_sel->add_row(res.rowcount());
    switch (res.rowcount())
    {
        case 0:
            return MISSING_INT;
        case 1:
            return res.fetch().as_int(0);
        default:
            error_consistency::throwf("select station ID query returned %u results", res.rowcount());
    }
}

int MySQLStation::insert_new(Tracer<>& trc, const dballe::DBStation& desc)
{
    // If no station was found, insert a new one
    int rep = tr.repinfo().get_id(desc.report.c_str());
    Querybuf qb;
    if (desc.ident.get())
    {
        string escaped_ident = conn.escape(desc.ident.get());
        qb.appendf(R"(
            INSERT INTO station (rep, lat, lon, ident) VALUES (%d, %d, %d, '%s')
        )", rep, desc.coords.lat, desc.coords.lon, escaped_ident.c_str());
    } else {
        qb.appendf(R"(
            INSERT INTO station (rep, lat, lon, ident) VALUES (%d, %d, %d, NULL)
        )", rep, desc.coords.lat, desc.coords.lon);
    }
    Tracer<> trc_ins(trc ? trc->trace_insert(qb, 1) : nullptr);
    conn.exec_no_data(qb);
    return conn.get_last_insert_id();
}

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

void MySQLStation::get_station_vars(Tracer<>& trc, int id_station, std::function<void(std::unique_ptr<wreport::Var>)> dest)
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

    Tracer<> trc_sel(trc ? trc->trace_select(qb) : nullptr);
    auto res = conn.exec_store(qb);
    while (auto row = res.fetch())
    {
        if (trc_sel) trc_sel->add_row();
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

void MySQLStation::add_station_vars(Tracer<>& trc, int id_station, Record& rec)
{
    Querybuf qb;
    qb.appendf(R"(
        SELECT d.code, d.value
          FROM station_data d
         WHERE d.id_station=%d
    )", id_station);

    Tracer<> trc_sel(trc ? trc->trace_select(qb) : nullptr);
    auto res = conn.exec_store(qb);
    while (auto row = res.fetch())
    {
        if (trc_sel) trc_sel->add_row();
        rec.set(newvar((wreport::Varcode)row.as_int(0), row.as_cstring(1)));
    }
}

void MySQLStation::run_station_query(Tracer<>& trc, const v7::StationQueryBuilder& qb, std::function<void(const dballe::DBStation&)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);

    dballe::DBStation station;
    conn.exec_use(qb.sql_query, [&](const sql::mysql::Row& row) {
        if (trc_sel) trc_sel->add_row();
        station.id = row.as_int(0);
        station.report = tr.repinfo().get_rep_memo(row.as_int(1));
        station.coords.lat = row.as_int(2);
        station.coords.lon = row.as_int(3);

        if (row.isnull(4))
            station.ident.clear();
        else
            station.ident = row.as_string(4);

        dest(station);
    });
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

}
}
}
}
