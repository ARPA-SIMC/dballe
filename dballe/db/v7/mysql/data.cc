#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/mysql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/values.h"
#include "dballe/core/varmatch.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::MySQLConnection;
using dballe::sql::MySQLStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {
namespace mysql {

template class MySQLDataCommon<StationDataTraits>;
template class MySQLDataCommon<DataTraits>;

template<typename Traits>
MySQLDataCommon<Traits>::MySQLDataCommon(dballe::sql::MySQLConnection& conn)
    : conn(conn)
{
}

template<typename Traits>
MySQLDataCommon<Traits>::~MySQLDataCommon()
{
}

template<typename Traits>
void MySQLDataCommon<Traits>::read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    char query[128];
    snprintf(query, 128, "SELECT attrs FROM %s WHERE id=%d", Traits::table_name, id_data);
    Values::decode(
            conn.exec_store(query).expect_one_result().as_blob(0),
            dest);
}

template<typename Traits>
void MySQLDataCommon<Traits>::write_attrs(int id_data, const Values& values)
{
    vector<uint8_t> encoded = values.encode();
    string escaped = conn.escape(encoded);
    Querybuf qb;
    qb.appendf("UPDATE %s SET attrs=X'%s' WHERE id=%d", Traits::table_name, escaped.c_str(), id_data);
    conn.exec_no_data(qb);
}

template<typename Traits>
void MySQLDataCommon<Traits>::remove_all_attrs(int id_data)
{
    char query[128];
    snprintf(query, 128, "UPDATE %s SET attrs=NULL WHERE id=%d", Traits::table_name, id_data);
    conn.exec_no_data(query);
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
void MySQLDataCommon<Traits>::remove(const v7::IdQueryBuilder& qb)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    std::unique_ptr<Varmatch> attr_filter;
    if (!qb.query.attr_filter.empty())
        attr_filter = Varmatch::parse(qb.query.attr_filter);

    Querybuf dq(512);
    dq.appendf("DELETE FROM %s WHERE id IN (", Traits::table_name);
    dq.start_list(",");
    bool found = false;
    auto res = conn.exec_store(qb.sql_query);
    while (auto row = res.fetch())
    {
        if (attr_filter.get() && !match_attrs(*attr_filter, row.as_blob(1))) return;

        // Note: if the query gets too long, we can split this in more DELETE
        // runs
        dq.append_list(row.as_cstring(0));
        found = true;
    }
    dq.append(")");
    if (found)
        conn.exec_no_data(dq);
}

MySQLStationData::MySQLStationData(MySQLConnection& conn)
    : MySQLDataCommon(conn)
{
}

void MySQLStationData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertStationVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
{
    vars.look_for_missing_ids();

    // Load the missing varcodes into the state and into vars
    if (!vars.to_query.empty())
    {
        char query[128];
        snprintf(query, 128, "SELECT id, code FROM station_data WHERE id_station=%d", vars.shared_context.station);
        auto res = conn.exec_store(query);
        while (auto row = res.fetch())
        {
            int id = row.as_int(0);
            wreport::Varcode code = row.as_int(1);
            cache.insert(unique_ptr<StationValueEntry>(new StationValueEntry(id, vars.shared_context.station, code)));
            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [code](const bulk::StationVar* v) { return v->var->code() == code; });
            if (vi == vars.to_query.end()) continue;
            (*vi)->id = id;
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
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;

                    string escaped_value = conn.escape(v.var->enqc());

                    Querybuf qb;
                    if (with_attrs && v.var->next_attr())
                    {
                        values::Encoder enc;
                        enc.append_attributes(*v.var);
                        string escaped_attrs = conn.escape(enc.buf);
                        qb.appendf("UPDATE station_data SET value='%s', attrs=X'%s' WHERE id=%d", escaped_value.c_str(), escaped_attrs.c_str(), v.id);
                    }
                    else
                        qb.appendf("UPDATE station_data SET value='%s', attrs=NULL WHERE id=%d", escaped_value.c_str(), v.id);
                    conn.exec_no_data(qb);
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
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            Querybuf qb;

            string escaped_value = conn.escape(v.var->enqc());

            if (with_attrs && v.var->next_attr())
            {
                values::Encoder enc;
                enc.append_attributes(*v.var);
                string escaped_attrs = conn.escape(enc.buf);
                qb.appendf("INSERT INTO station_data (id_station, code, value, attrs) VALUES (%d, %d, '%s', X'%s')",
                        vars.shared_context.station,
                        (int)v.var->code(),
                        escaped_value.c_str(),
                        escaped_attrs.c_str());
            }
            else
                qb.appendf("INSERT INTO station_data (id_station, code, value, attrs) VALUES (%d, %d, '%s', NULL)",
                        vars.shared_context.station,
                        (int)v.var->code(),
                        escaped_value.c_str());
            conn.exec_no_data(qb);

            v.id = conn.get_last_insert_id();
            cache.insert(unique_ptr<StationValueEntry>(new StationValueEntry(v.id, vars.shared_context.station, v.var->code())));
            // TODO: mark as newly inserted
            v.set_inserted();
        }
    }
}

void MySQLStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec_store("SELECT id, id_station, code, value, attrs FROM station_data");
    while (auto row = res.fetch())
    {
        const char* val = row.isnull(3) ? nullptr : row.as_cstring(3);
        dumper.print_row(row.as_int(0), row.as_int(1), row.as_int(2), val, row.as_blob(4));
    }
    dumper.print_tail();
}


MySQLData::MySQLData(MySQLConnection& conn)
    : MySQLDataCommon(conn)
{
}

void MySQLData::insert(dballe::db::v7::Transaction& t, v7::bulk::InsertVars& vars, bulk::UpdateMode update_mode, bool with_attrs)
{
    vars.look_for_missing_ids();

    // Load the missing varcodes into the state and into vars
    // If the station has just been inserted, then there is nothing in the
    // database that is not already in State, and we can skip this part.
    if (!vars.to_query.empty())
    {
        const auto& dt = vars.shared_context.datetime;
        char query[128];
        snprintf(query, 128, "SELECT id, id_levtr, code FROM data WHERE id_station=%d AND datetime='%04d-%02d-%02d %02d:%02d:%02d'",
                vars.shared_context.station,
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        auto res = conn.exec_store(query);
        while (auto row = res.fetch())
        {
            int id_levtr = row.as_int(1);
            wreport::Varcode code = row.as_int(2);
            int id = row.as_int(0);
            cache.insert(unique_ptr<ValueEntry>(new ValueEntry(id, vars.shared_context.station, id_levtr, vars.shared_context.datetime, code)));

            auto vi = std::find_if(vars.to_query.begin(), vars.to_query.end(), [id_levtr, code](const bulk::Var* v) { return v->id_levtr == id_levtr && v->var->code() == code; });
            if (vi == vars.to_query.end()) continue;
            (*vi)->id = id;
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
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;

                    string escaped_value = conn.escape(v.var->enqc());

                    Querybuf qb;
                    if (with_attrs && v.var->next_attr())
                    {
                        values::Encoder enc;
                        enc.append_attributes(*v.var);
                        string escaped_attrs = conn.escape(enc.buf);
                        qb.appendf("UPDATE data SET value='%s', attrs=X'%s' WHERE id=%d", escaped_value.c_str(), escaped_attrs.c_str(), v.id);
                    }
                    else
                        qb.appendf("UPDATE data SET value='%s', attrs=NULL WHERE id=%d", escaped_value.c_str(), v.id);
                    conn.exec_no_data(qb);
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
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;

            Querybuf qb;

            const auto& dt = vars.shared_context.datetime;
            string escaped_value = conn.escape(v.var->enqc());

            if (with_attrs && v.var->next_attr())
            {
                values::Encoder enc;
                enc.append_attributes(*v.var);
                string escaped_attrs = conn.escape(enc.buf);
                qb.appendf("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (%d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s', X'%s')",
                        vars.shared_context.station,
                        v.id_levtr,
                        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                        (int)v.var->code(),
                        escaped_value.c_str(),
                        escaped_attrs.c_str());
            }
            else
                qb.appendf("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (%d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s', NULL)",
                        vars.shared_context.station,
                        v.id_levtr,
                        dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                        (int)v.var->code(),
                        escaped_value.c_str());
            conn.exec_no_data(qb);

            v.id = conn.get_last_insert_id();
            cache.insert(unique_ptr<ValueEntry>(new ValueEntry(v.id, vars.shared_context.station, v.id_levtr, vars.shared_context.datetime, v.var->code())));
            v.set_inserted();
            // TODO: mark as newly inserted
        }
    }
}

void MySQLData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec_store("SELECT id, id_station, id_levtr, datetime, code, value, attrs FROM data");
    while (auto row = res.fetch())
    {
        const char* val = row.isnull(5) ? nullptr : row.as_cstring(5);
        dumper.print_row(row.as_int(0), row.as_int(1), row.as_int(2), row.as_datetime(3), row.as_int(4), val, row.as_blob(6));
    }
    dumper.print_tail();
}


}
}
}
}
