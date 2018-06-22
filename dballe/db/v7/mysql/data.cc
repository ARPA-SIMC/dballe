#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/db/v7/repinfo.h"
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

template class MySQLDataCommon<StationData>;
template class MySQLDataCommon<Data>;

template<typename Parent>
MySQLDataCommon<Parent>::MySQLDataCommon(v7::Transaction& tr, dballe::sql::MySQLConnection& conn)
    : Parent(tr), conn(conn)
{
}

template<typename Parent>
MySQLDataCommon<Parent>::~MySQLDataCommon()
{
}

template<typename Parent>
void MySQLDataCommon<Parent>::read_attrs(Tracer<>& trc, int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    char query[128];
    snprintf(query, 128, "SELECT attrs FROM %s WHERE id=%d", Parent::table_name, id_data);
    Tracer<> trc_sel(trc ? trc->trace_select(query) : nullptr);
    Values::decode(
            conn.exec_store(query).expect_one_result().as_blob(0),
            dest);
    if (trc_sel) trc_sel->add_row();
}

template<typename Parent>
void MySQLDataCommon<Parent>::write_attrs(Tracer<>& trc, int id_data, const Values& values)
{
    vector<uint8_t> encoded = values.encode();
    string escaped = conn.escape(encoded);
    Querybuf qb;
    qb.appendf("UPDATE %s SET attrs=X'%s' WHERE id=%d", Parent::table_name, escaped.c_str(), id_data);
    Tracer<> trc_upd(trc ? trc->trace_update(qb, 1) : nullptr);
    conn.exec_no_data(qb);
}

template<typename Parent>
void MySQLDataCommon<Parent>::remove_all_attrs(Tracer<>& trc, int id_data)
{
    char query[128];
    snprintf(query, 128, "UPDATE %s SET attrs=NULL WHERE id=%d", Parent::table_name, id_data);
    Tracer<> trc_upd(trc ? trc->trace_update(query, 1) : nullptr);
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

template<typename Parent>
void MySQLDataCommon<Parent>::remove(Tracer<>& trc, const v7::IdQueryBuilder& qb)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    std::unique_ptr<Varmatch> attr_filter;
    if (!qb.query.attr_filter.empty())
        attr_filter = Varmatch::parse(qb.query.attr_filter);

    Querybuf dq(512);
    dq.appendf("DELETE FROM %s WHERE id IN (", Parent::table_name);
    dq.start_list(",");
    unsigned count = 0;
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    auto res = conn.exec_store(qb.sql_query);
    while (auto row = res.fetch())
    {
        if (trc_sel) trc_sel->add_row();
        if (attr_filter.get() && !match_attrs(*attr_filter, row.as_blob(1))) return;

        // Note: if the query gets too long, we can split this in more DELETE
        // runs
        dq.append_list(row.as_cstring(0));
        ++count;
    }
    dq.append(")");
    if (count)
    {
        Tracer<> trc_del(trc ? trc->trace_delete(dq, count) : nullptr);
        conn.exec_no_data(dq);
    }
}

template<typename Parent>
void MySQLDataCommon<Parent>::update(Tracer<>& trc, std::vector<typename Parent::BatchValue>& vars, bool with_attrs)
{
    for (auto& v: vars)
    {
        string escaped_value = conn.escape(v.var->enqc());

        Querybuf qb;
        if (with_attrs && v.var->next_attr())
        {
            values::Encoder enc;
            enc.append_attributes(*v.var);
            string escaped_attrs = conn.escape(enc.buf);
            qb.appendf("UPDATE %s SET value='%s', attrs=X'%s' WHERE id=%d", Parent::table_name, escaped_value.c_str(), escaped_attrs.c_str(), v.id);
        }
        else
            qb.appendf("UPDATE %s SET value='%s', attrs=NULL WHERE id=%d", Parent::table_name, escaped_value.c_str(), v.id);
        Tracer<> trc_upd(trc ? trc->trace_update(qb, 1) : nullptr);
        conn.exec_no_data(qb);
    }
}


MySQLStationData::MySQLStationData(v7::Transaction& tr, MySQLConnection& conn)
    : MySQLDataCommon(tr, conn)
{
}

void MySQLStationData::query(Tracer<>& trc, int id_station, std::function<void(int id, wreport::Varcode code)> dest)
{
    char strquery[128];
    snprintf(strquery, 128, "SELECT id, code FROM station_data WHERE id_station=%d", id_station);
    Tracer<> trc_sel(trc ? trc->trace_select(strquery) : nullptr);
    auto res = conn.exec_store(strquery);
    while (auto row = res.fetch())
    {
        if (trc_sel) trc_sel->add_row();
        int id = row.as_int(0);
        wreport::Varcode code = row.as_int(1);
        dest(id, code);
    }
}

