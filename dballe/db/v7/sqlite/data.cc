#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace sqlite {

SQLiteStationData::SQLiteStationData(SQLiteConnection& conn)
    : conn(conn)
{
    // Create the statement for select id
    sstm = conn.sqlitestatement(R"(
        SELECT id, id_var, value
          FROM station_data
         WHERE id_station=?
         ORDER BY id_var
    )").release();
}

SQLiteStationData::~SQLiteStationData()
{
    if (sstm) delete sstm;
}

void SQLiteStationData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode)
{
    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    v7::bulk::AnnotateStationVars todo(vars);

    if (!vars.station->second.is_new)
    {
        // Get the current status of variables for this context
        sstm->bind_val(1, vars.station->second.id);
        sstm->execute([&]() {
            todo.annotate(
                    sstm->column_int(0),
                    sstm->column_int(1),
                    sstm->column_string(2));
        });
    } else {
        // Annotate with the data already inserted in this transaction, so that
        // we update if it conflicts
        for (const auto& i: t.state.stationvalues)
            todo.annotate(i.second.id, i.first.var.code(), i.first.var.enqc());
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (todo.do_update)
            {
                auto update_stm = conn.sqlitestatement("UPDATE station_data SET value=? WHERE id=?");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    // Warning: we do not know if v.var is a string, so the result of
                    // enqc is only valid until another enqc is called
                    update_stm->bind(v.var->enqc(), v.id_data);
                    update_stm->execute();
                    v.set_updated();
                }
            }
            break;
        case bulk::IGNORE:
            break;
        case bulk::ERROR:
            if (todo.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO station_data (id_station, id_var, value)
                 VALUES (%d, ?, ?)
        )", vars.station->second.id);
        auto insert = conn.sqlitestatement(dq);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            // Warning: we do not know if v.var is a string, so the result of
            // enqc is only valid until another enqc is called
            insert->bind(v.var->code(), v.var->enqc());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteStationData::remove(const v7::QueryBuilder& qb)
{
    auto stmd = conn.sqlitestatement("DELETE FROM station_data WHERE id=?");
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Compile the DELETE query for the data
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}

void SQLiteStationData::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table station_data:\n");
    fprintf(out, " id   st   var\n");
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_var, value FROM station_data");
    stm->execute([&]() {
        Varcode code = stm->column_int(2);

        fprintf(out, " %4d %4d %01d%02d%03d",
                stm->column_int(0),
                stm->column_int(1),
                WR_VAR_FXY(code));
        if (stm->column_isnull(3))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(3));

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}


SQLiteData::SQLiteData(SQLiteConnection& conn)
    : conn(conn)
{
    // Create the statement for select id
    sstm = conn.sqlitestatement(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=? AND datetime=?
         ORDER BY id_lev_tr, id_var
    )").release();
}

SQLiteData::~SQLiteData()
{
    if (sstm) delete sstm;
}

void SQLiteData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode)
{
    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    v7::bulk::AnnotateVars todo(vars);

    if (!vars.station->second.is_new)
    {
        // Get the current status of variables for this context
        sstm->bind_val(1, vars.station->second.id);
        sstm->bind_val(2, vars.datetime);
        sstm->execute([&]() {
            todo.annotate(
                    sstm->column_int(0),
                    sstm->column_int(1),
                    sstm->column_int(2),
                    sstm->column_string(3));
        });
    } else {
        // TODO: Annotate is still needed, with the data already inserted in
        // this transaction, so that we update if it conflicts
    }
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (todo.do_update)
            {
                auto update_stm = conn.sqlitestatement("UPDATE data SET value=? WHERE id=?");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    // Warning: we do not know if v.var is a string, so the result of
                    // enqc is only valid until another enqc is called
                    update_stm->bind(v.var->enqc(), v.id_data);
                    update_stm->execute();
                    v.set_updated();
                }
            }
            break;
        case bulk::IGNORE:
            break;
        case bulk::ERROR:
            if (todo.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO data (id_station, id_lev_tr, datetime, id_var, value)
                 VALUES (%d, ?, '%04d-%02d-%02d %02d:%02d:%02d', ?, ?)
        )", vars.station->second.id,
            vars.datetime.year, vars.datetime.month, vars.datetime.day,
            vars.datetime.hour, vars.datetime.minute, vars.datetime.second);
        auto insert = conn.sqlitestatement(dq);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            // Warning: we do not know if v.var is a string, so the result of
            // enqc is only valid until another enqc is called
            insert->bind(v.id_levtr, v.var->code(), v.var->enqc());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteData::remove(const v7::QueryBuilder& qb)
{
    auto stmd = conn.sqlitestatement("DELETE FROM data WHERE id=?");
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Compile the DELETE query for the data
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}

void SQLiteData::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   ltr  datetime              var\n");
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_lev_tr, datetime, id_var, value FROM data");
    stm->execute([&]() {
        int id_lev_tr = stm->column_int(2);
        const char* datetime = stm->column_string(3);
        Varcode code = stm->column_int(4);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        fprintf(out, " %4d %4d %s %s %01d%02d%03d",
                stm->column_int(0),
                stm->column_int(1),
                ltr,
                datetime,
                WR_VAR_FXY(code));
        if (stm->column_isnull(5))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(5));

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
}
