#include "data.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/qbuilder.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/trace.h"
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

template class SQLiteDataCommon<StationData>;
template class SQLiteDataCommon<Data>;

template<typename Parent>
SQLiteDataCommon<Parent>::SQLiteDataCommon(v7::Transaction& tr, dballe::sql::SQLiteConnection& conn)
    : Parent(tr), conn(conn)
{
    char query[64];
    snprintf(query, 64, "UPDATE %s set value=?, attrs=? WHERE id=?", Parent::table_name);
    ustm = conn.sqlitestatement(query).release();
}

template<typename Parent>
SQLiteDataCommon<Parent>::~SQLiteDataCommon()
{
    delete read_attrs_stm;
    delete write_attrs_stm;
    delete remove_attrs_stm;
    delete sstm;
    delete istm;
    delete ustm;
}

template<typename Parent>
void SQLiteDataCommon<Parent>::read_attrs(Tracer<>& trc, int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    if (!read_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "SELECT attrs FROM %s WHERE id=?", Parent::table_name);
        read_attrs_stm = conn.sqlitestatement(query).release();
    }
    Tracer<> trc_sel(trc ? trc->trace_select("SELECT attrs FROM … WHERE id=?") : nullptr);
    read_attrs_stm->bind_val(1, id_data);
    read_attrs_stm->execute_one([&]() {
        if (trc_sel) trc_sel->add_row();
        Values::decode(read_attrs_stm->column_blob(0), dest);
    });
}

template<typename Parent>
void SQLiteDataCommon<Parent>::write_attrs(Tracer<>& trc, int id_data, const Values& values)
{
    if (!write_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=? WHERE id=?", Parent::table_name);
        write_attrs_stm = conn.sqlitestatement(query).release();
    }
    Tracer<> trc_upd(trc ? trc->trace_update("UPDATE … SET attrs=? WHERE id=?", 1) : nullptr);
    vector<uint8_t> encoded = values.encode();
    write_attrs_stm->bind_val(1, encoded);
    write_attrs_stm->bind_val(2, id_data);
    write_attrs_stm->execute();
}

template<typename Parent>
void SQLiteDataCommon<Parent>::remove_all_attrs(Tracer<>& trc, int id_data)
{
    if (!remove_attrs_stm)
    {
        char query[64];
        snprintf(query, 64, "UPDATE %s SET attrs=NULL WHERE id=?", Parent::table_name);
        remove_attrs_stm = conn.sqlitestatement(query).release();
    }
    Tracer<> trc_upd(trc ? trc->trace_update("UPDATE … SET attrs=NULL WHERE id=?", 1) : nullptr);
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

template<typename Parent>
void SQLiteDataCommon<Parent>::remove(Tracer<>& trc, const v7::IdQueryBuilder& qb)
{
    char query[64];
    snprintf(query, 64, "DELETE FROM %s WHERE id=?", Parent::table_name);
    auto stmd = conn.sqlitestatement(query);
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    std::unique_ptr<Varmatch> attr_filter;
    if (!qb.query.attr_filter.empty())
        attr_filter = Varmatch::parse(qb.query.attr_filter);

    // Iterate all the data_id results, deleting the related data and attributes
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    stm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        if (attr_filter.get() && !match_attrs(*attr_filter, stm->column_blob(1))) return;

        // Compile the DELETE query for the data
        Tracer<> trc_del(trc ? trc->trace_delete(query, 1) : nullptr);
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}

template<typename Parent>
void SQLiteDataCommon<Parent>::update(Tracer<>& trc, std::vector<typename Parent::BatchValue>& vars, bool with_attrs)
{
    for (auto& v: vars)
    {
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

        Tracer<> trc_upd(trc ? trc->trace_update("UPDATE … set value=?, attrs=? WHERE id=?", 1) : nullptr);
        ustm->execute();
    }
}

static const char* select_station_data_query = "SELECT id, code FROM station_data WHERE id_station=?";
static const char* insert_station_data_query = "INSERT INTO station_data (id_station, code, value, attrs) VALUES (?, ?, ?, ?)";

SQLiteStationData::SQLiteStationData(v7::Transaction& tr, SQLiteConnection& conn)
    : SQLiteDataCommon(tr, conn)
{
    sstm = conn.sqlitestatement(select_station_data_query).release();
    istm = conn.sqlitestatement(insert_station_data_query).release();
}

void SQLiteStationData::query(Tracer<>& trc, int id_station, std::function<void(int id, wreport::Varcode code)> dest)
{
    sstm->bind_val(1, id_station);
    Tracer<> trc_sel(trc ? trc->trace_select(select_station_data_query) : nullptr);
    sstm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        int id = sstm->column_int(0);
        wreport::Varcode code = sstm->column_int(1);
        dest(id, code);
    });
}

