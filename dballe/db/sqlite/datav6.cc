#include "datav6.h"
#include "dballe/db/sql.h"
#include "dballe/db/querybuf.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/core/record.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sqlite {

SQLiteDataV6::SQLiteDataV6(SQLiteConnection& conn)
    : conn(conn)
{
    const char* insert_query =
        "INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* update_query =
        "UPDATE data SET value=? WHERE id=?";
        //"UPDATE data SET value=? WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";
    const char* insert_ignore_query =
        "INSERT OR IGNORE INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value) VALUES(?, ?, ?, ?, ?, ?)";
    const char* select_id_query =
        "SELECT id FROM data WHERE id_station=? AND id_report=? AND id_lev_tr=? AND datetime=? AND id_var=?";

    // Create the statement for insert
    istm = conn.sqlitestatement(insert_query).release();

    // Create the statement for update
    ustm = conn.sqlitestatement(update_query).release();

    // Create the statement for select id
    sidstm = conn.sqlitestatement(select_id_query).release();
}

SQLiteDataV6::~SQLiteDataV6()
{
    if (istm) delete istm;
    if (ustm) delete ustm;
    if (sidstm) delete sidstm;
}

void SQLiteDataV6::set_date(const Record& rec)
{
    date = rec.get_datetime();
}

void SQLiteDataV6::set_date(int ye, int mo, int da, int ho, int mi, int se)
{
    date = Datetime(ye, mo, da, ho, mi, se);
}

void SQLiteDataV6::set_station_info(int id_station, int id_report)
{
    this->id_station = id_station;
    this->id_report = id_report;
    // Use -1 instead of NULL, as NULL are considered different in UNIQUE
    // indices by some databases but not others, due to an ambiguity in the SQL
    // standard
    id_lev_tr = -1;
    date = Datetime(1000, 1, 1, 0, 0, 0);
}

void SQLiteDataV6::insert_or_fail(const wreport::Var& var, int* res_id)
{
    istm->bind_val(1, id_station);
    istm->bind_val(2, id_report);
    istm->bind_val(3, id_lev_tr);
    istm->bind_val(4, date);
    istm->bind_val(5, var.code());
    if (const char* val = var.value())
        istm->bind_val(6, val);
    else
        istm->bind_null_val(6);
    istm->execute();
    if (res_id) *res_id = conn.get_last_insert_id();
}

void SQLiteDataV6::insert_or_overwrite(const wreport::Var& var, int* res_id)
{
    // select id
    sidstm->bind_val(1, id_station);
    sidstm->bind_val(2, id_report);
    sidstm->bind_val(3, id_lev_tr);
    sidstm->bind_val(4, date);
    sidstm->bind_val(5, var.code());
    bool exists = false;
    int id;
    sidstm->execute([&]() {
        exists = true;
        id = sidstm->column_int(0);
    });

    if (exists)
    {
        if (const char* val = var.value())
            ustm->bind_val(1, val);
        else
            ustm->bind_null_val(1);
        ustm->bind_val(2, id);
        ustm->execute();
        if (res_id) *res_id = id;
    }
    else
    {
        istm->bind_val(1, id_station);
        istm->bind_val(2, id_report);
        istm->bind_val(3, id_lev_tr);
        istm->bind_val(4, date);
        istm->bind_val(5, var.code());
        if (const char* val = var.value())
            istm->bind_val(6, val);
        else
            istm->bind_null_val(6);
        istm->execute();
        if (res_id) *res_id = conn.get_last_insert_id();
    }
}

void SQLiteDataV6::insert(Transaction& t, sql::bulk::InsertV6& vars, bool update_existing)
{
    std::sort(vars.begin(), vars.end());

    const char* select_query = R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=? AND id_report=? AND datetime=?
         ORDER BY id_lev_tr, id_var
    )";

    // Get the current status of variables for this context
    auto stm = conn.sqlitestatement(select_query);
    stm->bind_val(1, vars.id_station);
    stm->bind_val(2, vars.id_report);
    stm->bind_val(3, vars.datetime);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    sql::bulk::AnnotateVarsV6 todo(vars);
    stm->execute([&]() {
        todo.annotate(
                stm->column_int(0),
                stm->column_int(1),
                stm->column_int(2),
                stm->column_string(3));
    });
    todo.annotate_end();

    // We now have a todo-list

    if (update_existing && todo.do_update)
    {
        auto update_stm = conn.sqlitestatement("UPDATE data SET value=? WHERE id=?");
        for (auto& v: vars)
        {
            if (!v.needs_update()) continue;
            update_stm->bind(v.var->value(), v.id_data);
            update_stm->execute();
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
        auto insert = conn.sqlitestatement(dq);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            insert->bind(v.id_levtr, v.var->code(), v.var->value());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteDataV6::remove(const v6::QueryBuilder& qb)
{
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
}

void SQLiteDataV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    stm->execute([&]() {
        int id_lev_tr = stm->column_int(3);
        const char* datetime = stm->column_string(4);
        Varcode code = stm->column_int(5);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %s %01d%02d%03d",
                stm->column_int(0),
                stm->column_int(1),
                stm->column_int(2),
                ltr,
                datetime,
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        if (stm->column_isnull(6))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(6));

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
