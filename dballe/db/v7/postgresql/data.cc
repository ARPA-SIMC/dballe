#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/sql/postgresql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include "dballe/core/values.h"
#include "dballe/core/varmatch.h"
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

template class PostgreSQLDataCommon<StationData>;
template class PostgreSQLDataCommon<Data>;

template<typename Parent>
PostgreSQLDataCommon<Parent>::PostgreSQLDataCommon(dballe::sql::PostgreSQLConnection& conn)
    : conn(conn)
{
}

template<typename Parent>
void PostgreSQLDataCommon<Parent>::read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    if (select_attrs_query_name.empty())
    {
        select_attrs_query_name = Parent::table_name;
        select_attrs_query_name += "v7_select_attrs";
        char query[64];
        snprintf(query, 64, "SELECT attrs FROM %s WHERE id=$1::int4", Parent::table_name);
        conn.prepare(select_attrs_query_name, query);
    }
    Values::decode(
            conn.exec_prepared_one_row(select_attrs_query_name, id_data).get_bytea(0, 0),
            dest);
}

template<typename Parent>
void PostgreSQLDataCommon<Parent>::write_attrs(int id_data, const Values& values)
{
    if (write_attrs_query_name.empty())
    {
        write_attrs_query_name = Parent::table_name;
        write_attrs_query_name += "v7_write_attrs";
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=$1::bytea WHERE id=$2::int4", Parent::table_name);
        conn.prepare(write_attrs_query_name, query);
    }
    vector<uint8_t> encoded = values.encode();
    conn.exec_prepared_no_data(write_attrs_query_name, encoded, id_data);
}

template<typename Parent>
void PostgreSQLDataCommon<Parent>::remove_all_attrs(int id_data)
{
    if (remove_attrs_query_name.empty())
    {
        remove_attrs_query_name = Parent::table_name;
        remove_attrs_query_name += "v7_remove_attrs";
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=NULL WHERE id=$1::int4", Parent::table_name);
        conn.prepare(remove_attrs_query_name, query);
    }
    conn.exec_prepared_no_data(remove_attrs_query_name, id_data);
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

template<typename Parent>
void PostgreSQLDataCommon<Parent>::remove(const v7::IdQueryBuilder& qb)
{
    if (!qb.query.attr_filter.empty())
    {
        // We need to apply attr_filter to all results of the query, so we
        // iterate the results and delete the matching ones one by one.
        std::unique_ptr<Varmatch> attr_filter = Varmatch::parse(qb.query.attr_filter);
        if (remove_data_query_name.empty())
        {
            remove_data_query_name = Parent::table_name;
            remove_data_query_name += "v7_remove_data";
            char query[64];
            snprintf(query, 64, "DELETE FROM %s WHERE id=$1::int4", Parent::table_name);
            conn.prepare(remove_data_query_name, query);
        }

        Result to_remove;
        if (qb.bind_in_ident)
            to_remove = conn.exec(qb.sql_query, qb.bind_in_ident);
        else
            to_remove = conn.exec(qb.sql_query);
        for (unsigned row = 0; row < to_remove.rowcount(); ++row)
        {
            if (!match_attrs(*attr_filter, to_remove.get_bytea(row, 1))) return;
            conn.exec_prepared(remove_data_query_name, (int)to_remove.get_int4(row, 0));
        }
    } else {
        Querybuf dq(512);
        dq.append("DELETE FROM ");
        dq.append(Parent::table_name);
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
}

template<typename Parent>
void PostgreSQLDataCommon<Parent>::update(dballe::db::v7::Transaction& t, std::vector<typename Parent::BatchValue>& vars, bool with_attrs)
{
    Querybuf qb(512);
    if (with_attrs)
    {
        qb.append("UPDATE ");
        qb.append(Parent::table_name);
        qb.append(" as d SET value=i.value, attrs=i.attrs FROM (values ");
        qb.start_list(",");
        for (auto& v: vars)
        {
            qb.start_list_item();
            qb.append("(");
            qb.append_int(v.id);
            qb.append(",");
            conn.append_escaped(qb, v.var->enqc());
            qb.append(",");
            if (v.var->next_attr())
            {
                values::Encoder enc;
                enc.append_attributes(*v.var);
                conn.append_escaped(qb, enc.buf);
            } else
                qb.append("NULL");
            qb.append("::bytea)");
        }
        qb.append(") AS i(id, value, attrs) WHERE d.id = i.id");
    } else {
        qb.append("UPDATE ");
        qb.append(Parent::table_name);
        qb.append(" as d SET value=i.value, attrs=NULL FROM (values ");
        qb.start_list(",");
        for (auto& v: vars)
        {
            qb.start_list_item();
            qb.append("(");
            qb.append_int(v.id);
            qb.append(",");
            conn.append_escaped(qb, v.var->enqc());
            qb.append(")");
        }
        qb.append(") AS i(id, value) WHERE d.id = i.id");
    }
    //fprintf(stderr, "Update query: %s\n", dq.c_str());
    conn.exec_no_data(qb);
}


PostgreSQLStationData::PostgreSQLStationData(PostgreSQLConnection& conn)
    : PostgreSQLDataCommon(conn)
{
    conn.prepare("station_datav7_select", "SELECT id, code FROM station_data WHERE id_station=$1::int4");
}

void PostgreSQLStationData::query(int id_station, std::function<void(int id, wreport::Varcode code)> dest)
{
    Result existing(conn.exec_prepared("station_datav7_select", id_station));
    for (unsigned row = 0; row < existing.rowcount(); ++row)
    {
        int id = existing.get_int4(row, 0);
        wreport::Varcode code = (Varcode)existing.get_int4(row, 1);
        dest(id, code);
    }
}

void PostgreSQLStationData::insert(dballe::db::v7::Transaction& t, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());

    char lead[64];
    snprintf(lead, 64, "(DEFAULT,%d,", id_station);

    Querybuf dq(512);
    dq.append("INSERT INTO station_data (id, id_station, code, value, attrs) VALUES ");
    dq.start_list(",");
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        dq.start_list_item();
        dq.append(lead);
        dq.append_int(v->var->code());
        dq.append(",");
        conn.append_escaped(dq, v->var->enqc());
        dq.append(",");
        if (with_attrs && v->var->next_attr())
        {
            values::Encoder enc;
            enc.append_attributes(*v->var);
            conn.append_escaped(dq, enc.buf);
        } else
            dq.append("NULL::bytea");
        dq.append(")");
    }
    dq.append(" RETURNING id");

    //fprintf(stderr, "Insert query: %s\n", dq.c_str());

    // Run the insert query and read back the new IDs
    Result res(conn.exec(dq));
    unsigned row = 0;
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        if (row >= res.rowcount()) break;
        v->id = res.get_int4(row, 0);
        ++row;
    }
}