void SQLiteStationData::insert(Tracer<>& trc, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    istm->bind_val(1, id_station);
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        istm->bind_val(2, v->var->code());
        istm->bind_val(3, v->var->enqc());
        values::Encoder enc;
        if (with_attrs && v->var->next_attr())
        {
            enc.append_attributes(*v->var);
            istm->bind_val(4, enc.buf);
        }
        else
            istm->bind_null_val(4);
        Tracer<> trc_ins(trc ? trc->trace_insert(insert_station_data_query, 1) : nullptr);
        istm->execute();
        v->id = conn.get_last_insert_id();
    }
}

void SQLiteStationData::run_station_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::DBStation station;
    stm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        wreport::Varcode code = stm->column_int(5);
        const char* value = stm->column_string(7);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(stm->column_blob(8), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = stm->column_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_data = stm->column_int(6);

        dest(station, id_data, move(var));
    });
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


static const char* select_data_query = "SELECT id, id_levtr, code FROM data WHERE id_station=? AND datetime=?";
static const char* insert_data_query = "INSERT INTO data (id_station, id_levtr, datetime, code, value, attrs) VALUES (?, ?, ?, ?, ?, ?)";

SQLiteData::SQLiteData(v7::Transaction& tr, SQLiteConnection& conn)
    : SQLiteDataCommon(tr, conn)
{
    sstm = conn.sqlitestatement(select_data_query).release();
    istm = conn.sqlitestatement(insert_data_query).release();
}

void SQLiteData::query(Tracer<>& trc, int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest)
{
    Tracer<> trc_sel(trc ? trc->trace_select(select_data_query) : nullptr);
    sstm->bind_val(1, id_station);
    sstm->bind_val(2, datetime);
    sstm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        int id_levtr = sstm->column_int(1);
        wreport::Varcode code = sstm->column_int(2);
        int id = sstm->column_int(0);
        dest(id, id_levtr, code);
    });
}

void SQLiteData::insert(Tracer<>& trc, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs)
{
    std::sort(vars.begin(), vars.end());
    istm->bind_val(1, id_station);
    istm->bind_val(3, datetime);
    for (auto v = vars.begin(); v != vars.end(); ++v)
    {
        // Skip duplicates
        auto next = v + 1;
        if (next != vars.end() && *v == *next)
            continue;
        Tracer<> trc_ins(trc ? trc->trace_insert(insert_data_query, 1) : nullptr);
        istm->bind_val(2, v->id_levtr);
        istm->bind_val(4, v->var->code());
        istm->bind_val(5, v->var->enqc());
        values::Encoder enc;
        if (with_attrs && v->var->next_attr())
        {
            enc.append_attributes(*v->var);
            istm->bind_val(6, enc.buf);
        }
        else
            istm->bind_null_val(6);
        istm->execute();

        v->id = conn.get_last_insert_id();
    }
}

void SQLiteData::run_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)> dest)
{
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::DBStation station;
    stm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        wreport::Varcode code = stm->column_int(6);
        const char* value = stm->column_string(9);
        auto var = newvar(code, value);
        if (qb.select_attrs)
            values::Decoder::decode_attrs(stm->column_blob(10), *var);

        // Postprocessing filter of attr_filter
        if (qb.attr_filter && !qb.match_attrs(*var))
            return;

        int id_station = stm->column_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = tr.repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_levtr = stm->column_int(5);
        int id_data = stm->column_int(7);
        Datetime datetime = stm->column_datetime(8);

        dest(station, id_levtr, datetime, id_data, move(var));
    });
}

void SQLiteData::run_summary_query(Tracer<>& trc, const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)> dest)
{
    Tracer<> trc_sel(trc ? trc->trace_select(qb.sql_query) : nullptr);
    auto stm = conn.sqlitestatement(qb.sql_query);

    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    dballe::DBStation station;
    stm->execute([&]() {
        if (trc_sel) trc_sel->add_row();
        int id_station = stm->column_int(0);
        if (id_station != station.id)
        {
            station.id = id_station;
            station.report = qb.tr->repinfo().get_rep_memo(stm->column_int(1));
            station.coords.lat = stm->column_int(2);
            station.coords.lon = stm->column_int(3);
            if (stm->column_isnull(4))
                station.ident.clear();
            else
                station.ident = stm->column_string(4);
        }

        int id_levtr = stm->column_int(5);
        wreport::Varcode code = stm->column_int(6);

        size_t count = 0;
        DatetimeRange datetime;
        if (qb.select_summary_details)
        {
            count = stm->column_int(7);
            datetime = DatetimeRange(stm->column_datetime(8), stm->column_datetime(9));
        }

        dest(station, id_levtr, code, datetime, count);
    });
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
