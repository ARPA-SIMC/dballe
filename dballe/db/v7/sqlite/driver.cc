#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/core/values.h"
#include "dballe/sql/sqlite.h"
#include "dballe/var.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Transaction;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

Driver::Driver(SQLiteConnection& conn)
    : v7::Driver(conn), conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v7::Repinfo> Driver::create_repinfo()
{
    return unique_ptr<v7::Repinfo>(new SQLiteRepinfoV7(conn));
}

std::unique_ptr<v7::Station> Driver::create_station()
{
    return unique_ptr<v7::Station>(new SQLiteStation(conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr()
{
    return unique_ptr<v7::LevTr>(new SQLiteLevTr(conn));
}

std::unique_ptr<v7::StationData> Driver::create_station_data()
{
    return unique_ptr<v7::StationData>(new SQLiteStationData(conn));
}

std::unique_ptr<v7::Data> Driver::create_data()
{
    return unique_ptr<v7::Data>(new SQLiteData(conn));
}

void Driver::run_station_query(const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station&)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::Station station;
    stm->execute([&]() {
        station.ana_id = stm->column_int(0);
        station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
        station.coords.lat = stm->column_int(2);
        station.coords.lon = stm->column_int(3);

        if (stm->column_isnull(4))
            station.ident.clear();
        else
            station.ident = stm->column_string(4);

        dest(station);
    });
}

void Driver::run_station_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::Station station;
    stm->execute([&]() {
        wreport::Varcode code = stm->column_int(5);
        const char* value = stm->column_string(7);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(stm->column_blob(8), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = stm->column_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_data = stm->column_int(6);

        dest(station, id_data, move(var));
    });
}

void Driver::run_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::Station station;
    stm->execute([&]() {
        wreport::Varcode code = stm->column_int(6);
        const char* value = stm->column_string(9);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(stm->column_blob(10), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = stm->column_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_levtr = stm->column_int(5);
        int id_data = stm->column_int(7);
        Datetime datetime = stm->column_datetime(8);

        dest(station, id_levtr, datetime, id_data, move(var));
    });
}

void Driver::run_summary_query(const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::Station station;
    stm->execute([&]() {
        int id_station = stm->column_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_levtr = stm->column_int(5);
        wreport::Varcode code = stm->column_int(6);

        size_t count = 0;
        DatetimeRange datetime;
        if (qb.select_summary_details)
        {
            count = stm->column_int(7);
            datetime = DatetimeRange(stm->column_datetime(8), stm->column_datetime(9));
        }

        dest(station, id_levtr, code, datetime, count);
    });
}


void Driver::create_tables_v7()
{
    conn.exec(R"(
        CREATE TABLE repinfo (
           id           INTEGER PRIMARY KEY,
           memo         VARCHAR(30) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL,
           UNIQUE (prio),
           UNIQUE (memo)
        );
    )");
    conn.exec(R"(
        CREATE TABLE station (
           id    INTEGER PRIMARY KEY,
           rep   INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           lat   INTEGER NOT NULL,
           lon   INTEGER NOT NULL,
           ident CHAR(64),
           UNIQUE (rep, lat, lon, ident)
        );
        CREATE INDEX pa_rep ON station(rep);
        CREATE INDEX pa_lon ON station(lon);
    )");
    conn.exec(R"(
        CREATE TABLE levtr (
           id         INTEGER PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           pind       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE (ltype1, l1, ltype2, l2, pind, p1, p2)
        );
    )");
    conn.exec(R"(
        CREATE TABLE station_data (
           id          INTEGER PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BLOB,
           UNIQUE (id_station, code)
        );
    )");
    conn.exec(R"(
        CREATE TABLE data (
           id          INTEGER PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_levtr    INTEGER NOT NULL REFERENCES levtr(id) ON DELETE CASCADE,
           datetime    TEXT NOT NULL,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BLOB,
           UNIQUE (id_station, datetime, id_levtr, code)
        );
        CREATE INDEX data_lt ON data(id_levtr);
    )");

    conn.set_setting("version", "V7");
}
void Driver::delete_tables_v7()
{
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("station_data");
    conn.drop_table_if_exists("levtr");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::vacuum_v7()
{
    conn.exec(R"(
        DELETE FROM levtr WHERE id IN (
            SELECT ltr.id
              FROM levtr ltr
         LEFT JOIN data d ON d.id_levtr = ltr.id
             WHERE d.id_levtr is NULL)
    )");
    conn.exec(R"(
        DELETE FROM station_data WHERE id IN (
            SELECT sd.id
              FROM station_data sd
              LEFT JOIN data dd ON sd.id_station = dd.id_station
             WHERE dd.id IS NULL)
    )");
    conn.exec(R"(
        DELETE FROM station WHERE id IN (
            SELECT p.id
              FROM station p
         LEFT JOIN data d ON d.id_station = p.id
             WHERE d.id is NULL)
    )");
}

}
}
}
}
