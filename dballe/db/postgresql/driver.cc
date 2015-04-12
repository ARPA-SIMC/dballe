/*
 * db/postgresql/driver - Backend PostgreSQL driver
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
namespace postgresql {

Driver::Driver(PostgreSQLConnection& conn)
    : conn(conn)
{
}

Driver::~Driver()
{
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov5()
{
    return unique_ptr<sql::Repinfo>(new PostgreSQLRepinfoV5(conn));
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<sql::Repinfo>(new PostgreSQLRepinfoV6(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv5()
{
    return unique_ptr<sql::Station>(new PostgreSQLStationV5(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv6()
{
    return unique_ptr<sql::Station>(new PostgreSQLStationV6(conn));
}

std::unique_ptr<sql::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<sql::LevTr>(new PostgreSQLLevTrV6(conn));
}

std::unique_ptr<sql::DataV5> Driver::create_datav5()
{
    throw error_unimplemented("datav5 not implemented for PostgreSQL");
}

std::unique_ptr<sql::DataV6> Driver::create_datav6()
{
    return unique_ptr<sql::DataV6>(new PostgreSQLDataV6(conn));
}

std::unique_ptr<sql::AttrV5> Driver::create_attrv5()
{
    throw error_unimplemented("attrv5 not implemented for PostgreSQL");
}

std::unique_ptr<sql::AttrV6> Driver::create_attrv6()
{
    return unique_ptr<sql::AttrV6>(new PostgreSQLAttrV6(conn));
}

void Driver::bulk_insert_v6(sql::bulk::InsertV6& vars, bool update_existing)
{
    std::sort(vars.begin(), vars.end());

    using namespace postgresql;
    const char* select_query = R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=$1::int4 AND id_report=$2::int4 AND datetime=$3::timestamp
         ORDER BY id_lev_tr, id_var
    )";

    // TODO: call this on the parent, which is managing the transaction
    //conn.exec_no_data("LOCK TABLE data IN EXCLUSIVE MODE");

    // Get the current status of variables for this context
    Result existing(conn.exec(select_query, vars.id_station, vars.id_report, vars.datetime));

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateVarsV6 todo(vars);
    for (unsigned row = 0; row < existing.rowcount(); ++row)
    {
        if (!todo.annotate(
                existing.get_int4(row, 0),
                existing.get_int4(row, 1),
                (Varcode)existing.get_int4(row, 2),
                existing.get_string(row, 3)))
            break;
    }
    todo.annotate_end();

    // We now have a todo-list

    if (update_existing && todo.do_update)
    {
        Querybuf dq(512);
        dq.append("UPDATE data as d SET value=i.value FROM (values ");
        dq.start_list(",");
        for (auto& v: vars)
        {
            if (!v.needs_update()) continue;
            char* escaped_val = PQescapeLiteral(conn, v.var->value(), strlen(v.var->value()));
            if (!escaped_val)
                throw error_postgresql(conn, string("escaping string '") + v.var->value() + "'");
            dq.append_listf("(%d, %s)", v.id_data, escaped_val);
            PQfreemem(escaped_val);
            v.set_updated();
        }
        dq.append(") AS i(id, value) WHERE d.id = i.id");
        //fprintf(stderr, "Update query: %s\n", dq.c_str());
        conn.exec_no_data(dq);
    }

    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.append("INSERT INTO data (id, id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES ");
        dq.start_list(",");
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            char* escaped_val = PQescapeLiteral(conn, v.var->value(), strlen(v.var->value()));
            if (!escaped_val)
                throw error_postgresql(conn, string("escaping string '") + v.var->value() + "'");
            dq.append_listf("(DEFAULT, %d, %d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, %s)",
                    vars.id_station, vars.id_report, v.id_levtr,
                    vars.datetime.date.year, vars.datetime.date.month, vars.datetime.date.day,
                    vars.datetime.time.hour, vars.datetime.time.minute, vars.datetime.time.second,
                    (int)v.var->code(), escaped_val);
            PQfreemem(escaped_val);
        }
        dq.append(" RETURNING id");

        //fprintf(stderr, "Insert query: %s\n", dq.c_str());

        // Run the insert query and read back the new IDs
        Result res(conn.exec(dq));
        unsigned row = 0;
        auto v = vars.begin();
        while (row < res.rowcount() && v != vars.end())
        {
            if (!v->needs_insert())
            {
               ++v;
               continue;
            }
            v->id_data = res.get_int4(row, 0);
            v->set_inserted();
            ++v;
            ++row;
        }
    }
}

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(sql::SQLRecordV6& rec)> dest)
{
    using namespace dballe::db::postgresql;

    // fprintf(stderr, "QUERY %d %s\n", qb.bind_in_ident, qb.sql_query.c_str());

    // Start the query asynchronously
    int res;
    if (qb.bind_in_ident)
    {
        const char* args[1] = { qb.bind_in_ident };
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 1, nullptr, args, nullptr, nullptr, 1);
    } else {
        res = PQsendQueryParams(conn, qb.sql_query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
    }
    if (!res)
        throw error_postgresql(conn, "executing " + qb.sql_query);

    // http://www.postgresql.org/docs/9.4/static/libpq-single-row-mode.html
    if (!PQsetSingleRowMode(conn))
    {
        string errmsg(PQerrorMessage(conn));
        conn.cancel_running_query_nothrow();
        conn.discard_all_input_nothrow();
        throw error_postgresql(errmsg, "cannot set single row mode for query " + qb.sql_query);
    }

    sql::SQLRecordV6 rec;
    while (true)
    {
        Result res(PQgetResult(conn));
        if (!res) break;

        // Note: Even when PQresultStatus indicates a fatal error, PQgetResult
        // should be called until it returns a null pointer to allow libpq to
        // process the error information completely.
        //  (http://www.postgresql.org/docs/9.1/static/libpq-async.html)

        // If we get what we don't want, cancel, flush our input and throw
        if (PQresultStatus(res) == PGRES_SINGLE_TUPLE)
        {
            // Ok, we have a tuple
        } else if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            // No more rows will arrive
            continue;
        } else {
            // An error arrived
            conn.cancel_running_query_nothrow();
            conn.discard_all_input_nothrow();

            switch (PQresultStatus(res))
            {
                case PGRES_COMMAND_OK:
                    throw error_postgresql("command_ok", "no data returned by query " + qb.sql_query);
                default:
                    throw error_postgresql(res, "executing " + qb.sql_query);
            }
        }
        // fprintf(stderr, "ST %d vi %d did %d d %d sd %d\n", qb.select_station, qb.select_varinfo, qb.select_data_id, qb.select_data, qb.select_summary_details);
        for (unsigned row = 0; row < res.rowcount(); ++row)
        {
            int output_seq = 0;
            if (qb.select_station)
            {
                rec.out_ana_id = res.get_int4(row, output_seq++);
                rec.out_lat = res.get_int4(row, output_seq++);
                rec.out_lon = res.get_int4(row, output_seq++);
                if (res.is_null(row, output_seq))
                {
                    rec.out_ident_size = -1;
                    rec.out_ident[0] = 0;
                } else {
                    const char* ident = res.get_string(row, output_seq);
                    rec.out_ident_size = min(PQgetlength(res, row, output_seq), 63);
                    memcpy(rec.out_ident, ident, rec.out_ident_size);
                    rec.out_ident[rec.out_ident_size] = 0;
                }
                ++output_seq;
            }

            if (qb.select_varinfo)
            {
                rec.out_rep_cod = res.get_int4(row, output_seq++);
                rec.out_id_ltr = res.get_int4(row, output_seq++);
                rec.out_varcode = res.get_int4(row, output_seq++);
            }

            if (qb.select_data_id)
                rec.out_id_data = res.get_int4(row, output_seq++);

            if (qb.select_data)
            {
                rec.out_datetime = res.get_timestamp(row, output_seq++);

                int value_len = min(PQgetlength(res, row, output_seq), 255);
                const char* value = res.get_string(row, output_seq++);
                memcpy(rec.out_value, value, value_len);
                rec.out_value[value_len] = 0;
            }

            if (qb.select_summary_details)
            {
                rec.out_id_data = res.get_int8(row, output_seq++);
                rec.out_datetime = res.get_timestamp(row, output_seq++);
                rec.out_datetimemax = res.get_timestamp(row, output_seq++);
            }

            // rec.dump(stderr);
            try {
                dest(rec);
            } catch (std::exception& e) {
                // If we get an exception from downstream, cancel, flush all
                // input and rethrow it
                conn.cancel_running_query_nothrow();
                conn.discard_all_input_nothrow();
                throw;
            }
        }
    }
}

void Driver::run_delete_query_v6(const v6::QueryBuilder& qb)
{
    Querybuf dq(512);
    dq.append("DELETE FROM data WHERE id IN (");
    dq.append(qb.sql_query);
    dq.append(")");
    if (qb.bind_in_ident)
    {
        conn.exec_no_data(dq.c_str(), qb.bind_in_ident);
    } else {
        conn.exec_no_data(dq.c_str());
    }
}

void Driver::create_tables_v5()
{
}
void Driver::create_tables_v6()
{
    conn.exec_no_data(R"(
        CREATE TABLE station (
           id         SERIAL PRIMARY KEY,
           lat        INTEGER NOT NULL,
           lon        INTEGER NOT NULL,
           ident      VARCHAR(64)
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX pa_uniq ON station(lat, lon, ident);");
    conn.exec_no_data("CREATE INDEX pa_lon ON station(lon);");
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
        CREATE TABLE lev_tr (
           id          SERIAL PRIMARY KEY,
           ltype1      INTEGER NOT NULL,
           l1          INTEGER NOT NULL,
           ltype2      INTEGER NOT NULL,
           l2          INTEGER NOT NULL,
           ptype       INTEGER NOT NULL,
           p1          INTEGER NOT NULL,
           p2          INTEGER NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX lev_tr_uniq ON lev_tr(ltype1, l1, ltype2, l2, ptype, p1, p2);");
    conn.exec_no_data(R"(
        CREATE TABLE data (
           id          SERIAL PRIMARY KEY,
           id_station  INTEGER NOT NULL REFERENCES station (id) ON DELETE CASCADE,
           id_report   INTEGER NOT NULL REFERENCES repinfo (id) ON DELETE CASCADE,
           id_lev_tr   INTEGER NOT NULL,
           datetime    TIMESTAMP NOT NULL,
           id_var      INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX data_uniq on data(id_station, datetime, id_lev_tr, id_report, id_var);");
    conn.exec_no_data("CREATE INDEX data_ana ON data(id_station);");
    conn.exec_no_data("CREATE INDEX data_report ON data(id_report);");
    conn.exec_no_data("CREATE INDEX data_dt ON data(datetime);");
    conn.exec_no_data("CREATE INDEX data_lt ON data(id_lev_tr);");
    conn.exec_no_data(R"(
        CREATE TABLE attr (
           id_data     INTEGER NOT NULL REFERENCES data (id) ON DELETE CASCADE,
           type        INTEGER NOT NULL,
           value       VARCHAR(255) NOT NULL
        );
    )");
    conn.exec_no_data("CREATE UNIQUE INDEX attr_uniq ON attr(id_data, type);");
/*
 * Not a good idea: it works on ALL inserts, even on those that should fail
    "CREATE RULE data_insert_or_update AS "
    " ON INSERT TO data "
    " WHERE (new.id_context, new.id_var) IN ( "
    " SELECT id_context, id_var "
    " FROM data "
    " WHERE id_context=new.id_context AND id_var=new.id_var) "
    " DO INSTEAD "
    " UPDATE data SET value=new.value "
    " WHERE id_context=new.id_context AND id_var=new.id_var",
*/
    /*"CREATE FUNCTION identity (val anyelement, val1 anyelement, OUT val anyelement) AS 'select $2' LANGUAGE sql STRICT",
    "CREATE AGGREGATE anyval ( basetype=anyelement, sfunc='identity', stype='anyelement' )",*/
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

}
}
}
