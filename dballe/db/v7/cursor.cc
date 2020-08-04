#include "cursor.h"
#include "qbuilder.h"
#include "db.h"
#include "transaction.h"
#include "dballe/sql/sql.h"
#include <dballe/db/v7/driver.h>
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/types.h"
#include "dballe/var.h"
#include "dballe/core/var.h"
#include "dballe/core/data.h"
#include "dballe/core/query.h"
#include "wreport/var.h"
#include <unordered_map>
#include <cstring>
#include <cassert>

namespace {

// Consts used for to_record_todo
const unsigned int TOREC_PSEUDOANA   = 1 << 0;
const unsigned int TOREC_BASECONTEXT = 1 << 1;
const unsigned int TOREC_DATACONTEXT = 1 << 2;
const unsigned int TOREC_DATA        = 1 << 3;

}

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

void StationRows::load(Tracer<>& trc, const StationQueryBuilder& qb)
{
    results.clear();
    tr->station().run_station_query(trc, qb, [&](const dballe::DBStation& desc) {
        results.emplace_back(desc);
    });
    at_start = true;
    cur = results.begin();
}

const DBValues& StationRows::values() const
{
    if (!cur->values.get())
    {
        cur->values.reset(new DBValues);
        Tracer<> trc(tr->trc ? tr->trc->trace_add_station_vars() : nullptr);
        // FIXME: this could be made more efficient by querying all matching
        // station values, and merging rows during load, so it would only do
        // one query to the database
        tr->station().add_station_vars(trc, cur->station.id, *cur->values);
    }
    return *cur->values;
}


void StationDataRows::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    tr->station_data().run_station_data_query(trc, qb, [&](const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var) {
        results.emplace_back(station, id_data, std::move(var));
    });
    at_start = true;
    cur = results.begin();
}

void DataRows::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    std::set<int> ids;
    tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
        results.emplace_back(station, id_levtr, datetime, id_data, std::move(var));
        ids.insert(id_levtr);
    });
    at_start = true;
    cur = results.begin();

    tr->levtr().prefetch_ids(trc, ids);
}

bool DataRows::add_to_best_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)
{
    int prio = tr->repinfo().get_priority(station.report);

    if (results.empty()) goto append;
    if (station.coords != results.back().station.coords) goto append;
    if (station.ident != results.back().station.ident) goto append;
    if (id_levtr != results.back().id_levtr) goto append;
    if (datetime != results.back().datetime) goto append;
    if (var->code() != results.back().value.code()) goto append;

    if (prio <= insert_cur_prio) return false;

    // Replace
    results.back().station = station;
    results.back().value = DBValue(id_data, std::move(var));
    insert_cur_prio = prio;
    return true;

append:
    results.emplace_back(station, id_levtr, datetime, id_data, std::move(var));
    insert_cur_prio = prio;
    return true;
}

void DataRows::load_best(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
        if (add_to_best_results(station, id_levtr, datetime, id_data, move(var)))
            ids.insert(id_levtr);
    });
    at_start = true;
    cur = results.begin();

    tr->levtr().prefetch_ids(trc, ids);
}

void SummaryRows::load(Tracer<>& trc, const SummaryQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    tr->data().run_summary_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t count) {
        results.emplace_back(station, id_levtr, code, datetime, count);
        ids.insert(id_levtr);
    });
    at_start = true;
    cur = results.begin();

    tr->levtr().prefetch_ids(trc, ids);
}


template<typename Impl>
int Base<Impl>::remaining() const
{
    if (rows.at_start)
        return rows.results.size();
    else
        return rows.results.end() - rows.cur - 1;
}

template<typename Impl>
void Base<Impl>::discard()
{
    rows.discard();
}

template<typename Impl>
unsigned Base<Impl>::test_iterate(FILE* dump)
{
    unsigned count;
    for (count = 0; next(); ++count)
        if (dump)
            rows->dump(dump);
    return count;
}

template class Base<Stations>;
template class Base<StationData>;
template class Base<Data>;
template class Base<Summary>;


void StationRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s\n", station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get());
}

void StationDataRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s ",
            station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get());
    value.print(out);
}

void DataRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d ",
            station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get(), id_levtr);
    datetime.print_iso8601(out, ' ');
    fprintf(out, " ");
    value.print(out);
}

void SummaryRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d %d%02d%03d\n",
            station.id, station.report.c_str(), station.coords.dlat(), station.coords.dlon(), station.ident.get(), id_levtr, WR_VAR_FXY(code));
}

DBValues Stations::get_values() const
{
    return rows.values();
}

void Stations::remove()
{
    core::Query query;
    query.ana_id = rows->station.id;
    rows.tr->remove_station_data(query);
    rows.tr->remove_data(query);
}


StationData::StationData(DataQueryBuilder& qb, bool with_attributes)
    : Base(qb.tr), with_attributes(with_attributes) {}

void StationData::query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const wreport::Var* a = rows->value->next_attr(); a != NULL; a = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    } else {
        rows.tr->attr_query_station(attr_reference_id(), dest);
    }
}

void StationData::remove()
{
    rows.tr->remove_station_data_by_id(rows->value.data_id);
}


Data::Data(DataQueryBuilder& qb, bool with_attributes)
    : Base(qb.tr), with_attributes(with_attributes) {}

void Data::query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const Var* a = rows->value->next_attr(); a != NULL; a = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    } else {
        rows.tr->attr_query_data(attr_reference_id(), dest);
    }
}

void Data::remove()
{
    rows.tr->remove_data_by_id(rows->value.data_id);
}


void Summary::remove()
{
    core::Query query;
    query.ana_id = rows->station.id;
    auto levtr = rows.get_levtr();
    query.level = levtr.level;
    query.trange = levtr.trange;
    query.varcodes.insert(rows->code);
    rows.tr->remove_data(query);
}

std::unique_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    StationQueryBuilder qb(tr, q, modifiers);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Stations(tr);
    std::unique_ptr<db::CursorStation> res(resptr);
    resptr->rows.load(trc, qb);
    // std::move is redundant, but needed by centos7's obsolete compiler
    return std::move(res);
}

std::unique_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, true);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    std::unique_ptr<db::CursorStationData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        throw error_unimplemented("best queries of station vars");
        //auto resptr = new Best(tr, modifiers);
        //res.reset(resptr);
        //resptr->load(qb);
    } else {
        auto resptr = new StationData(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        res.reset(resptr);
        resptr->rows.load(trc, qb);
    }
    // std::move is redundant, but needed by centos7's obsolete compiler
    return std::move(res);
}

std::unique_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    std::unique_ptr<CursorData> res;
    auto resptr = new Data(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
    res.reset(resptr);

    if (modifiers & DBA_DB_MODIFIER_BEST)
        resptr->rows.load_best(trc, qb);
    else
        resptr->rows.load(trc, qb);
    // std::move is redundant, but needed by centos7's obsolete compiler
    return std::move(res);
}

std::unique_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on summary queries");

    SummaryQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto resptr = new Summary(tr);
    std::unique_ptr<CursorSummary> res(resptr);
    resptr->rows.load(trc, qb);
    // std::move is redundant, but needed by centos7's obsolete compiler
    return std::move(res);
}

void run_delete_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool station_vars, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & DBA_DB_MODIFIER_BEST)
        throw error_consistency("cannot use query=best on delete queries");

    IdQueryBuilder qb(tr, q, modifiers, station_vars);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    if (station_vars)
        tr->station_data().remove(trc, qb);
    else
        tr->data().remove(trc, qb);
}


}
}
}
}
