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
#include "datav5.h"
#include "datav6.h"
#include "attrv5.h"
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

std::unique_ptr<sql::Repinfo> Driver::create_repinfov5()
{
    return unique_ptr<sql::Repinfo>(new ODBCRepinfoV5(conn));
}

std::unique_ptr<sql::Repinfo> Driver::create_repinfov6()
{
    return unique_ptr<sql::Repinfo>(new ODBCRepinfoV6(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv5()
{
    return unique_ptr<sql::Station>(new ODBCStationV5(conn));
}

std::unique_ptr<sql::Station> Driver::create_stationv6()
{
    return unique_ptr<sql::Station>(new ODBCStationV6(conn));
}

std::unique_ptr<sql::LevTr> Driver::create_levtrv6()
{
    return unique_ptr<sql::LevTr>(new ODBCLevTrV6(conn));
}

std::unique_ptr<sql::DataV5> Driver::create_datav5()
{
    return unique_ptr<sql::DataV5>(new ODBCDataV5(conn));
}

std::unique_ptr<sql::DataV6> Driver::create_datav6()
{
    return unique_ptr<sql::DataV6>(new ODBCDataV6(conn));
}

std::unique_ptr<sql::AttrV5> Driver::create_attrv5()
{
    return unique_ptr<sql::AttrV5>(new ODBCAttrV5(conn));
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

void Driver::run_delete_query_v6(const v6::QueryBuilder& qb)
{
    auto stm = conn.odbcstatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_in(1, qb.bind_in_ident);

    // Get the list of data to delete
    int out_id_data;
    stm->bind_out(1, out_id_data);

    // Compile the DELETE query for the data
    auto stmd = conn.odbcstatement("DELETE FROM data WHERE id=?");
    stmd->bind_in(1, out_id_data);

    // Compile the DELETE query for the attributes
    auto stma = conn.odbcstatement("DELETE FROM attr WHERE id_data=?");
    stma->bind_in(1, out_id_data);

    stm->execute();

    // Iterate all the data_id results, deleting the related data and attributes
    while (stm->fetch())
    {
        stmd->execute_ignoring_results();
        stma->execute_ignoring_results();
    }
}

}
}
}
