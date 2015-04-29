/*
 * db/odbc/driver - Backend ODBC driver
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
#include <sqltypes.h>
#include <sql.h>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace odbc {

Driver::Driver(ODBCConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<sql::Repinfo>(new ODBCRepinfoV6(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv6()
{
    return unique_ptr<sql::Station>(new ODBCStationV6(conn));
}

std::unique_ptr<sql::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<sql::LevTr>(new ODBCLevTrV6(conn));
}

std::unique_ptr<sql::DataV6> Driver::create_datav6()
{
    return unique_ptr<sql::DataV6>(new ODBCDataV6(conn));
}

std::unique_ptr<sql::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<sql::AttrV6>(new ODBCAttrV6(conn));
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(sql::SQLRecordV6& rec)> dest)
{
    auto stm = conn.odbcstatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_in(1, qb.bind_in_ident);

    sql::SQLRecordV6 rec;
    int output_seq = 1;

    SQLLEN out_ident_ind;

    if (qb.select_station)
    {
        stm->bind_out(output_seq++, rec.out_ana_id);
        stm->bind_out(output_seq++, rec.out_lat);
        stm->bind_out(output_seq++, rec.out_lon);
        stm->bind_out(output_seq++, rec.out_ident, sizeof(rec.out_ident), out_ident_ind);
    }

    if (qb.select_varinfo)
    {
        stm->bind_out(output_seq++, rec.out_rep_cod);
        stm->bind_out(output_seq++, rec.out_id_ltr);
        stm->bind_out(output_seq++, rec.out_varcode);
    }

    if (qb.select_data_id)
        stm->bind_out(output_seq++, rec.out_id_data);

    SQL_TIMESTAMP_STRUCT out_datetime;
    if (qb.select_data)
    {
        stm->bind_out(output_seq++, out_datetime);
        stm->bind_out(output_seq++, rec.out_value, sizeof(rec.out_value));
    }

    SQL_TIMESTAMP_STRUCT out_datetimemax;
    if (qb.select_summary_details)
    {
        stm->bind_out(output_seq++, rec.out_id_data);
        stm->bind_out(output_seq++, out_datetime);
        stm->bind_out(output_seq++, out_datetimemax);
    }

    stm->execute();

    while (stm->fetch())
    {
        // Apply fixes here to demangle timestamps and NULL indicators
        if (out_ident_ind == SQL_NULL_DATA)
            rec.out_ident_size = -1;
        else
            rec.out_ident_size = out_ident_ind;

        rec.out_datetime.date.year = out_datetime.year;
        rec.out_datetime.date.month = out_datetime.month;
        rec.out_datetime.date.day = out_datetime.day;
        rec.out_datetime.time.hour = out_datetime.hour;
        rec.out_datetime.time.minute = out_datetime.minute;
        rec.out_datetime.time.second = out_datetime.second;

        if (qb.select_summary_details)
        {
            rec.out_datetimemax.date.year = out_datetimemax.year;
            rec.out_datetimemax.date.month = out_datetimemax.month;
            rec.out_datetimemax.date.day = out_datetimemax.day;
            rec.out_datetimemax.time.hour = out_datetimemax.hour;
            rec.out_datetimemax.time.minute = out_datetimemax.minute;
            rec.out_datetimemax.time.second = out_datetimemax.second;
        }

        dest(rec);
    }

    stm->close_cursor();
}

void Driver::create_tables_v6()
{
    switch (conn.server_type)
    {
        case ServerType::MYSQL:
            conn.exec(R"(
                CREATE TABLE station (
                   id         INTEGER auto_increment PRIMARY KEY,
                   lat        INTEGER NOT NULL,
                   lon        INTEGER NOT NULL,
                   ident      CHAR(64),
                   UNIQUE INDEX(lat, lon, ident(8)),
                   INDEX(lon)
                ) ENGINE=InnoDB;
            )");
            conn.exec(R"(
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
            conn.exec(R"(
                CREATE TABLE lev_tr (
                   id          INTEGER auto_increment PRIMARY KEY,
                   ltype1      INTEGER NOT NULL,
                   l1          INTEGER NOT NULL,
                   ltype2      INTEGER NOT NULL,
                   l2          INTEGER NOT NULL,
                   ptype       INTEGER NOT NULL,
                   p1          INTEGER NOT NULL,
                   p2          INTEGER NOT NULL,
                   UNIQUE INDEX (ltype1, l1, ltype2, l2, ptype, p1, p2)
               ) ENGINE=InnoDB;
            )");
            conn.exec(R"(
                CREATE TABLE data (
                   id          INTEGER auto_increment PRIMARY KEY,
                   id_station  SMALLINT NOT NULL,
                   id_report   INTEGER NOT NULL,
                   id_lev_tr   INTEGER NOT NULL,
                   datetime    DATETIME NOT NULL,
                   id_var      SMALLINT NOT NULL,
                   value       VARCHAR(255) NOT NULL,
                   UNIQUE INDEX(id_station, datetime, id_lev_tr, id_report, id_var),
                   INDEX(datetime),
                   INDEX(id_lev_tr)
               ) ENGINE=InnoDB;
            )");
            conn.exec(R"(
                CREATE TABLE attr (
                   id_data     INTEGER NOT NULL,
                   type        SMALLINT NOT NULL,
                   value       VARCHAR(255) NOT NULL,
                   UNIQUE INDEX (id_data, type)
               ) ENGINE=InnoDB;
            )");
            break;
        case ServerType::ORACLE:
            conn.exec(R"(
                CREATE TABLE station (
                   id         INTEGER PRIMARY KEY,
                   lat        INTEGER NOT NULL,
                   lon        INTEGER NOT NULL,
                   ident      VARCHAR2(64),
                   UNIQUE (lat, lon, ident)
                );
                CREATE INDEX pa_lon ON station(lon);
                CREATE SEQUENCE seq_station;
            )");
            conn.exec(R"(
                CREATE TABLE repinfo (
                   id           INTEGER PRIMARY KEY,
                   memo         VARCHAR2(30) NOT NULL,
                   description  VARCHAR2(255) NOT NULL,
                   prio         INTEGER NOT NULL,
                   descriptor   CHAR(6) NOT NULL,
                   tablea       INTEGER NOT NULL,
                   UNIQUE (prio),
                   UNIQUE (memo)
                )
            )");
            conn.exec(R"(
                CREATE TABLE lev_tr (
                   id          INTEGER PRIMARY KEY,
                   ltype1      INTEGER NOT NULL,
                   l1          INTEGER NOT NULL,
                   ltype2      INTEGER NOT NULL,
                   l2          INTEGER NOT NULL,
                   ptype       INTEGER NOT NULL,
                   p1          INTEGER NOT NULL,
                   p2          INTEGER NOT NULL,
                );
                CREATE SEQUENCE seq_lev_tr;
                CREATE UNIQUE INDEX lev_tr_uniq ON lev_tr(ltype1, l1, ltype2, l2, ptype, p1, p2);
            )");
            conn.exec(R"(
                CREATE TABLE data (
                   id          SERIAL PRIMARY KEY,
                   id_station  INTEGER NOT NULL,
                   id_report   INTEGER NOT NULL,
                   id_lev_tr   INTEGER NOT NULL,
                   datetime    DATE NOT NULL,
                   id_var      INTEGER NOT NULL,
                   value       VARCHAR(255) NOT NULL,
                );
                CREATE UNIQUE INDEX data_uniq(id_station, datetime, id_lev_tr, id_report, id_var);
                CREATE INDEX data_sta ON data(id_station);
                CREATE INDEX data_rep ON data(id_report);
                CREATE INDEX data_dt ON data(datetime);
                CREATE INDEX data_lt ON data(id_lev_tr);
            )");
            conn.exec(R"(
                CREATE TABLE attr ("
                   id_data     INTEGER NOT NULL,
                   type        INTEGER NOT NULL,
                   value       VARCHAR(255) NOT NULL,
                );
                CREATE UNIQUE INDEX attr_uniq ON attr(id_data, type);
            )");
            break;
        default:
            throw error_unimplemented("ODBC connection is only supported for MySQL and Oracle");
    }
    conn.set_setting("version", "V6");
}
void Driver::delete_tables_v6()
{
    conn.drop_sequence_if_exists("seq_lev_tr");  // Oracle only
    conn.drop_sequence_if_exists("seq_station"); // Oracle only
    conn.drop_table_if_exists("attr");
    conn.drop_table_if_exists("data");
    conn.drop_table_if_exists("lev_tr");
    conn.drop_table_if_exists("repinfo");
    conn.drop_table_if_exists("station");
    conn.drop_settings();
}
void Driver::vacuum_v6()
{
    switch (conn.server_type)
    {
        case ServerType::MYSQL:
            conn.exec("DELETE c FROM lev_tr c LEFT JOIN data d ON d.id_lev_tr = c.id WHERE d.id_lev_tr IS NULL");
            conn.exec("DELETE p FROM station p LEFT JOIN data d ON d.id_station = p.id WHERE d.id IS NULL");
            break;
        default:
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
            break;
    }
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec(query);
}


}
}
}
