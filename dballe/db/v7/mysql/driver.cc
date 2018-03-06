#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/core/values.h"
#include "dballe/sql/mysql.h"
#include "dballe/var.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::MySQLConnection;
using dballe::sql::MySQLStatement;
using dballe::sql::Transaction;
using dballe::sql::Querybuf;
using dballe::sql::mysql::Row;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

Driver::Driver(MySQLConnection& conn)
    : v7::Driver(conn), conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v7::Repinfo> Driver::create_repinfo()
{
    return unique_ptr<v7::Repinfo>(new MySQLRepinfoV7(conn));
}

std::unique_ptr<v7::Station> Driver::create_station()
{
    return unique_ptr<v7::Station>(new MySQLStation(conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr()
{
    return unique_ptr<v7::LevTr>(new MySQLLevTr(conn));
}

std::unique_ptr<v7::StationData> Driver::create_station_data()
{
    return unique_ptr<v7::StationData>(new MySQLStationData(conn));
}

std::unique_ptr<v7::Data> Driver::create_data()
{
    return unique_ptr<v7::Data>(new MySQLData(conn));
}

void Driver::run_station_query(const v7::StationQueryBuilder& qb, std::function<void(const dballe::Station&)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        station.ana_id = row.as_int(0);
        station.report = qb.tr->db->repinfo().get_rep_memo(row.as_int(1));
        station.coords.lat = row.as_int(2);
        station.coords.lon = row.as_int(3);

        if (row.isnull(4))
            station.ident.clear();
        else
            station.ident = row.as_string(4);

        dest(station);
    });
}

void Driver::run_station_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        wreport::Varcode code = row.as_int(5);
        const char* value = row.as_cstring(7);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(row.as_blob(8), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = row.as_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->db->repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_data = row.as_int(6);

        dest(station, id_data, move(var));
    });
}

void Driver::run_data_query(const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        wreport::Varcode code = row.as_int(6);
        const char* value = row.as_cstring(9);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(row.as_blob(10), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = row.as_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->db->repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_levtr = row.as_int(5);
        int id_data = row.as_int(7);
        Datetime datetime = row.as_datetime(8);

        dest(station, id_levtr, datetime, id_data, move(var));
    });
}

void Driver::run_summary_query(const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        int id_station = row.as_int(0);
        if (id_station != station.ana_id)
        {
            station.ana_id = id_station;
            station.report = qb.tr->db->repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_levtr = row.as_int(5);
        wreport::Varcode code = row.as_int(6);

        size_t count = 0;
        DatetimeRange datetime;
        if (qb.select_summary_details)
        {
            count = row.as_int(7);
            datetime = DatetimeRange(row.as_datetime(8), row.as_datetime(9));
        }

        dest(station, id_levtr, code, datetime, count);
    });
}


void Driver::create_tables_v7()
{
    conn.exec_no_data(R"(
        CREATE TABLE repinfo (
           id           SMALLINT PRIMARY KEY,
           memo         VARCHAR(20) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL,
           UNIQUE INDEX (prio),
           UNIQUE INDEX (memo)
        ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         INTEGER auto_increment PRIMARY KEY,
           rep        INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE INDEX(rep, lat, lon, ident(8)),
           INDEX(rep),
           INDEX(lon)
        ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE levtr (
           id          INTEGER auto_increment PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           pind        INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE INDEX (ltype1, l1, ltype2, l2, pind, p1, p2)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE station_data (
           id          INTEGER auto_increment PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           code        SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BLOB,
           UNIQUE INDEX(id_station, code)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          INTEGER auto_increment PRIMARY KEY,
           id_station  INTEGER NOT NULL,
           id_levtr    INTEGER NOT NULL,
           datetime    DATETIME NOT NULL,
           code        SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BLOB,
           UNIQUE INDEX(id_station, datetime, id_levtr, code),
           INDEX(id_levtr)
       ) ENGINE=InnoDB;
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
    conn.exec_no_data("DELETE ltr FROM levtr ltr LEFT JOIN data d ON d.id_levtr=ltr.id WHERE d.id_levtr IS NULL");
    conn.exec_no_data(R"(
        DELETE sd
          FROM station_data sd
          LEFT JOIN data dd ON sd.id_station = dd.id_station
         WHERE dd.id IS NULL
    )");
    conn.exec_no_data("DELETE s FROM station s LEFT JOIN data d ON d.id_station = s.id WHERE d.id IS NULL");
}

}
}
}
}
