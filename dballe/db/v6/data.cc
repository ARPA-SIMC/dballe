/*
 * db/v6/data - data table management
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "data.h"
#include "db.h"
#include "dballe/db/internals.h"
#include <dballe/core/record.h>

#include <sql.h>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v6 {

Data::Data(DB& db)
    : db(db), istm(0), ustm(0), iistm(0)
{
    const char* insert_query =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* replace_query_mysql =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)"
        " ON DUPLICATE KEY UPDATE value=VALUES(value)";
    const char* replace_query_sqlite =
        "INSERT OR REPLACE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* replace_query_oracle =
        "MERGE INTO data USING"
        " (SELECT ? as station, ? as report, ? as lev_tr, ? as d, ? as var, ? as val FROM dual)"
        " ON (id_station=station AND id_report=report AND id_lev_tr=lev_tr AND datetime=d AND id_var=var)"
        " WHEN MATCHED THEN UPDATE SET value=val"
        " WHEN NOT MATCHED THEN"
        "  INSERT (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES (station, report, lev_tr, datetime, var, val)";
    const char* replace_query_postgres =
        "UPDATE data SET value=? WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";
    const char* insert_ignore_query_mysql =
        "INSERT IGNORE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* insert_ignore_query_sqlite =
        "INSERT OR IGNORE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* select_id_query =
        "SELECT id FROM data WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";
    /* FIXME: there is a useless WHEN MATCHED, but there does not seem a way to
     * have a MERGE with only a WHEN NOT, although on the internet one finds
     * several examples with it * /
    const char* insert_ignore_query_oracle =
        "MERGE INTO data USING"
        " (SELECT ? as cnt, ? as var, ? as val FROM dual)"
        " ON (id_context=cnt AND id_var=var)"
        " WHEN MATCHED THEN UPDATE SET value=value"
        " WHEN NOT MATCHED THEN"
        "  INSERT (id_context, id_var, value) VALUES (cnt, var, val)";
    */

    /* Create the statement for insert */
    istm = new db::Statement(*db.conn);
    istm->bind_in(1, id_station);
    istm->bind_in(2, id_report);
    istm->bind_in(3, id_lev_tr, id_lev_tr_ind);
    istm->bind_in(4, date);
    istm->bind_in(5, id_var);
    istm->bind_in(6, value, value_ind);
    istm->prepare(insert_query);

    /* Create the statement for replace */
    ustm = new db::Statement(*db.conn);
    if (db.conn->server_type == POSTGRES)
    {
        ustm->bind_in(1, value, value_ind);
        ustm->bind_in(2, id_station);
        ustm->bind_in(3, id_report);
        ustm->bind_in(4, id_lev_tr, id_lev_tr_ind);
        ustm->bind_in(5, date);
        ustm->bind_in(6, id_var);
    } else {
        ustm->bind_in(1, id_station);
        ustm->bind_in(2, id_report);
        ustm->bind_in(3, id_lev_tr, id_lev_tr_ind);
        ustm->bind_in(4, date);
        ustm->bind_in(5, id_var);
        ustm->bind_in(6, value, value_ind);
    }
    switch (db.conn->server_type)
    {
        case MYSQL: ustm->prepare(replace_query_mysql); break;
        case SQLITE: ustm->prepare(replace_query_sqlite); break;
        case ORACLE: ustm->prepare(replace_query_oracle); break;
        case POSTGRES: ustm->prepare(replace_query_postgres); break;
        default: ustm->prepare(replace_query_postgres); break;
    }

    /* Create the statement for insert ignore */
    iistm = new db::Statement(*db.conn);
    iistm->bind_in(1, id_station);
    iistm->bind_in(2, id_report);
    iistm->bind_in(3, id_lev_tr, id_lev_tr_ind);
    iistm->bind_in(4, date);
    iistm->bind_in(5, id_var);
    iistm->bind_in(6, value, value_ind);
    switch (db.conn->server_type)
    {
        case POSTGRES: iistm->prepare(insert_query); iistm->ignore_error = "23505"; break;
        case ORACLE: iistm->prepare(insert_query); iistm->ignore_error = "23000"; break;
        //case ORACLE: iistm->prepare(insert_ignore_query_oracle); break;
        case MYSQL: iistm->prepare(insert_ignore_query_mysql); break;
        case SQLITE: iistm->prepare(insert_ignore_query_sqlite); break;
        default: iistm->prepare(insert_ignore_query_sqlite); break;
    }

    /* Create the statement for select id */
    sidstm = new db::Statement(*db.conn);
    sidstm->bind_in(1, id_station);
    sidstm->bind_in(2, id_report);
    sidstm->bind_in(3, id_lev_tr, id_lev_tr_ind);
    sidstm->bind_in(4, date);
    sidstm->bind_in(5, id_var);
    sidstm->bind_out(1, id);
    sidstm->prepare(select_id_query);
}

