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

template<typename Interface, typename Row>
int Base<Interface, Row>::remaining() const
{
    if (at_start)
        return results.size();
    else
        return results.end() - cur - 1;
}

template<typename Interface, typename Row>
bool Base<Interface, Row>::next()
{
    if (at_start)
        at_start = false;
    else if (cur != results.end())
        ++cur;
    return cur != results.end();
}

template<typename Interface, typename Row>
void Base<Interface, Row>::discard()
{
    at_start = false;
    cur = results.end();
}

template<typename Interface, typename Row>
int Base<Interface, Row>::get_priority() const
{
    return tr->repinfo().get_priority(cur->station.report);
}

template<typename Interface, typename Row>
unsigned Base<Interface, Row>::test_iterate(FILE* dump)
{
    unsigned count;
    for (count = 0; next(); ++count)
        if (dump)
            cur->dump(dump);
    return count;
}

template class Base<CursorStation, StationRow>;
template class Base<CursorStationData, StationDataRow>;
template class Base<CursorData, DataRow>;
template class Base<CursorSummary, SummaryRow>;


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

const DBValues& Stations::values() const
{
    if (!this->cur->values.get())
    {
        this->cur->values.reset(new DBValues);
        Tracer<> trc(tr->trc ? tr->trc->trace_add_station_vars() : nullptr);
        // FIXME: this could be made more efficient by querying all matching
        // station values, and merging rows during load, so it would only do
        // one query to the database
        tr->station().add_station_vars(trc, this->cur->station.id, *this->cur->values);
    }
    return *this->cur->values;
}

DBValues Stations::get_values() const
{
    return values();
}

void Stations::load(Tracer<>& trc, const StationQueryBuilder& qb)
{
    results.clear();
    this->tr->station().run_station_query(trc, qb, [&](const dballe::DBStation& desc) {
        results.emplace_back(desc);
    });
    at_start = true;
    cur = results.begin();
}


StationData::StationData(DataQueryBuilder& qb, bool with_attributes)
    : Base(qb.tr), with_attributes(with_attributes) {}

void StationData::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    this->tr->station_data().run_station_data_query(trc, qb, [&](const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var) {
        results.emplace_back(station, id_data, std::move(var));
    });
    at_start = true;
    cur = results.begin();
}

void StationData::attr_query(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const wreport::Var* a = cur->value->next_attr(); a != NULL; a = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    } else {
        tr->attr_query_station(attr_reference_id(), dest);
    }
}


template<typename Interface, typename Row>
const LevTrEntry& LevtrBase<Interface, Row>::get_levtr() const
{
    if (levtr == nullptr)
        // We prefetch levtr info for all IDs, so we do not need to hit the database here
        levtr = &(this->tr->levtr().lookup_cache(this->cur->id_levtr));
    return *levtr;
}

template<typename Interface, typename Row>
Level LevtrBase<Interface, Row>::get_level() const { return get_levtr().level; }

template<typename Interface, typename Row>
Trange LevtrBase<Interface, Row>::get_trange() const { return get_levtr().trange; }

template class LevtrBase<CursorData, DataRow>;
template class LevtrBase<CursorSummary, SummaryRow>;


Data::Data(DataQueryBuilder& qb, bool with_attributes)
    : LevtrBase(qb.tr), with_attributes(with_attributes) {}

void Data::load(Tracer<>& trc, const DataQueryBuilder& qb)
{
    results.clear();
    std::set<int> ids;
    this->tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
        results.emplace_back(station, id_levtr, datetime, id_data, std::move(var));
        ids.insert(id_levtr);
    });
    at_start = true;
    cur = results.begin();

    this->tr->levtr().prefetch_ids(trc, ids);
}

void Data::attr_query(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read)
{
    if (!force_read && with_attributes)
    {
        for (const Var* a = cur->value->next_attr(); a != NULL; a = a->next_attr())
            dest(std::unique_ptr<wreport::Var>(new Var(*a)));
    } else {
        tr->attr_query_data(attr_reference_id(), dest);
    }
}


void Summary::load(Tracer<>& trc, const SummaryQueryBuilder& qb)
{
    results.clear();
    set<int> ids;
    this->tr->data().run_summary_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t count) {
        results.emplace_back(station, id_levtr, code, datetime, count);
        ids.insert(id_levtr);
    });
    at_start = true;
    cur = results.begin();

    this->tr->levtr().prefetch_ids(trc, ids);
}

namespace {

#if 0
template<typename Interface, typename Row>
struct VectorBase : public Base<Interface, Row>
{
    using Base<Interface, Row>::Base;

#if 0
    int get_station_id() const override { return cur->station.id; }
    std::string get_report() const override { return cur->station.report; }
    Coords get_coords() const override { return cur->station.coords; }
    Ident get_ident() const override { return cur->station.ident; }
#endif

#if 0
    void to_data(dballe::Data& data) override
    {
        cur->to_data(*this->tr, core::Data::downcast(data));
    }
#endif
};
#endif


struct Best : public Data
{
    using Data::Data;

    int insert_cur_prio;

    /// Append or replace the last result according to priotity. Returns false if the value has been ignored.
    bool add_to_results(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)
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

    void load(Tracer<>& trc, const DataQueryBuilder& qb)
    {
        results.clear();
        set<int> ids;
        this->tr->data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
            if (add_to_results(station, id_levtr, datetime, id_data, move(var)))
                ids.insert(id_levtr);
        });
        at_start = true;
        cur = results.begin();

        this->tr->levtr().prefetch_ids(trc, ids);
    }

    void attr_query(std::function<void(std::unique_ptr<wreport::Var>)> dest, bool force_read) override
    {
        if (!force_read && with_attributes)
        {
            for (const Var* a = cur->value->next_attr(); a != NULL; a = a->next_attr())
                dest(std::unique_ptr<wreport::Var>(new Var(*a)));
        } else {
            tr->attr_query_data(attr_reference_id(), dest);
        }
    }
};


}

unique_ptr<dballe::CursorStation> run_station_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
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
    unique_ptr<db::CursorStation> res(resptr);
    resptr->load(trc, qb);
    return std::move(res);
}

unique_ptr<dballe::CursorStationData> run_station_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, true);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<db::CursorStationData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        throw error_unimplemented("best queries of station vars");
        //auto resptr = new Best(tr, modifiers);
        //res.reset(resptr);
        //resptr->load(qb);
    } else {
        auto resptr = new StationData(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        res.reset(resptr);
        resptr->load(trc, qb);
    }
    return std::move(res);
}

unique_ptr<dballe::CursorData> run_data_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
{
    unsigned int modifiers = q.get_modifiers();
    DataQueryBuilder qb(tr, q, modifiers, false);
    qb.build();

    if (explain)
    {
        fprintf(stderr, "EXPLAIN "); q.print(stderr);
        tr->db->conn->explain(qb.sql_query, stderr);
    }

    unique_ptr<CursorData> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
    {
        auto resptr = new Best(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        res.reset(resptr);
        resptr->load(trc, qb);
    } else {
        auto resptr = new Data(qb, modifiers & DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        res.reset(resptr);
        resptr->load(trc, qb);
    }
    return std::move(res);
}

unique_ptr<dballe::CursorSummary> run_summary_query(Tracer<>& trc, std::shared_ptr<v7::Transaction> tr, const core::Query& q, bool explain)
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
    unique_ptr<CursorSummary> res(resptr);
    resptr->load(trc, qb);
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

    tr->data().remove(trc, qb);
}


}
}
}
}
