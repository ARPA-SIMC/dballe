#include "datav6.h"
#include "dballe/db/sql.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/core/record.h"
#include <algorithm>
#include <cstring>
#include <sqltypes.h>
#include <sql.h>

using namespace wreport;

namespace dballe {
namespace db {
namespace odbc {

ODBCDataV6::ODBCDataV6(ODBCConnection& conn)
    : conn(conn), istm(0), ustm(0), ioustm(0), sidstm(0)
{
    const char* insert_query =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* replace_query_mysql =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)"
        " ON DUPLICATE KEY UPDATE value=VALUES(value)";
// Do not use this: REPLACE is a DELETE+INSERT, which changes the primary key
//    const char* replace_query_sqlite =
//        "INSERT OR REPLACE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* replace_query_oracle =
        "MERGE INTO data USING"
        " (SELECT ? as station, ? as report, ? as lev_tr, ? as d, ? as var, ? as val FROM dual)"
        " ON (id_station=station AND id_report=report AND id_lev_tr=lev_tr AND datetime=d AND id_var=var)"
        " WHEN MATCHED THEN UPDATE SET value=val"
        " WHEN NOT MATCHED THEN"
        "  INSERT (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES (station, report, lev_tr, datetime, var, val)";
    const char* update_query =
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

    /* Override queries for some databases */
    switch (conn.server_type)
    {
        case ServerType::ORACLE:
            insert_query = "INSERT INTO data (id, id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(seq_data.NextVal, ?, ?, ?, ?, ?, ?)";
            seq_data = new Sequence(conn, "seq_data");
            break;
        default: break;
    }

    /* Create the statement for insert */
    istm = conn.odbcstatement(insert_query).release();
    istm->bind_in(1, id_station);
    istm->bind_in(2, id_report);
    istm->bind_in(3, id_lev_tr);
    istm->bind_in(4, date);
    istm->bind_in(5, id_var);
    istm->bind_in(6, value, value_ind);

    /* Create the statement for update */
    ustm = conn.odbcstatement(update_query).release();
    ustm->bind_in(1, value, value_ind);
    ustm->bind_in(2, id_station);
    ustm->bind_in(3, id_report);
    ustm->bind_in(4, id_lev_tr);
    ustm->bind_in(5, date);
    ustm->bind_in(6, id_var);

    /* Create the statement for replace (where available) */
    switch (conn.server_type)
    {
        case ServerType::MYSQL: ioustm = conn.odbcstatement(replace_query_mysql).release(); break;
        case ServerType::ORACLE: ioustm = conn.odbcstatement(replace_query_oracle).release(); break;
        default:
            break;
    }
    switch (conn.server_type)
    {
        case ServerType::MYSQL:
        case ServerType::ORACLE:
            ioustm->bind_in(1, id_station);
            ioustm->bind_in(2, id_report);
            ioustm->bind_in(3, id_lev_tr);
            ioustm->bind_in(4, date);
            ioustm->bind_in(5, id_var);
            ioustm->bind_in(6, value, value_ind);
            break;
        default:
            break;
    }