Data::~Data()
{
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (iistm) delete iistm;
}

void Data::set_date(const Record& rec)
{
    /* Also input the seconds, defaulting to 0 if not found */
    const Var* year = rec.key_peek(DBA_KEY_YEAR);
    const Var* month = rec.key_peek(DBA_KEY_MONTH);
    const Var* day = rec.key_peek(DBA_KEY_DAY);
    const Var* hour = rec.key_peek(DBA_KEY_HOUR);
    const Var* min = rec.key_peek(DBA_KEY_MIN);
    const Var* sec = rec.key_peek(DBA_KEY_SEC);
    /* Datetime needs to be computed */
    if (year && month && day && hour && min)
    {
        date.year = year->enqi();
        date.month = month->enqi();
        date.day = day->enqi();
        date.hour = hour->enqi();
        date.minute = min->enqi();
        date.second = sec ? sec->enqi() : 0;
        date.fraction = 0;
    }
    else
        throw error_notfound("datetime informations not found among context information");
}

void Data::set_station_info()
{
    id_lev_tr = 0;
    id_lev_tr_ind = SQL_NULL_DATA;
    date.year = 1000;
    date.month = 1;
    date.day = 1;
    date.hour = 0;
    date.minute = 0;
    date.second = 0;
    date.fraction = 0;
}

void Data::set_id_lev_tr(int id)
{
    id_lev_tr = id;
    id_lev_tr_ind = sizeof(id_lev_tr);
}

void Data::set(const wreport::Var& var)
{
    id_var = var.code();
    set_value(var.value());
}

void Data::set_value(const char* qvalue)
{
    if (qvalue == NULL)
    {
        value[0] = 0;
        value_ind = SQL_NULL_DATA;
    } else {
        int len = strlen(qvalue);
        if (len > 255) len = 255;
        memcpy(value, qvalue, len);
        value[len] = 0;
        value_ind = len;
    }
}

void Data::insert_or_fail(bool want_id)
{
    istm->execute_and_close();
    if (want_id)
        id = db.last_data_insert_id();
}

bool Data::insert_or_ignore(bool want_id)
{
    int sqlres = iistm->execute();
    bool res;
    if (db.conn->server_type == POSTGRES || db.conn->server_type == ORACLE)
        res = ((sqlres == SQL_SUCCESS) || (sqlres == SQL_SUCCESS_WITH_INFO));
    else
        res = iistm->rowcount() != 0;
    iistm->close_cursor_if_needed();
    if (want_id)
        id = db.last_data_insert_id();
    return res;
}

void Data::insert_or_overwrite(bool want_id)
{
    if (want_id)
    {
        // select id
        sidstm->execute();
        if (sidstm->fetch_expecting_one())
            ;
        else
        {
            istm->execute_and_close();
            id = db.last_data_insert_id();
        }
    } else {
        if (db.conn->server_type == POSTGRES)
        {
            if (ustm->execute_and_close() == SQL_NO_DATA)
                istm->execute_and_close();
        } else
            ustm->execute_and_close();
    }
}

void Data::dump(FILE* out)
{
    DBALLE_SQL_C_SINT_TYPE id;
    DBALLE_SQL_C_SINT_TYPE id_station;
    DBALLE_SQL_C_SINT_TYPE id_report;
    DBALLE_SQL_C_SINT_TYPE id_lev_tr;
    SQLLEN id_lev_tr_ind;
    SQL_TIMESTAMP_STRUCT date;
    wreport::Varcode id_var;
    char value[255];
    SQLLEN value_ind;

    db::Statement stm(*(db.conn));
    stm.bind_out(1, id);
    stm.bind_out(2, id_station);
    stm.bind_out(3, id_report);
    stm.bind_out(4, id_lev_tr, id_lev_tr_ind);
    stm.bind_out(5, date);
    stm.bind_out(6, id_var);
    stm.bind_out(7, value, 255, value_ind);
    stm.exec_direct("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    int count;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    for (count = 0; stm.fetch(); ++count)
    {
        char ltr[20];
        if (id_lev_tr_ind == SQL_NULL_DATA)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", (int)id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %04d-%02d-%02d %02d:%02d:%02d.%d %01d%02d%03d",
                (int)id, (int)id_station, (int)id_report, ltr,
                (int)date.year, (int)date.month, (int)date.day,
                (int)date.hour, (int)date.minute, (int)date.second,
                (int)date.fraction,
                WR_VAR_F(id_var), WR_VAR_X(id_var), WR_VAR_Y(id_var));
        if (value_ind == SQL_NTS)
            fprintf(out, "\n");
        else
            fprintf(out, " %.*s\n", (int)value_ind, value);
    }
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
    stm.close_cursor();
}

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
