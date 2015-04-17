/*
 * db/sqlite/driver - Backend SQLite driver
 *
 * Copyright (C) 2014--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "levtr.h"
#include "datav6.h"
#include "attrv6.h"
#include "dballe/db/v6/qbuilder.h"
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace sqlite {

Driver::Driver(SQLiteConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov5()
{
    return unique_ptr<sql::Repinfo>(new SQLiteRepinfoV5(conn));
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<sql::Repinfo>(new SQLiteRepinfoV6(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv5()
{
    return unique_ptr<sql::Station>(new SQLiteStationV5(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv6()
{
    return unique_ptr<sql::Station>(new SQLiteStationV6(conn));
}

std::unique_ptr<sql::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<sql::LevTr>(new SQLiteLevTrV6(conn));
}

std::unique_ptr<sql::DataV5> Driver::create_datav5()
{
    throw error_unimplemented("datav5 not implemented for SQLite");
}

std::unique_ptr<sql::DataV6> Driver::create_datav6()
{
    return unique_ptr<sql::DataV6>(new SQLiteDataV6(conn));
}

std::unique_ptr<sql::AttrV5> Driver::create_attrv5()
{
    throw error_unimplemented("attrv5 not implemented for SQLite");
}

std::unique_ptr<sql::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<sql::AttrV6>(new SQLiteAttrV6(conn));
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(sql::SQLRecordV6& rec)> dest)
{
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    sql::SQLRecordV6 rec;
    stm->execute([&]() {
        int output_seq = 0;

        if (qb.select_station)
        {
            rec.out_ana_id = stm->column_int(output_seq++);
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
            rec.out_rep_cod = stm->column_int(output_seq++);
            rec.out_id_ltr = stm->column_int(output_seq++);
            rec.out_varcode = stm->column_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = stm->column_int(output_seq++);

        if (qb.select_data)
        {
            rec.out_datetime = stm->column_datetime(output_seq++);

            const char* value = stm->column_string(output_seq++);
            unsigned val_size = min(strlen(value), (string::size_type)255);
            memcpy(rec.out_value, value, val_size);
            rec.out_value[val_size] = 0;
        }

        if (qb.select_summary_details)
        {
            rec.out_id_data = stm->column_int(output_seq++);
            rec.out_datetime = stm->column_datetime(output_seq++);
            rec.out_datetimemax = stm->column_datetime(output_seq++);
        }

        dest(rec);
    });
}

void Driver::create_tables_v5()
{
    conn.exec(R"(
        CREATE TABLE station (
           id         INTEGER PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE (lat, lon, ident)
        );
        CREATE INDEX pa_lon ON station(lon);
    )");
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
        CREATE TABLE context (
           id          INTEGER PRIMARY KEY,
           id_ana      INTEGER NOT NULL,
           id_report   INTEGER NOT NULL,
           datetime    TEXT NOT NULL,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE (id_ana, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2, id_report)
        );
        CREATE INDEX co_ana ON context(id_ana);
        CREATE INDEX co_report ON context(id_report);
        CREATE INDEX co_dt ON context(datetime);
        CREATE INDEX co_lt ON context(ltype1, l1, ltype2, l2);
        CREATE INDEX co_pt ON context(ptype, p1, p2);
    )");
    conn.exec(R"(
        CREATE TABLE data (
           id_context  INTEGER NOT NULL,
           id_var      INTEGER NOT NULL,
           value       VARCHAR2(255) NOT NULL,
           UNIQUE (id_var, id_context)
        );
        CREATE INDEX da_co ON data(id_context);
    )");
    conn.exec(R"(
        CREATE TABLE attr (
           id_context  INTEGER NOT NULL,
           id_var      INTEGER NOT NULL,
           type        INTEGER NOT NULL,
           value       VARCHAR2(255) NOT NULL,
           UNIQUE (id_context, id_var, type)
        );
        CREATE INDEX at_da ON attr(id_context, id_var);
    )");
}
void Driver::create_tables_v6()
{
    conn.exec(R"(
        CREATE TABLE station (
           id         INTEGER PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE (lat, lon, ident)
        );
        CREATE INDEX pa_lon ON station(lon);
    )");
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
        CREATE TABLE lev_tr (
           id         INTEGER PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE (ltype1, l1, ltype2, l2, ptype, p1, p2)
        );
    )");
    conn.exec(R"(
        CREATE TABLE data (
           id          INTEGER PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_report   INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           id_lev_tr   INTEGER NOT NULL,
           datetime    TEXT NOT NULL,
           id_var      INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE (id_station, datetime, id_lev_tr, id_report, id_var)
        );
        CREATE INDEX data_report ON data(id_report);
        CREATE INDEX data_lt ON data(id_lev_tr);
    )");
    conn.exec(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL REFERENCES data (id) ON DELETE CASCADE,
           type        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE (id_data, type)
        );
    )");
    conn.set_setting("version", "V6");
}
void Driver::delete_tables_v5()
{
    conn.drop_table_if_exists("attr");
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("context");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::delete_tables_v6()
{
    conn.drop_table_if_exists("attr");
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("lev_tr");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::vacuum_v5()
{
    conn.exec(R"(
        DELETE FROM context
              WHERE id IN (
                      SELECT c.id
                        FROM context c
                   LEFT JOIN data d ON d.id_context = c.id
                       WHERE d.id_context IS NULL)
    )");
    conn.exec(R"(
        DELETE FROM station
              WHERE id IN (
                      SELECT p.id
                        FROM station p
                   LEFT JOIN context c ON c.id_ana = p.id
                       WHERE c.id IS NULL)
    )");
}
void Driver::vacuum_v6()
{
    conn.exec(R"(
        DELETE FROM lev_tr WHERE id IN (
            SELECT ltr.id
              FROM lev_tr ltr
         LEFT JOIN data d ON d.id_lev_tr = ltr.id
             WHERE d.id_lev_tr is NULL)
    )");
    conn.exec(R"(
        DELETE FROM station WHERE id IN (
            SELECT p.id
              FROM station p
         LEFT JOIN data d ON d.id_station = p.id
             WHERE d.id is NULL)
    )");
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec(query);
}

}
}
}
