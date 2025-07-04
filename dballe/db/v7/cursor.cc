#include "cursor.h"
#include "db.h"
#include "dballe/core/data.h"
#include "dballe/core/query.h"
#include "dballe/core/var.h"
#include "dballe/db/v7/data.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/sql/sql.h"
#include "dballe/types.h"
#include "dballe/var.h"
#include "qbuilder.h"
#include "transaction.h"
#include "wreport/var.h"
#include <cassert>
#include <cstring>
#include <dballe/db/v7/driver.h>
#include <unordered_map>

namespace {

// Consts used for to_record_todo
const unsigned int TOREC_PSEUDOANA   = 1 << 0;
const unsigned int TOREC_BASECONTEXT = 1 << 1;
const unsigned int TOREC_DATACONTEXT = 1 << 2;
const unsigned int TOREC_DATA        = 1 << 3;

} // namespace

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {
namespace cursor {

template <typename Impl> int Base<Impl>::remaining() const
{
    if (at_start)
        return results.size();
    else
        return results.size() - 1;
}

template <typename Impl> unsigned Base<Impl>::test_iterate(FILE* dump)
{
    unsigned count;
    for (count = 0; next(); ++count)
        if (dump)
            row().dump(dump);
    return count;
}

template class Base<Stations>;
template class Base<StationData>;
template class Base<Data>;
template class Base<Summary>;

void StationRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s\n", station.id,
            station.report.c_str(), station.coords.dlat(),
            station.coords.dlon(), station.ident.get());
}

void StationDataRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s ", station.id,
            station.report.c_str(), station.coords.dlat(),
            station.coords.dlon(), station.ident.get());
    value.print(out);
}

void DataRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d ", station.id,
            station.report.c_str(), station.coords.dlat(),
            station.coords.dlon(), station.ident.get(), id_levtr);
    datetime.print_iso8601(out, ' ');
    fprintf(out, " ");
    value.print(out);
}

void SummaryRow::dump(FILE* out) const
{
    fprintf(out, "%02d %8.8s %02.4f %02.4f %-10s %4d %d%02d%03d\n", station.id,
            station.report.c_str(), station.coords.dlat(),
            station.coords.dlon(), station.ident.get(), id_levtr,
            WR_VAR_FXY(code));
}

void Stations::load(Tracer<>& trc, const StationQueryBuilder& qb)
{
    results.clear();
    tr->station().run_station_query(
        trc, qb,
        [&](const dballe::DBStation& desc) { results.emplace_back(desc); });
    at_start = true;
}

const DBValues& Stations::values() const
{
    if (!results.front().values.get())
    {
        results.front().values.reset(new DBValues);
        Tracer<> trc(tr->trc ? tr->trc->trace_add_station_vars() : nullptr);
        // FIXME: this could be made more efficient by querying all matching
        // station values, and merging rows during load, so it would only do
        // one query to the database
        tr->station().add_station_vars(trc, results.front().station.id,
                                       *results.front().values);
    }
    return *results.front().values;
}

DBValues Stations::get_values() const { return values(); }

void Stations::remove()
{
    core::Query query;
    query.ana_id = row().station.id;
    tr->remove_station_data(query);
    tr->remove_data(query);
}

StationData::StationData(DataQueryBuilder& qb, bool with_attributes)
    : Base(qb.tr), with_attributes(with_attributes)
{
}

void StationData::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    tr->station_data().run_station_data_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_data,
            std::unique_ptr<wreport::Var> var) {
            results.emplace_back(station, id_data, std::move(var));
        });
    at_start = true;
}

void StationData::query_attrs(
    std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const wreport::Var* a = row().value->next_attr(); a != NULL;
             a                     = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    }
    else
    {
        tr->attr_query_station(attr_reference_id(), dest);
    }
}

void StationData::remove()
{
    tr->remove_station_data_by_id(row().value.data_id);
}

Data::Data(DataQueryBuilder& qb, bool with_attributes)
    : LevTrBase(qb.tr), with_attributes(with_attributes)
{
}

void Data::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    std::set<int> ids;
    tr->data().run_data_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_levtr,
            const Datetime& datetime, int id_data,
            std::unique_ptr<wreport::Var> var) {
            results.emplace_back(station, id_levtr, datetime, id_data,
                                 std::move(var));
            ids.insert(id_levtr);
        });
    at_start = true;

    tr->levtr().prefetch_ids(trc, ids);
}

