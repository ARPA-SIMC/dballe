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
    // Scan vars adding the State pointer to the current database values, if any
    vars.map_known_values();

    // Load the missing varcodes into the state and into vars
    if (!vars.to_query.empty())
    {
        Querybuf q(512);
        q.appendf("SELECT id, id_var, value FROM station_data WHERE id_station=%d AND id_var IN (", vars.shared_context.station->second.id);
        q.start_list(",");
        for (const auto& vi: vars.to_query)
            q.append_listf("%d", (int)vi->var->code());
        q.append(")");

        auto stm = conn.sqlitestatement(q);
        stm->execute([&]() {
            StationValueState vs;
            vs.value = stm->column_string(2);
            vs.id = stm->column_int(0);
            vs.is_new = false;
            wreport::Varcode code = stm->column_int(1);

            auto cur = t.state.add_stationvalue(StationValueDesc(vars.shared_context.station, code), vs);
            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [code](const bulk::StationVar* v) { return v->var->code() == code; });
            if (vi == vars.to_query.end()) return;
            (*vi)->cur = cur;
            vars.to_query.erase(vi);
        });
    }

    // Compute the action plan
    vars.compute_plan();

    // Execute the plan

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (vars.do_update)
            {
                auto update_stm = conn.sqlitestatement("UPDATE station_data SET value=? WHERE id=?");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    v.cur->second.value = v.var->enqc();
                    update_stm->bind(v.cur->second.value, v.cur->second.id);
                    update_stm->execute();
                    v.set_updated();
                }
            }
            break;
        case bulk::IGNORE:
            break;
        case bulk::ERROR:
            if (vars.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (vars.do_insert)
    {
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO station_data (id_station, id_var, value)
                 VALUES (%d, ?, ?)
        )", vars.shared_context.station->second.id);
        auto insert = conn.sqlitestatement(dq);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            StationValueState vs;
            vs.value = v.var->enqc();
            insert->bind(v.var->code(), vs.value);
            insert->execute();

            vs.id = conn.get_last_insert_id();
            vs.is_new = true;

            v.cur = t.state.add_stationvalue(StationValueDesc(vars.shared_context.station, v.var->code()), vs);
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
    istm = conn.sqlitestatement("INSERT INTO data (id_station, id_lev_tr, datetime, id_var, value) VALUES (?, ?, ?, ?, ?)").release();
    ustm = conn.sqlitestatement("UPDATE data SET value=? WHERE id=?").release();
}

SQLiteData::~SQLiteData()
{
    delete istm;
    delete ustm;
}

void SQLiteData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode)
{
    // Scan vars adding the State pointer to the current database values, if any
    vars.map_known_values();

    // Load the missing varcodes into the state and into vars
    // If the station has just been inserted, then there is nothing in the
    // database that is not already in State, and we can skip this part.
    if (!vars.to_query.empty())
    {
        const auto& dt = vars.shared_context.datetime;
        Querybuf q(512);
        q.appendf("SELECT id, id_lev_tr, id_var, value FROM data"
                  " WHERE id_station=%d AND datetime='%04d-%02d-%02d %02d:%02d:%02d'"
                  " AND id_lev_tr IN (",
                  vars.shared_context.station->second.id,
                  dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        q.start_list(",");
        for (const auto& v: vars.to_query)
            q.append_listf("%d", (int)v->levtr->second.id);
        q.append(") AND id_var IN (");
        q.start_list(",");
        for (const auto& v: vars.to_query)
            q.append_listf("%d", (int)v->var->code());
        q.append(")");

        auto stm = conn.sqlitestatement(q);
        stm->execute([&]() {
            int id_levtr = stm->column_int(1);
            wreport::Varcode code = stm->column_int(2);

            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [id_levtr, code](const bulk::Var* v) { return v->levtr->second.id == id_levtr && v->var->code() == code; });
            if (vi == vars.to_query.end()) return;

            ValueState vs;
            vs.value = stm->column_string(3);
            vs.id = stm->column_int(0);
            vs.is_new = false;

            (*vi)->cur = t.state.add_value(ValueDesc(vars.shared_context.station, (*vi)->levtr, vars.shared_context.datetime, code), vs);
            vars.to_query.erase(vi);
        });
    }

    // Compute the action plan
    vars.compute_plan();

    // Execute the plan

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (vars.do_update)
            {
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    v.cur->second.value = v.var->enqc();
                    ustm->bind(v.cur->second.value, v.cur->second.id);
                    ustm->execute();
                    v.set_updated();
                }
            }
            break;
        case bulk::IGNORE:
            break;
        case bulk::ERROR:
            if (vars.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (vars.do_insert)
    {
        istm->bind_val(1, vars.shared_context.station->second.id);
        istm->bind_val(3, vars.shared_context.datetime);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            ValueState vs;
            vs.value = v.var->enqc();
            istm->bind_val(2, v.levtr->second.id);
            istm->bind_val(4, v.var->code());
            istm->bind_val(5, vs.value);
            istm->execute();

            vs.id = conn.get_last_insert_id();
            vs.is_new = true;

            v.cur = t.state.add_value(ValueDesc(vars.shared_context.station, v.levtr, vars.shared_context.datetime, v.var->code()), vs);
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
