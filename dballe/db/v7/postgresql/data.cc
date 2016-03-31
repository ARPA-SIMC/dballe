#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/postgresql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using namespace dballe::sql::postgresql;
using dballe::sql::PostgreSQLConnection;
using dballe::sql::Querybuf;
using dballe::sql::error_postgresql;

namespace dballe {
namespace db {
namespace v7 {
namespace postgresql {

template class PostgreSQLDataCommon<StationDataTraits>;
template class PostgreSQLDataCommon<DataTraits>;

template<typename Traits>
PostgreSQLDataCommon<Traits>::PostgreSQLDataCommon(dballe::sql::PostgreSQLConnection& conn)
    : conn(conn)
{
}

template<typename Traits>
void PostgreSQLDataCommon<Traits>::read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
}

template<typename Traits>
void PostgreSQLDataCommon<Traits>::write_attrs(int id_data, const Values& values)
{
}

template<typename Traits>
void PostgreSQLDataCommon<Traits>::remove_all_attrs(int data_id)
{
}

template<typename Traits>
void PostgreSQLDataCommon<Traits>::remove(const v7::QueryBuilder& qb)
{
    Querybuf dq(512);
    dq.append("DELETE FROM ");
    dq.append(Traits::table_name);
    dq.append(" WHERE id IN (");
    dq.append(qb.sql_query);
    dq.append(")");
    if (qb.bind_in_ident)
    {
        conn.exec_no_data(dq.c_str(), qb.bind_in_ident);
    } else {
        conn.exec_no_data(dq.c_str());
    }
}


PostgreSQLStationData::PostgreSQLStationData(PostgreSQLConnection& conn)
    : PostgreSQLDataCommon(conn)
{
    conn.prepare("station_datav7_select", "SELECT id, code FROM station_data WHERE id_station=$1::int4");
}

void PostgreSQLStationData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
{
    // Scan vars adding the State pointer to the current database values, if any
    vars.map_known_values();

    // Load the missing varcodes into the state and into vars
    if (!vars.to_query.empty())
    {
        Result existing(conn.exec_prepared("station_datav7_select", vars.shared_context.station->second.id));
        for (unsigned row = 0; row < existing.rowcount(); ++row)
        {
            StationValueState vs(existing.get_int4(row, 0), false);
            wreport::Varcode code = (Varcode)existing.get_int4(row, 1);

            auto cur = t.state.add_stationvalue(StationValueDesc(vars.shared_context.station, code), vs);
            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [code](const bulk::StationVar* v) { return v->var->code() == code; });
            if (vi == vars.to_query.end()) return;
            (*vi)->cur = cur;
            vars.to_query.erase(vi);
        }
    }

    // Compute the action plan
    vars.compute_plan();

    // Execute the plan

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (vars.do_update)
            {
                Querybuf dq(512);
                dq.append("UPDATE station_data as d SET value=i.value FROM (values ");
                dq.start_list(",");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    const char* value = v.var->enqc();
                    char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
                    if (!escaped_val)
                        throw error_postgresql(conn, string("cannot escape string '") + value + "'");
                    dq.append_listf("(%d, %s)", v.cur->second.id, escaped_val);
                    PQfreemem(escaped_val);
                    v.set_updated();
                }
                dq.append(") AS i(id, value) WHERE d.id = i.id");
                //fprintf(stderr, "Update query: %s\n", dq.c_str());
                conn.exec_no_data(dq);
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
        dq.append("INSERT INTO station_data (id, id_station, code, value) VALUES ");
        dq.start_list(",");
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            const char* value = v.var->enqc();
            char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
            if (!escaped_val)
                throw error_postgresql(conn, string("cannot escape string '") + value + "'");
            dq.append_listf("(DEFAULT, %d, %d, %s)", vars.shared_context.station->second.id, (int)v.var->code(), escaped_val);
            PQfreemem(escaped_val);
        }
        dq.append(" RETURNING id");

        //fprintf(stderr, "Insert query: %s\n", dq.c_str());

        // Run the insert query and read back the new IDs
        Result res(conn.exec(dq));
        unsigned row = 0;
        auto v = vars.begin();
        while (row < res.rowcount() && v != vars.end())
        {
            if (!v->needs_insert())
            {
               ++v;
               continue;
            }

            StationValueState vs;
            vs.id = res.get_int4(row, 0);
            vs.is_new = true;

            v->cur = t.state.add_stationvalue(StationValueDesc(vars.shared_context.station, v->var->code()), vs);
            v->set_inserted();

            ++v;
            ++row;
        }
    }
}

void PostgreSQLStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec("SELECT id, id_station, code, value FROM station_data");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* val = res.is_null(row, 3) ? nullptr : res.get_string(row, 3);
        dumper.print_row(res.get_int4(row, 0), res.get_int4(row, 1), res.get_int4(row, 2), val);
    }
    dumper.print_tail();
}


PostgreSQLData::PostgreSQLData(PostgreSQLConnection& conn)
    : PostgreSQLDataCommon(conn)
{
    conn.prepare("datav7_select", "SELECT id, id_levtr, code FROM data WHERE id_station=$1::int4 AND datetime=$2::timestamp");
}

void PostgreSQLData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
{
    // Scan vars adding the State pointer to the current database values, if any
    vars.map_known_values();

    // Load the missing varcodes into the state and into vars
    // If the station has just been inserted, then there is nothing in the
    // database that is not already in State, and we can skip this part.
    if (!vars.to_query.empty())
    {
        Result existing(conn.exec_prepared("datav7_select", vars.shared_context.station->second.id, vars.shared_context.datetime));
        for (unsigned row = 0; row < existing.rowcount(); ++row)
        {
            ValueState vs(existing.get_int4(row, 0), false);
            int id_levtr = existing.get_int4(row, 1);
            wreport::Varcode code = (Varcode)existing.get_int4(row, 2);
            auto cur = t.state.add_value(ValueDesc(vars.shared_context.station, id_levtr, vars.shared_context.datetime, code), vs);

            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [id_levtr, code](const bulk::Var* v) { return v->levtr.id == id_levtr && v->var->code() == code; });
            if (vi == vars.to_query.end()) continue;
            (*vi)->cur = cur;
            vars.to_query.erase(vi);
        }
    }

    // Compute the action plan
    vars.compute_plan();

    // Execute the plan

    switch (update_mode)
    {
        case bulk::UPDATE:
            if (vars.do_update)
            {
                Querybuf dq(512);
                dq.append("UPDATE data as d SET value=i.value FROM (values ");
                dq.start_list(",");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    const char* value = v.var->enqc();
                    char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
                    if (!escaped_val)
                        throw error_postgresql(conn, string("cannot escape string '") + value + "'");
                    dq.start_list_item();
                    dq.append("(");
                    dq.append_int(v.cur->second.id);
                    dq.append(",");
                    dq.append(escaped_val);
                    dq.append(")");
                    PQfreemem(escaped_val);
                    v.set_updated();
                }
                dq.append(") AS i(id, value) WHERE d.id = i.id");
                // fprintf(stderr, "Update query: %s\n", dq.c_str());
                conn.exec_no_data(dq);
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
        const Datetime& dt = vars.shared_context.datetime;
        Querybuf dq(512);
        dq.append("INSERT INTO data (id, id_station, datetime, id_levtr, code, value) VALUES ");
        dq.start_list(",");

        char val_lead[64];
        snprintf(val_lead, 64, "(DEFAULT,%d,'%04d-%02d-%02d %02d:%02d:%02d',",
                    vars.shared_context.station->second.id,
                    dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            const char* value = v.var->enqc();
            char* escaped_val = PQescapeLiteral(conn, value, strlen(value));
            if (!escaped_val)
                throw error_postgresql(conn, string("cannot escape string '") + value + "'");
            dq.start_list_item();
            dq.append(val_lead);
            dq.append_int(v.levtr.id);
            dq.append(",");
            dq.append_int(v.var->code());
            dq.append(",");
            dq.append(escaped_val);
            dq.append(")");
            PQfreemem(escaped_val);
        }
        dq.append(" RETURNING id");

        // fprintf(stderr, "Insert query: %s\n", dq.c_str());

        // Run the insert query and read back the new IDs
        Result res(conn.exec(dq));
        unsigned row = 0;
        auto v = vars.begin();
        while (row < res.rowcount() && v != vars.end())
        {
            if (!v->needs_insert())
            {
               ++v;
               continue;
            }
            ValueState vs(res.get_int4(row, 0), true);
            v->cur = t.state.add_value(ValueDesc(vars.shared_context.station, v->levtr.id, dt, v->var->code()), vs);
            v->set_inserted();
            ++v;
            ++row;
        }
    }
}

void PostgreSQLData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec("SELECT id, id_station, id_levtr, datetime, code, value FROM data");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* val = res.is_null(row, 5) ? nullptr : res.get_string(row, 5);
        dumper.print_row(res.get_int4(row, 0), res.get_int4(row, 1), res.get_int4(row, 2), res.get_timestamp(row, 3), res.get_int4(row, 4), val);
    };
    dumper.print_tail();
}

}
}
}
}