void MySQLStationData::insert(Tracer<>& trc, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        Querybuf qb;
        string escaped_value = conn.escape(v->var->enqc());
        if (with_attrs && v->var->next_attr())
        {
            values::Encoder enc;
            enc.append_attributes(*v->var);
            string escaped_attrs = conn.escape(enc.buf);
            qb.appendf("INSERT INTO station_data (id_station, code, value, attrs) VALUES (%d, %d, '%s', X'%s')",
                    id_station,
                    (int)v->var->code(),
                    escaped_value.c_str(),
                    escaped_attrs.c_str());
        }
        else
            qb.appendf("INSERT INTO station_data (id_station, code, value, attrs) VALUES (%d, %d, '%s', NULL)",
                    id_station,
                    (int)v->var->code(),
                    escaped_value.c_str());
        Tracer<> trc_ins(trc ? trc->trace_insert(qb, 1) : nullptr);
        conn.exec_no_data(qb);
        v->id = conn.get_last_insert_id();
    }
}

void MySQLStationData::run_station_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");

    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const sql::mysql::Row& row) {
        if (trc_sel) trc_sel->add_row();
        wreport::Varcode code = row.as_int(5);
        const char* value = row.as_cstring(7);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(row.as_blob(8), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = row.as_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = tr.repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_data = row.as_int(6);

        dest(station, id_data, move(var));
    });
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


MySQLData::MySQLData(v7::Transaction& tr, MySQLConnection& conn)
    : MySQLDataCommon(tr, conn)
{
}

void MySQLData::query(Tracer<>& trc, int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest)
{
    const auto& dt = datetime;
    char strquery[128];
    snprintf(strquery, 128, "SELECT id, id_levtr, code FROM data WHERE id_station=%d AND datetime='%04d-%02d-%02d %02d:%02d:%02d'",
            id_station, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    Tracer<> trc_sel(trc ? trc->trace_select(strquery) : nullptr);
    auto res = conn.exec_store(strquery);
    while (auto row = res.fetch())
    {
        if (trc_sel) trc_sel->add_row();
        int id_levtr = row.as_int(1);
        wreport::Varcode code = row.as_int(2);
        int id = row.as_int(0);
        dest(id, id_levtr, code);
    }
}

void MySQLData::insert(Tracer<>& trc, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        Querybuf qb;

        const auto& dt = datetime;
        string escaped_value = conn.escape(v->var->enqc());

        if (with_attrs && v->var->next_attr())
        {
            values::Encoder enc;
            enc.append_attributes(*v->var);
            string escaped_attrs = conn.escape(enc.buf);
            qb.appendf("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (%d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s', X'%s')",
                    id_station, v->id_levtr, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                    (int)v->var->code(),
                    escaped_value.c_str(),
                    escaped_attrs.c_str());
        }
        else
            qb.appendf("INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (%d, %d, '%04d-%02d-%02d %02d:%02d:%02d', %d, '%s', NULL)",
                    id_station, v->id_levtr, dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second,
                    (int)v->var->code(),
                    escaped_value.c_str());
        Tracer<> trc_ins(trc ? trc->trace_insert(qb, 1) : nullptr);
        conn.exec_no_data(qb);
        v->id = conn.get_last_insert_id();
    }
}

void MySQLData::run_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const sql::mysql::Row& row) {
        if (trc_sel) trc_sel->add_row();
        wreport::Varcode code = row.as_int(6);
        const char* value = row.as_cstring(9);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(row.as_blob(10), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = row.as_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = tr.repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_levtr = row.as_int(5);
        int id_data = row.as_int(7);
        Datetime datetime = row.as_datetime(8);

        dest(station, id_levtr, datetime, id_data, move(var));
    });
}

void MySQLData::run_summary_query(Tracer<>& trc, const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::Station& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)> dest)
{
    if (qb.bind_in_ident)
        throw error_unimplemented("binding in MySQL driver is not implemented");
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);

    dballe::Station station;
    conn.exec_use(qb.sql_query, [&](const sql::mysql::Row& row) {
        if (trc_sel) trc_sel->add_row();
        int id_station = row.as_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = tr.repinfo().get_rep_memo(row.as_int(1));
            station.coords.lat = row.as_int(2);
            station.coords.lon = row.as_int(3);
            if (row.isnull(4))
                station.ident.clear();
            else
                station.ident = row.as_string(4);
        }

        int id_levtr = row.as_int(5);
        wreport::Varcode code = row.as_int(6);

        size_t count = 0;
        DatetimeRange datetime;
        if (qb.select_summary_details)
        {
            count = row.as_int(7);
            datetime = DatetimeRange(row.as_datetime(8), row.as_datetime(9));
        }

        dest(station, id_levtr, code, datetime, count);
    });
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
