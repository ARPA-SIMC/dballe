/*
 * db/mysql/driver - Backend MySQL driver
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "internals.h"
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
namespace mysql {

Driver::Driver(MySQLConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov5()
{
    return unique_ptr<sql::Repinfo>(new MySQLRepinfoV5(conn));
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<sql::Repinfo>(new MySQLRepinfoV6(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv5()
{
    return unique_ptr<sql::Station>(new MySQLStationV5(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv6()
{
    return unique_ptr<sql::Station>(new MySQLStationV6(conn));
}

std::unique_ptr<sql::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<sql::LevTr>(new MySQLLevTrV6(conn));
}

std::unique_ptr<sql::DataV5> Driver::create_datav5()
{
    throw error_unimplemented("datav5 not implemented for MySQL");
}

std::unique_ptr<sql::DataV6> Driver::create_datav6()
{
    return unique_ptr<sql::DataV6>(new MySQLDataV6(conn));
}

std::unique_ptr<sql::AttrV5> Driver::create_attrv5()
{
    throw error_unimplemented("attrv5 not implemented for MySQL");
}

std::unique_ptr<sql::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<sql::AttrV6>(new MySQLAttrV6(conn));
}

void Driver::bulk_insert_v6(sql::bulk::InsertV6& vars, bool update_existing)
{
    std::sort(vars.begin(), vars.end());

    // Get the current status of variables for this context
    Querybuf select;
    select.appendf(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=%d AND id_report=%d AND datetime='%04d-%02d-%02d %02d:%02d:%02d'
         ORDER BY id_lev_tr, id_var
    )", vars.id_station, vars.id_report,
        vars.datetime.date.year, vars.datetime.date.month, vars.datetime.date.day,
        vars.datetime.time.hour, vars.datetime.time.minute, vars.datetime.time.second);
    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateVarsV6 todo(vars);
    auto select_res = conn.exec_store(select);
    while (auto row = select_res.fetch())
    {
        todo.annotate(
                row.as_int(0),
                row.as_int(1),
                row.as_int(2),
                row.as_cstring(3));
    }
    todo.annotate_end();

    // We now have a todo-list

    if (update_existing && todo.do_update)
    {
        for (auto& v: vars)
        {
            if (!v.needs_update()) continue;
            string escaped_value = conn.escape(v.var->value());
            Querybuf update;
            update.appendf("UPDATE data SET value='%s' WHERE id=%d", escaped_value.c_str(), v.id_data);
            conn.exec_no_data(update);
            v.set_updated();
        }
    }

    if (todo.do_insert)
    {
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            string escaped_value = conn.escape(v.var->value());
            Querybuf insert(512);
            insert.appendf(R"(
                INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
                     VALUES (%d, %d, %d '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s')
            )", vars.id_station, vars.id_report, v.id_levtr,
                vars.datetime.date.year, vars.datetime.date.month, vars.datetime.date.day,
                vars.datetime.time.hour, vars.datetime.time.minute, vars.datetime.time.second,
                v.var->code(), escaped_value.c_str());
            conn.exec_no_data(insert);
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(sql::SQLRecordV6& rec)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    sql::SQLRecordV6 rec;
    conn.exec_use(qb.sql_query, [&](const Row& row) {
        int output_seq = 0;
        SQLLEN out_ident_ind;

        if (qb.select_station)
        {
            rec.out_ana_id = row.as_int(output_seq++);
            rec.out_lat = row.as_int(output_seq++);
            rec.out_lon = row.as_int(output_seq++);
            if (row.isnull(output_seq))
            {
                rec.out_ident_size = -1;
                rec.out_ident[0] = 0;
            } else {
                const char* ident = row.as_cstring(output_seq);
                rec.out_ident_size = min(strlen(ident), (string::size_type)63);
                memcpy(rec.out_ident, ident, rec.out_ident_size);
                rec.out_ident[rec.out_ident_size] = 0;
            }
            ++output_seq;
        }

        if (qb.select_varinfo)
        {
            rec.out_rep_cod = row.as_int(output_seq++);
            rec.out_id_ltr = row.as_int(output_seq++);
            rec.out_varcode = row.as_int(output_seq++);
        }

        if (qb.select_data_id)
            rec.out_id_data = row.as_int(output_seq++);

        if (qb.select_data)
        {
            rec.out_datetime = row.as_datetime(output_seq++);

            const char* value = row.as_cstring(output_seq++);
            unsigned val_size = min(strlen(value), (string::size_type)255);
            memcpy(rec.out_value, value, val_size);
            rec.out_value[val_size] = 0;
        }

        if (qb.select_summary_details)
        {
            rec.out_id_data = row.as_int(output_seq++);
            rec.out_datetime = row.as_datetime(output_seq++);
            rec.out_datetimemax = row.as_datetime(output_seq++);
        }

        dest(rec);
    });
}

void Driver::run_delete_query_v6(const v6::QueryBuilder& qb)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");
    Querybuf dq(512);
    dq.append("DELETE FROM data WHERE id IN (");
    dq.append(qb.sql_query);
    dq.append(")");
    conn.exec_no_data(dq);
}

void Driver::create_tables_v5()
{
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         INTEGER auto_increment PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE INDEX(lat, lon, ident(8)),
           INDEX(lon)
        ) ENGINE=InnoDB;
    )");
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
        CREATE TABLE context (
           id          INTEGER auto_increment PRIMARY KEY,
           id_ana      INTEGER NOT NULL,
           id_report   SMALLINT NOT NULL,
           datetime    DATETIME NOT NULL,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL,
           UNIQUE INDEX (id_ana, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2, id_report),
           INDEX (id_ana),
           INDEX (id_report),
           INDEX (datetime),
           INDEX (ltype1, l1, ltype2, l2),
           INDEX (ptype, p1, p2)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE data (
           id_context  INTEGER NOT NULL,
           id_var      SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           INDEX (id_context),
           UNIQUE INDEX(id_var, id_context)
       ) ENGINE=InnoDB;
    )");
    conn.exec_no_data(R"(
        CREATE TABLE attr (
           id_context  INTEGER NOT NULL,
           id_var      SMALLINT NOT NULL,
           type        SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           INDEX (id_context, id_var),
           UNIQUE INDEX (id_context, id_var, type)
       ) ENGINE=InnoDB;
    )");
}
void Driver::create_tables_v6()
{
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         INTEGER auto_increment PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      CHAR(64),
           UNIQUE INDEX(lat, lon, ident(8)),
           INDEX(lon)
        ) ENGINE=InnoDB;
    )");
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
    conn.exec_no_data(R"(
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
    conn.exec_no_data(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL,
           type        SMALLINT NOT NULL,
           value       VARCHAR(255) NOT NULL,
           UNIQUE INDEX (id_data, type)
       ) ENGINE=InnoDB;
    )");
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
    conn.exec_no_data("DELETE c FROM context c LEFT JOIN data d ON d.id_context = c.id WHERE d.id_context IS NULL");
    conn.exec_no_data("DELETE p FROM station p LEFT JOIN context c ON c.id_ana = p.id WHERE c.id is NULL");
}
void Driver::vacuum_v6()
{
    conn.exec_no_data("DELETE c FROM lev_tr c LEFT JOIN data d ON d.id_lev_tr = c.id WHERE d.id_lev_tr IS NULL");
    conn.exec_no_data("DELETE p FROM station p LEFT JOIN data d ON d.id_station = p.id WHERE d.id IS NULL");
}

void Driver::exec_no_data(const std::string& query)
{
    conn.exec_no_data(query);
}

}
}
}