bool Data::add_to_best_results(const dballe::DBStation& station, int id_levtr,
                               const Datetime& datetime, int id_data,
                               std::unique_ptr<wreport::Var> var)
{
    int prio = tr->repinfo().get_priority(station.report);

    if (results.empty())
        goto append;
    if (station.coords != results.back().station.coords)
        goto append;
    if (station.ident != results.back().station.ident)
        goto append;
    if (id_levtr != results.back().id_levtr)
        goto append;
    if (datetime != results.back().datetime)
        goto append;
    if (var->code() != results.back().value.code())
        goto append;

    if (prio <= insert_cur_prio)
        return false;

    // Replace
    results.back().station = station;
    results.back().value   = DBValue(id_data, std::move(var));
    insert_cur_prio        = prio;
    return true;

append:
    results.emplace_back(station, id_levtr, datetime, id_data, std::move(var));
    insert_cur_prio = prio;
    return true;
}

void Data::load_best(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    tr->data().run_data_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_levtr,
            const Datetime& datetime, int id_data,
            std::unique_ptr<wreport::Var> var) {
            if (add_to_best_results(station, id_levtr, datetime, id_data,
                                    move(var)))
                ids.insert(id_levtr);
        });
    at_start = true;

    tr->levtr().prefetch_ids(trc, ids);
}

bool Data::add_to_last_results(const dballe::DBStation& station, int id_levtr,
                               const Datetime& datetime, int id_data,
                               std::unique_ptr<wreport::Var> var)
{
    if (results.empty())
        goto append;
    if (station.id != results.back().station.id)
        goto append;
    if (id_levtr != results.back().id_levtr)
        goto append;
    if (var->code() != results.back().value.code())
        goto append;

    if (datetime <= results.back().datetime)
        // Ignore older values than what we have
        return false;

    // Replace
    results.back().station  = station;
    results.back().id_levtr = id_levtr;
    results.back().datetime = datetime;
    results.back().value    = DBValue(id_data, std::move(var));
    return true;

append:
    results.emplace_back(station, id_levtr, datetime, id_data, std::move(var));
    return true;
}

void Data::load_last(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    tr->data().run_data_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_levtr,
            const Datetime& datetime, int id_data,
            std::unique_ptr<wreport::Var> var) {
            if (add_to_last_results(station, id_levtr, datetime, id_data,
                                    move(var)))
                ids.insert(id_levtr);
        });
    at_start = true;

    tr->levtr().prefetch_ids(trc, ids);
}

void Data::query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest,
                       bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const Var* a = row().value->next_attr(); a != NULL;
             a            = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    }
    else
    {
        tr->attr_query_data(attr_reference_id(), dest);
    }
}

void Data::remove() { tr->remove_data_by_id(row().value.data_id); }

void Summary::load(Tracer<>& trc, const SummaryQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    tr->data().run_summary_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_levtr,
            wreport::Varcode code, const DatetimeRange& datetime,
            size_t count) {
            results.emplace_back(station, id_levtr, code, datetime, count);
            ids.insert(id_levtr);
        });
    at_start = true;

    tr->levtr().prefetch_ids(trc, ids);
}

void Summary::remove()
{
    core::Query query;
    query.ana_id      = row().station.id;
    const auto& levtr = get_levtr();
    query.level       = levtr.level;
    query.trange      = levtr.trange;
    query.varcodes.insert(row().code);
    tr->remove_data(query);
}

std::shared_ptr<dballe::CursorStation>
run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr,
                  const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    StationQueryBuilder qb(tr, q, modifiers);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN ");
        q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto res = std::make_shared<Stations>(tr);
    res->load(trc, qb);
    return res;
}

std::shared_ptr<dballe::CursorStationData>
run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr,
                       const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, true);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN ");
        q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    if (modifiers & (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_LAST))
    {
        throw error_unimplemented(
            "best or last queries of station values are not implemented");
        // auto resptr = new Best(tr, modifiers);
        // res.reset(resptr);
        // resptr->load(qb);
    }
    else
    {
        auto res = std::make_shared<StationData>(
            qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        res->load(trc, qb);
        return res;
    }
}

std::shared_ptr<dballe::CursorData>
run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr,
               const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN ");
        q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto res =
        std::make_shared<Data>(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
    if (modifiers & DBA_DB_MODIFIER_BEST)
        res->load_best(trc, qb);
    else if (modifiers & DBA_DB_MODIFIER_LAST)
        res->load_last(trc, qb);
    else
        res->load(trc, qb);
    return res;
}

std::shared_ptr<dballe::CursorSummary>
run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr,
                  const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_LAST))
        throw error_consistency(
            "cannot use query=best or query=last on summary queries");

    SummaryQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN ");
        q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    auto res = std::make_shared<Summary>(tr);
    res->load(trc, qb);
    return res;
}

void run_delete_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr,
                      const core::Query& q, bool station_vars, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    if (modifiers & (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_LAST))
        throw error_consistency(
            "cannot use query=best or query=last on delete queries");

    IdQueryBuilder qb(tr, q, modifiers, station_vars);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN ");
        q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    if (station_vars)
        tr->station_data().remove(trc, qb);
    else
        tr->data().remove(trc, qb);
}

} // namespace cursor
} // namespace v7
} // namespace db
} // namespace dballe
