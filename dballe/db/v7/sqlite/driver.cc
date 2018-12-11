#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "data.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
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

std::unique_ptr<v7::Repinfo> Driver::create_repinfo(v7::Transaction& tr)
{
    return unique_ptr<v7::Repinfo>(new SQLiteRepinfoV7(conn));
}

std::unique_ptr<v7::Station> Driver::create_station(v7::Transaction& tr)
{
    return unique_ptr<v7::Station>(new SQLiteStation(tr, conn));
}

std::unique_ptr<v7::LevTr> Driver::create_levtr(v7::Transaction& tr)
{
    return unique_ptr<v7::LevTr>(new SQLiteLevTr(tr, conn));
}

std::unique_ptr<v7::StationData> Driver::create_station_data(v7::Transaction& tr)
{
    return unique_ptr<v7::StationData>(new SQLiteStationData(tr, conn));
}

std::unique_ptr<v7::Data> Driver::create_data(v7::Transaction& tr)
{
    return unique_ptr<v7::Data>(new SQLiteData(tr, conn));
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
