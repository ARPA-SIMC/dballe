#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/values.h"
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

template class SQLiteDataCommon<StationDataTraits>;
template class SQLiteDataCommon<DataTraits>;

template<typename Traits>
SQLiteDataCommon<Traits>::SQLiteDataCommon(dballe::sql::SQLiteConnection& conn)
    : conn(conn)
{
    char query[64];
    snprintf(query, 64, "UPDATE %s set value=?, attrs=? WHERE id=?", Traits::table_name);
    ustm = conn.sqlitestatement(query).release();
}

template<typename Traits>
SQLiteDataCommon<Traits>::~SQLiteDataCommon()
{
    delete read_attrs_stm;
    delete write_attrs_stm;
    delete remove_attrs_stm;
    delete sstm;
    delete istm;
    delete ustm;
}

template<typename Traits>
void SQLiteDataCommon<Traits>::read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    if (!read_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "SELECT attrs FROM %s WHERE id=?", Traits::table_name);
        read_attrs_stm = conn.sqlitestatement(query).release();
    }
    read_attrs_stm->bind_val(1, id_data);
    read_attrs_stm->execute_one([&]() {
        Values::decode(read_attrs_stm->column_blob(0), dest);
    });
}

template<typename Traits>
void SQLiteDataCommon<Traits>::write_attrs(int id_data, const Values& values)
{
    if (!write_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=? WHERE id=?", Traits::table_name);
        write_attrs_stm = conn.sqlitestatement(query).release();
    }
    vector<uint8_t> encoded = values.encode();
    write_attrs_stm->bind_val(1, encoded);
    write_attrs_stm->bind_val(2, id_data);
    write_attrs_stm->execute();
}

template<typename Traits>
void SQLiteDataCommon<Traits>::remove_all_attrs(int id_data)
{
    if (!remove_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=NULL WHERE id=?", Traits::table_name);
        remove_attrs_stm = conn.sqlitestatement(query).release();
    }
    remove_attrs_stm->bind_val(1, id_data);
    remove_attrs_stm->execute();
}

template<typename Traits>
void SQLiteDataCommon<Traits>::remove(const v7::QueryBuilder& qb)
{
    char query[64];
    snprintf(query, 64, "DELETE FROM %s WHERE id=?", Traits::table_name);
    auto stmd = conn.sqlitestatement(query);
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Compile the DELETE query for the data
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}


SQLiteStationData::SQLiteStationData(SQLiteConnection& conn)
    : SQLiteDataCommon(conn)
{
    sstm = conn.sqlitestatement("SELECT id, code FROM station_data WHERE id_station=?").release();
    istm = conn.sqlitestatement("INSERT INTO station_data (id_station, code, value, attrs) VALUES (?, ?, ?, ?)").release();
}

void SQLiteStationData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
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
                    ustm->bind_val(1, v.var->enqc());
                    values::Encoder enc;
                    if (with_attrs)
                    {
                        enc.append_attributes(*v.var);
                        ustm->bind_val(2, enc.buf);
                    }
                    else
                        ustm->bind_null_val(2);
                    ustm->bind_val(3, v.cur->second.id);

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
            values::Encoder enc;
            if (with_attrs)
            {
                enc.append_attributes(*v.var);
                istm->bind_val(4, enc.buf);
            }
            else
                istm->bind_null_val(4);
            istm->execute();

            StationValueState vs;
            vs.id = conn.get_last_insert_id();
            vs.is_new = true;

            v.cur = t.state.add_stationvalue(StationValueDesc(vars.shared_context.station, v.var->code()), vs);
            v.set_inserted();
        }
    }
}

void SQLiteStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, code, value FROM station_data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(3) ? nullptr : stm->column_string(3);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), val);
    });
    dumper.print_tail();
}


SQLiteData::SQLiteData(SQLiteConnection& conn)
    : SQLiteDataCommon(conn)
{
    sstm = conn.sqlitestatement("SELECT id, id_levtr, code FROM data WHERE id_station=? AND datetime=?").release();
    istm = conn.sqlitestatement("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (?, ?, ?, ?, ?, ?)").release();
}

void SQLiteData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
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
                    ustm->bind_val(1, v.var->enqc());
                    values::Encoder enc;
                    if (with_attrs)
                    {
                        enc.append_attributes(*v.var);
                        ustm->bind_val(2, enc.buf);
                    }
                    else
                        ustm->bind_null_val(2);
                    ustm->bind_val(3, v.cur->second.id);

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
            values::Encoder enc;
            if (with_attrs)
            {
                enc.append_attributes(*v.var);
                istm->bind_val(6, enc.buf);
            }
            else
                istm->bind_null_val(6);
            istm->execute();

            ValueState vs;
            vs.id = conn.get_last_insert_id();
            vs.is_new = true;

            v.cur = t.state.add_value(ValueDesc(vars.shared_context.station, v.levtr.id, vars.shared_context.datetime, v.var->code()), vs);
            v.set_inserted();
        }
    }
}

void SQLiteData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_levtr, datetime, code, value FROM data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(5) ? nullptr : stm->column_string(5);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), stm->column_datetime(3), stm->column_int(4), val);
    });
    dumper.print_tail();
}


}
}
}
}
