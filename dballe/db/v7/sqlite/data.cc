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
    sstm = conn.sqlitestatement("SELECT id, code FROM station_data WHERE id_station=?").release();
    istm = conn.sqlitestatement("INSERT INTO station_data (id_station, code, value) VALUES (?, ?, ?)").release();
    ustm = conn.sqlitestatement("UPDATE station_data SET value=? WHERE id=?").release();
}

SQLiteStationData::~SQLiteStationData()
{
    delete sstm;
    delete istm;
    delete ustm;
}

void SQLiteStationData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode)
{
    // Scan vars adding the State pointer to the current database values, if any
    vars.map_known_values();

    // Load the missing varcodes into the state and into vars
    if (!vars.to_query.empty())
    {
        sstm->bind_val(1, vars.shared_context.station->second.id);
        sstm->execute([&]() {
            StationValueState vs;
            vs.id = sstm->column_int(0);
            vs.is_new = false;
            wreport::Varcode code = sstm->column_int(1);

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
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    ustm->bind(v.var->enqc(), v.cur->second.id);
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
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            istm->bind_val(2, v.var->code());
            istm->bind_val(3, v.var->enqc());
            istm->execute();

            StationValueState vs;
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
    auto stm = conn.sqlitestatement("SELECT id, id_station, code, value FROM station_data");
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
    sstm = conn.sqlitestatement("SELECT id, id_levtr, code FROM data WHERE id_station=? AND datetime=?").release();
    istm = conn.sqlitestatement("INSERT INTO data (id_station, id_levtr, datetime, code, value) VALUES (?, ?, ?, ?, ?)").release();
    ustm = conn.sqlitestatement("UPDATE data SET value=? WHERE id=?").release();
}

SQLiteData::~SQLiteData()
{
    delete sstm;
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
        sstm->bind_val(1, vars.shared_context.station->second.id);
        sstm->bind_val(2, vars.shared_context.datetime);
        sstm->execute([&]() {
            int id_levtr = sstm->column_int(1);
            wreport::Varcode code = sstm->column_int(2);

            ValueState vs;
            vs.id = sstm->column_int(0);
            vs.is_new = false;
            auto cur = t.state.add_value(ValueDesc(vars.shared_context.station, id_levtr, vars.shared_context.datetime, code), vs);

            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [id_levtr, code](const bulk::Var* v) { return v->levtr.id == id_levtr && v->var->code() == code; });
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
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    ustm->bind(v.var->enqc(), v.cur->second.id);
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
            istm->bind_val(2, v.levtr.id);
            istm->bind_val(4, v.var->code());
            istm->bind_val(5, v.var->enqc());
            istm->execute();

            ValueState vs;
            vs.id = conn.get_last_insert_id();
            vs.is_new = true;

            v.cur = t.state.add_value(ValueDesc(vars.shared_context.station, v.levtr.id, vars.shared_context.datetime, v.var->code()), vs);
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
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_levtr, datetime, code, value FROM data");
    stm->execute([&]() {
        int id_levtr = stm->column_int(2);
        const char* datetime = stm->column_string(3);
        Varcode code = stm->column_int(4);

        char ltr[20];
        if (id_levtr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_levtr);

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
