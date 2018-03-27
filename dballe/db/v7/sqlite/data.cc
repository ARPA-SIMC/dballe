#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/values.h"
#include "dballe/core/varmatch.h"
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

namespace {

bool match_attrs(const Varmatch& match, const std::vector<uint8_t>& attrs)
{
    bool found = false;
    Values::decode(attrs, [&](std::unique_ptr<wreport::Var> var) {
        if (match(*var))
            found = true;
    });
    return found;
}

}

template<typename Traits>
void SQLiteDataCommon<Traits>::remove(const v7::IdQueryBuilder& qb)
{
    char query[64];
    snprintf(query, 64, "DELETE FROM %s WHERE id=?", Traits::table_name);
    auto stmd = conn.sqlitestatement(query);
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    std::unique_ptr<Varmatch> attr_filter;
    if (!qb.query.attr_filter.empty())
        attr_filter = Varmatch::parse(qb.query.attr_filter);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        if (attr_filter.get() && !match_attrs(*attr_filter, stm->column_blob(1))) return;

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

void SQLiteStationData::insert(dballe::db::v7::Transaction& tr, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
{
    vars.look_for_missing_ids();

    // Load the missing varcodes into the state and into vars
    if (!vars.to_query.empty())
    {
        sstm->bind_val(1, vars.shared_context.station);
        sstm->execute([&]() {
            int id = sstm->column_int(0);
            wreport::Varcode code = sstm->column_int(1);
            //cache.insert(unique_ptr<StationValueEntry>(new StationValueEntry(id, vars.shared_context.station, code)));
            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [code](const bulk::StationVar* v) { return v->var->code() == code; });
            if (vi == vars.to_query.end()) return;
            (*vi)->id = id;
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
                    if (with_attrs && v.var->next_attr())
                    {
                        enc.append_attributes(*v.var);
                        ustm->bind_val(2, enc.buf);
                    }
                    else
                        ustm->bind_null_val(2);
                    ustm->bind_val(3, v.id);

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
        istm->bind_val(1, vars.shared_context.station);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            istm->bind_val(2, v.var->code());
            istm->bind_val(3, v.var->enqc());
            values::Encoder enc;
            if (with_attrs && v.var->next_attr())
            {
                enc.append_attributes(*v.var);
                istm->bind_val(4, enc.buf);
            }
            else
                istm->bind_null_val(4);
            istm->execute();

            v.id = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, code, value, attrs FROM station_data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(3) ? nullptr : stm->column_string(3);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), val, stm->column_blob(4));
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
    vars.look_for_missing_ids();

    // Load the missing varcodes into the state and into vars
    // If the station has just been inserted, then there is nothing in the
    // database that is not already in State, and we can skip this part.
    if (!vars.to_query.empty())
    {
        sstm->bind_val(1, vars.shared_context.station);
        sstm->bind_val(2, vars.shared_context.datetime);
        sstm->execute([&]() {
            int id_levtr = sstm->column_int(1);
            wreport::Varcode code = sstm->column_int(2);

            int id = sstm->column_int(0);
            //cache.insert(unique_ptr<ValueEntry>(new ValueEntry(id, vars.shared_context.station, id_levtr, vars.shared_context.datetime, code)));

            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [id_levtr, code](const bulk::Var* v) { return v->id_levtr == id_levtr && v->var->code() == code; });
            if (vi == vars.to_query.end()) return;
            (*vi)->id = id;
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
                    if (with_attrs && v.var->next_attr())
                    {
                        enc.append_attributes(*v.var);
                        ustm->bind_val(2, enc.buf);
                    }
                    else
                        ustm->bind_null_val(2);
                    ustm->bind_val(3, v.id);

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
        istm->bind_val(1, vars.shared_context.station);
        istm->bind_val(3, vars.shared_context.datetime);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            istm->bind_val(2, v.id_levtr);
            istm->bind_val(4, v.var->code());
            istm->bind_val(5, v.var->enqc());
            values::Encoder enc;
            if (with_attrs && v.var->next_attr())
            {
                enc.append_attributes(*v.var);
                istm->bind_val(6, enc.buf);
            }
            else
                istm->bind_null_val(6);
            istm->execute();

            v.id = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_levtr, datetime, code, value, attrs FROM data");
    stm->execute([&]() {
        const char* val = stm->column_isnull(5) ? nullptr : stm->column_string(5);
        dumper.print_row(stm->column_int(0), stm->column_int(1), stm->column_int(2), stm->column_datetime(3), stm->column_int(4), val, stm->column_blob(6));
    });
    dumper.print_tail();
}

}
}
}
}
