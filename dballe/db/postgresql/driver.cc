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

void Driver::run_built_query_v6(
        const v6::QueryBuilder& qb,
        std::function<void(sql::SQLRecordV6& rec)> dest)
{
    using namespace dballe::db::postgresql;

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
                rec.out_id_data = res.get_int4(row, output_seq++);
                rec.out_datetime = res.get_timestamp(row, output_seq++);
                rec.out_datetimemax = res.get_timestamp(row, output_seq++);
            }

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
#if 0
    auto stmd = conn.sqlitestatement("DELETE FROM data WHERE id=?");
    auto stma = conn.sqlitestatement("DELETE FROM attr WHERE id_data=?");
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Get the list of data to delete
        int out_id_data = stm->column_int(0);

        // Compile the DELETE query for the data
        stmd->bind_val(1, out_id_data);
        stmd->execute();

        // Compile the DELETE query for the attributes
        stma->bind_val(1, out_id_data);
        stma->execute();
    });
#endif
    throw error_unimplemented("delete V6");
}

}
}
}