    /* Create the statement for select id */
    sidstm = conn.odbcstatement(select_id_query).release();
    sidstm->bind_in(1, id_station);
    sidstm->bind_in(2, id_report);
    sidstm->bind_in(3, id_lev_tr);
    sidstm->bind_in(4, date);
    sidstm->bind_in(5, id_var);
    sidstm->bind_out(1, id);
}

ODBCDataV6::~ODBCDataV6()
{
    delete seq_data;
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (ioustm) delete ioustm;
    if (sidstm) delete sidstm;
}

void ODBCDataV6::set_date(const Record& rec)
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

void ODBCDataV6::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date.year = ye;
    date.month = mo;
    date.day = da;
    date.hour = ho;
    date.minute = mi;
    date.second = se;
    date.fraction = 0;
}

void ODBCDataV6::set_station_info(int id_station, int id_report)
{
    this->id_station = id_station;
    this->id_report = id_report;
    // Use -1 instead of NULL, as NULL are considered different in UNIQUE
    // indices by some databases but not others, due to an ambiguity in the SQL
    // standard
    id_lev_tr = -1;
    date.year = 1000;
    date.month = 1;
    date.day = 1;
    date.hour = 0;
    date.minute = 0;
    date.second = 0;
    date.fraction = 0;
}

void ODBCDataV6::set(const wreport::Var& var)
{
    id_var = var.code();
    set_value(var.value());
}

void ODBCDataV6::set_value(const char* qvalue)
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

void ODBCDataV6::insert_or_fail(const wreport::Var& var, int* res_id)
{
    set(var);
    istm->execute_and_close();
    if (res_id)
    {
        if (seq_data)
            *res_id = seq_data->read();
        else
            *res_id = conn.get_last_insert_id();
    }
}

void ODBCDataV6::insert_or_overwrite(const wreport::Var& var, int* res_id)
{
    set(var);
    if (res_id)
    {
        // select id
        sidstm->execute();
        if (sidstm->fetch_expecting_one())
        {
            ustm->execute_and_close();
            *res_id = id;
        }
        else
        {
            istm->execute_and_close();
            if (seq_data)
                *res_id = seq_data->read();
            else
                *res_id = conn.get_last_insert_id();
        }
    } else {
        if (ioustm)
            // Try to use a single query where available
            ioustm->execute_and_close();
        else {
            if (ustm->execute_and_close() == SQL_NO_DATA)
                istm->execute_and_close();
        }
    }
}

void ODBCDataV6::insert(Transaction& t, sql::bulk::InsertV6& vars, bool update_existing)
{
    std::sort(vars.begin(), vars.end());

    Querybuf select_query(512);
    select_query.appendf(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=%d AND id_report=%d AND datetime={ts '%04d-%02d-%02d %02d:%02d:%02d'}
         ORDER BY id_lev_tr, id_var
    )", vars.id_station, vars.id_report,
        vars.datetime.date.year, vars.datetime.date.month, vars.datetime.date.day,
        vars.datetime.time.hour, vars.datetime.time.minute, vars.datetime.time.second);

    // Get the current status of variables for this context
    auto stm = conn.odbcstatement(select_query);

    int id_data;
    int id_levtr;
    int id_var;
    char value[255];
    stm->bind_out(1, id_data);
    stm->bind_out(2, id_levtr);
    stm->bind_out(3, id_var);
    stm->bind_out(4, value, 255);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateVarsV6 todo(vars);
    stm->execute();
    while (stm->fetch())
        todo.annotate(id_data, id_levtr, id_var, value);
    todo.annotate_end();

    // We now have a todo-list

    if (update_existing && todo.do_update)
    {
        auto update_stm = conn.odbcstatement("UPDATE data SET value=? WHERE id=?");
        for (auto& v: vars)
        {
            if (!v.needs_update()) continue;
            update_stm->bind_in(1, v.var->value());
            update_stm->bind_in(2, v.id_data);
            update_stm->execute_and_close();
            v.set_updated();
        }
    }

    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
                 VALUES (%d, %d, ?, '%04d-%02d-%02d %02d:%02d:%02d', ?, ?)
        )", vars.id_station, vars.id_report,
            vars.datetime.date.year, vars.datetime.date.month, vars.datetime.date.day,
            vars.datetime.time.hour, vars.datetime.time.minute, vars.datetime.time.second);
        auto insert = conn.odbcstatement(dq);
        int varcode;
        insert->bind_in(2, varcode);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            insert->bind_in(1, v.id_levtr);
            varcode = v.var->code();
            insert->bind_in(3, v.var->value());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void ODBCDataV6::remove(const v6::QueryBuilder& qb)
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


void ODBCDataV6::dump(FILE* out)
{
    int id;
    int id_station;
    int id_report;
    int id_lev_tr;
    SQL_TIMESTAMP_STRUCT date;
    wreport::Varcode id_var;
    char value[255];
    SQLLEN value_ind;

    auto stm = conn.odbcstatement("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    stm->bind_out(1, id);
    stm->bind_out(2, id_station);
    stm->bind_out(3, id_report);
    stm->bind_out(4, id_lev_tr);
    stm->bind_out(5, date);
    stm->bind_out(6, id_var);
    stm->bind_out(7, value, 255, value_ind);
    stm->execute();
    int count;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    for (count = 0; stm->fetch(); ++count)
    {
        char ltr[20];
        if (id_lev_tr == -1)
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
    stm->close_cursor();
}

}
}
}
