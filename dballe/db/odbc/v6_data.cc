#include "v6_data.h"
#include "dballe/db/sql.h"
#include "dballe/db/odbcworkarounds.h"
#include "dballe/db/v6/db.h"
#include "dballe/core/record.h"
#include <sqltypes.h>
#include <sql.h>
#include <cstring>

using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

ODBCData::ODBCData(DB& db)
    : db(db), istm(0), ustm(0), ioustm(0), iistm(0), sidstm(0)
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

    /* Create the statement for insert */
    istm = new db::Statement(*db.conn);
    istm->bind_in(1, id_station);
    istm->bind_in(2, id_report);
    istm->bind_in(3, id_lev_tr);
    istm->bind_in(4, date);
    istm->bind_in(5, id_var);
    istm->bind_in(6, value, value_ind);
    istm->prepare(insert_query);

    /* Create the statement for update */
    ustm = new db::Statement(*db.conn);
    ustm->bind_in(1, value, value_ind);
    ustm->bind_in(2, id_station);
    ustm->bind_in(3, id_report);
    ustm->bind_in(4, id_lev_tr);
    ustm->bind_in(5, date);
    ustm->bind_in(6, id_var);
    ustm->prepare(update_query);

    /* Create the statement for replace (where available) */
    switch (db.conn->server_type)
    {
        case MYSQL:
        case ORACLE:
            ioustm = new db::Statement(*db.conn);
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
    switch (db.conn->server_type)
    {
        case MYSQL: ioustm->prepare(replace_query_mysql); break;
        case ORACLE: ioustm->prepare(replace_query_oracle); break;
        default:
            break;
    }

    /* Create the statement for insert ignore */
    iistm = new db::Statement(*db.conn);
    iistm->bind_in(1, id_station);
    iistm->bind_in(2, id_report);
    iistm->bind_in(3, id_lev_tr);
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
    sidstm->bind_in(3, id_lev_tr);
    sidstm->bind_in(4, date);
    sidstm->bind_in(5, id_var);
    sidstm->bind_out(1, id);
    sidstm->prepare(select_id_query);
}

ODBCData::~ODBCData()
{
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (ioustm) delete ioustm;
    if (iistm) delete iistm;
    if (sidstm) delete sidstm;
}

void ODBCData::set_context(int id_station, int id_report, int id_lev_tr)
{
    this->id_station = id_station;
    this->id_report = id_report;
    this->id_lev_tr = id_lev_tr;
}

void ODBCData::set_date(const Record& rec)
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

void ODBCData::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date.year = ye;
    date.month = mo;
    date.day = da;
    date.hour = ho;
    date.minute = mi;
    date.second = se;
    date.fraction = 0;
}

void ODBCData::set_station_info(int id_station, int id_report)
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

void ODBCData::set(const wreport::Var& var)
{
    id_var = var.code();
    set_value(var.value());
}

void ODBCData::set_value(const char* qvalue)
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

void ODBCData::insert_or_fail(const wreport::Var& var, int* res_id)
{
    set(var);
    istm->execute_and_close();
    if (res_id)
        *res_id = db.last_data_insert_id();
}

bool ODBCData::insert_or_ignore(const wreport::Var& var, int* res_id)
{
    set(var);
    int sqlres = iistm->execute();
    bool res;
    if (db.conn->server_type == POSTGRES || db.conn->server_type == ORACLE)
        res = ((sqlres == SQL_SUCCESS) || (sqlres == SQL_SUCCESS_WITH_INFO));
    else
        res = iistm->rowcount() != 0;
    iistm->close_cursor_if_needed();
    if (res_id)
        *res_id = db.last_data_insert_id();
    return res;
}

void ODBCData::insert_or_overwrite(const wreport::Var& var, int* res_id)
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
            *res_id = db.last_data_insert_id();
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

void ODBCData::dump(FILE* out)
{
    DBALLE_SQL_C_SINT_TYPE id;
    DBALLE_SQL_C_SINT_TYPE id_station;
    DBALLE_SQL_C_SINT_TYPE id_report;
    DBALLE_SQL_C_SINT_TYPE id_lev_tr;
    SQL_TIMESTAMP_STRUCT date;
    wreport::Varcode id_var;
    char value[255];
    SQLLEN value_ind;

    db::Statement stm(*(db.conn));
    stm.bind_out(1, id);
    stm.bind_out(2, id_station);
    stm.bind_out(3, id_report);
    stm.bind_out(4, id_lev_tr);
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
    stm.close_cursor();
}

}
}
}
