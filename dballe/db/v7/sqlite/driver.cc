#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/qbuilder.h"
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

void Driver::run_built_query_v7(
        const v7::QueryBuilder& qb,
        std::function<void(v7::SQLRecordV7& rec)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    v7::SQLRecordV7 rec;
    stm->execute([&]() {
        int output_seq = 0;

        if (qb.select_station)
        {
            rec.out_ana_id = stm->column_int(output_seq++);
            rec.out_rep_cod = stm->column_int(output_seq++);
            rec.out_lat = stm->column_int(output_seq++);
            rec.out_lon = stm->column_int(output_seq++);
            if (stm->column_isnull(output_seq))
            {
                rec.out_ident_size = -1;
                rec.out_ident[0] = 0;
            } else {
                const char* ident = stm->column_string(output_seq);
                rec.out_ident_size = min(strlen(ident), (string::size_type)63);
                memcpy(rec.out_ident, ident, rec.out_ident_size);
                rec.out_ident[rec.out_ident_size] = 0;
            }
            ++output_seq;
        }

        if (qb.select_varinfo)
        {
            if (!qb.query_station_vars)
                rec.out_id_ltr = stm->column_int(output_seq++);
            rec.out_varcode = stm->column_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = stm->column_int(output_seq++);

        if (qb.select_data)
        {
            if (!qb.query_station_vars)
                rec.out_datetime = stm->column_datetime(output_seq++);

            const char* value = stm->column_string(output_seq++);
            unsigned val_size = min(strlen(value), (string::size_type)255);
            memcpy(rec.out_value, value, val_size);
            rec.out_value[val_size] = 0;
        }

        if (qb.select_summary_details)
        {
            rec.out_id_data = stm->column_int(output_seq++);
            if (!qb.query_station_vars)
            {
                rec.out_datetime = stm->column_datetime(output_seq++);
                rec.out_datetimemax = stm->column_datetime(output_seq++);
            }
        }

        dest(rec);
    });
}

void Driver::run_station_query(const v7::StationQueryBuilder& qb, std::function<void(int id, const StationDesc&)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    StationDesc desc;
    stm->execute([&]() {
        int id = stm->column_int(0);
        desc.rep = stm->column_int(1);
        desc.coords.lat = stm->column_int(2);
        desc.coords.lon = stm->column_int(3);

        if (stm->column_isnull(4))
            desc.ident.clear();
        else
            desc.ident = stm->column_string(4);

        dest(id, desc);
    });
}

void Driver::run_station_data_query(const v7::DataQueryBuilder& qb, std::function<void(int id_station, const StationDesc& station, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    StationDesc station;
    int cur_id_station = -1;

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
        if (id_station != cur_id_station)
        {
            station.rep = stm->column_int(1);
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
            cur_id_station = id_station;
        }

        int id_data = stm->column_int(6);

        dest(id_station, station, id_data, move(var));
    });
}

void Driver::run_data_query(const v7::DataQueryBuilder& qb, std::function<void(int id_station, const StationDesc& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    StationDesc station;
    int cur_id_station = -1;

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
        if (id_station != cur_id_station)
        {
            station.rep = stm->column_int(1);
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
            cur_id_station = id_station;
        }

        int id_levtr = stm->column_int(5);
        int id_data = stm->column_int(7);
        Datetime datetime = stm->column_datetime(8);

        dest(id_station, station, id_levtr, datetime, id_data, move(var));
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