void PostgreSQLStationData::dump(FILE* out)
{
    StationDataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec("SELECT id, id_station, code, value, attrs FROM station_data");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* val = res.is_null(row, 3) ? nullptr : res.get_string(row, 3);
        dumper.print_row(res.get_int4(row, 0), res.get_int4(row, 1), res.get_int4(row, 2), val, res.get_bytea(row, 4));
    }
    dumper.print_tail();
}


PostgreSQLData::PostgreSQLData(PostgreSQLConnection& conn)
    : PostgreSQLDataCommon(conn)
{
    conn.prepare("datav7_select", "SELECT id, id_levtr, code FROM data WHERE id_station=$1::int4 AND datetime=$2::timestamp");
}

void PostgreSQLData::query(int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest)
{
    Result existing(conn.exec_prepared("datav7_select", id_station, datetime));
    for (unsigned row = 0; row < existing.rowcount(); ++row)
    {
        int id = existing.get_int4(row, 0);
        int id_levtr = existing.get_int4(row, 1);
        wreport::Varcode code = (Varcode)existing.get_int4(row, 2);
        dest(id, id_levtr, code);
    }
}

void PostgreSQLData::insert(dballe::db::v7::Transaction& t, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());

    const Datetime& dt = datetime;
    char val_lead[64];
    snprintf(val_lead, 64, "(DEFAULT,%d,'%04d-%02d-%02d %02d:%02d:%02d',",
                id_station, 
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    Querybuf dq(512);
    dq.append("INSERT INTO data (id, id_station, datetime, id_levtr, code, value, attrs) VALUES ");
    dq.start_list(",");
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        dq.start_list_item();
        dq.append(val_lead);
        dq.append_int(v->id_levtr);
        dq.append(",");
        dq.append_int(v->var->code());
        dq.append(",");
        conn.append_escaped(dq, v->var->enqc());
        dq.append(",");
        if (with_attrs && v->var->next_attr())
        {
            values::Encoder enc;
            enc.append_attributes(*v->var);
            conn.append_escaped(dq, enc.buf);
        } else
            dq.append("NULL::bytea");
        dq.append(")");
    }
    dq.append(" RETURNING id");

    // fprintf(stderr, "Insert query: %s\n", dq.c_str());

    // Run the insert query and read back the new IDs
    Result res(conn.exec(dq));
    unsigned row = 0;
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        if (row >= res.rowcount()) break;
        v->id = res.get_int4(row, 0);
        ++row;
    }
}

void PostgreSQLData::dump(FILE* out)
{
    DataDumper dumper(out);

    dumper.print_head();
    auto res = conn.exec("SELECT id, id_station, id_levtr, datetime, code, value, attrs FROM data");
    for (unsigned row = 0; row < res.rowcount(); ++row)
    {
        const char* val = res.is_null(row, 5) ? nullptr : res.get_string(row, 5);
        dumper.print_row(res.get_int4(row, 0), res.get_int4(row, 1), res.get_int4(row, 2), res.get_timestamp(row, 3), res.get_int4(row, 4), val, res.get_bytea(row, 6));
    };
    dumper.print_tail();
}

}
}
}
}
