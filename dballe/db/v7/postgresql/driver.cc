#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/core/values.h"
#include "dballe/sql/postgresql.h"
#include "dballe/var.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::error_postgresql;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

Driver::Driver(PostgreSQLConnection& conn)
    : v7::Driver(conn), conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<v7::Repinfo> Driver::create_repinfo(v7::Transaction& tr)
{
    return unique_ptr<v7::Repinfo>(new PostgreSQLRepinfo(conn));
}

std::unique_ptr<v7::Station> Driver::create_station(v7::Transaction& tr)
{
    return unique_ptr<v7::Station>(new PostgreSQLStation(tr, conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr(v7::Transaction& tr)
{
    return unique_ptr<v7::LevTr>(new PostgreSQLLevTr(tr, conn));
}

std::unique_ptr<v7::StationData> Driver::create_station_data(v7::Transaction& tr)
{
    return unique_ptr<v7::StationData>(new PostgreSQLStationData(tr, conn));
}

std::unique_ptr<v7::Data> Driver::create_data(v7::Transaction& tr)
{
    return unique_ptr<v7::Data>(new PostgreSQLData(tr, conn));
}

void Driver::create_tables_v7()
{
    conn.exec_no_data(R"(
        CREATE TABLE repinfo (
           id           INTEGER PRIMARY KEY,
           memo         VARCHAR(30) NOT NULL,
           description  VARCHAR(255) NOT NULL,
           prio         INTEGER NOT NULL,
           descriptor   CHAR(6) NOT NULL,
           tablea       INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX ri_memo_uniq ON repinfo(memo);");
    conn.exec_no_data("CREATE UNIQUE INDEX ri_prio_uniq ON repinfo(prio);");

    conn.exec_no_data(R"(
        CREATE TABLE station (
           id     SERIAL PRIMARY KEY,
           rep    INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           lat    INTEGER NOT NULL,
           lon    INTEGER NOT NULL,
           ident  VARCHAR(64)
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX pa_uniq ON station(rep, lat, lon, ident);");
    conn.exec_no_data("CREATE INDEX pa_lon ON station(lon);");

    conn.exec_no_data(R"(
        CREATE TABLE levtr (
           id       SERIAL PRIMARY KEY,
           ltype1   INTEGER NOT NULL,
           l1       INTEGER NOT NULL,
           ltype2   INTEGER NOT NULL,
           l2       INTEGER NOT NULL,
           pind     INTEGER NOT NULL,
           p1       INTEGER NOT NULL,
           p2       INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX levtr_uniq ON levtr(ltype1, l1, ltype2, l2, pind, p1, p2);");

    conn.exec_no_data(R"(
        CREATE TABLE station_data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BYTEA
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX station_data_uniq on station_data(id_station, code);");

    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_levtr    INTEGER NOT NULL REFERENCES levtr(id) ON DELETE CASCADE,
           datetime    TIMESTAMP NOT NULL,
           code        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           attrs       BYTEA
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX data_uniq on data(id_station, datetime, id_levtr, code);");
    // When possible, replace with a postgresql 9.5 BRIN index
    conn.exec_no_data("CREATE INDEX data_dt ON data(datetime);");

    conn.set_setting("version", "V7");
}
void Driver::delete_tables_v7()
{
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("station_data");
    conn.drop_table_if_exists("levtr");
    conn.drop_table_if_exists("station");
    conn.drop_table_if_exists("repinfo");
    conn.drop_settings();
}
void Driver::vacuum_v7()
{
    conn.exec_no_data(R"(
        DELETE FROM levtr WHERE id IN (
            SELECT ltr.id
              FROM levtr ltr
         LEFT JOIN data d ON d.id_levtr = ltr.id
             WHERE d.id_levtr is NULL)
    )");
    conn.exec_no_data(R"(
        DELETE FROM station_data WHERE id IN (
            SELECT sd.id
              FROM station_data sd
              LEFT JOIN data dd ON sd.id_station = dd.id_station
             WHERE dd.id IS NULL)
    )");
    conn.exec_no_data(R"(
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
