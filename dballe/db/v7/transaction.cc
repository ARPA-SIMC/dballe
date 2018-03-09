#include "transaction.h"
#include "db.h"
#include "driver.h"
#include "levtr.h"
#include "cursor.h"
#include "station.h"
#include "repinfo.h"
#include "dballe/core/query.h"
#include "dballe/sql/sql.h"
#include <cassert>
#include <memory>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Transaction::Transaction(std::shared_ptr<v7::DB> db, std::unique_ptr<dballe::Transaction> sql_transaction)
    : db(db), sql_transaction(sql_transaction.release())
{
    m_repinfo = db->driver().create_repinfo().release();
    m_station = db->driver().create_station().release();
}

Transaction::~Transaction()
{
    rollback();
    delete m_station;
    delete m_repinfo;
    delete sql_transaction;
}

v7::Repinfo& Transaction::repinfo()
{
    return *m_repinfo;
}

v7::Station& Transaction::station()
{
    return *m_station;
}


void Transaction::commit()
{
    if (fired) return;
    sql_transaction->commit();
    fired = true;
}

void Transaction::rollback()
{
    if (fired) return;
    sql_transaction->rollback();
    clear_cached_state();
    fired = true;
}

void Transaction::clear_cached_state()
{
    state.clear();
    repinfo().read_cache();
    station().clear_cache();
}

Transaction& Transaction::downcast(dballe::Transaction& transaction)
{
    v7::Transaction* t = dynamic_cast<v7::Transaction*>(&transaction);
    assert(t);
    return *t;
}

void Transaction::remove_all()
{
    auto tr = db->trace.trace_remove_all();
    db->driver().remove_all_v7();
    clear_cached_state();
    tr->done();
}

void Transaction::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    // Insert the station data, and get the ID
    int si = obtain_station(state, vals.info, station_can_add);

    v7::bulk::InsertStationVars vars(state, si);
    vals.info.ana_id = si;

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var);

    // Do the insert
    v7::StationData& d = db->station_data();
    d.insert(*this, vars, can_replace ? v7::bulk::UPDATE : v7::bulk::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.cur->second.id);
}

void Transaction::insert_data(DataValues& vals, bool can_replace, bool station_can_add)
{
    /* Check for the existance of non-lev_tr data, otherwise it's all
     * useless.  Not inserting data is fine in case of setlev_trana */
    if (vals.values.empty())
        throw error_notfound("no variables found in input record");

    // Insert the station data, and get the ID
    int si = obtain_station(state, vals.info, station_can_add);

    v7::bulk::InsertVars vars(state, si, vals.info.datetime);
    vals.info.ana_id = si;

    // Insert the lev_tr data, and get the ID
    auto ltri = db->levtr().obtain_id(state, LevTrDesc(vals.info.level, vals.info.trange));

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var, ltri->second);

    // Do the insert
    v7::Data& d = db->data();
    d.insert(*this, vars, can_replace ? v7::bulk::UPDATE : v7::bulk::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.cur->second.id);
}

void Transaction::remove_station_data(const Query& query)
{
    auto tr = db->trace.trace_remove_station_data(query);
    cursor::run_delete_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), true, db->explain_queries);
    tr->done();
}

void Transaction::remove(const Query& query)
{
    auto tr = db->trace.trace_remove(query);
    cursor::run_delete_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), false, db->explain_queries);
    tr->done();
}

std::unique_ptr<db::CursorStation> Transaction::query_stations(const Query& query)
{
    auto tr = db->trace.trace_query_stations(query);
    auto res = cursor::run_station_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorStationData> Transaction::query_station_data(const Query& query)
{
    auto tr = db->trace.trace_query_station_data(query);
    auto res = cursor::run_station_data_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries, false);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorData> Transaction::query_data(const Query& query)
{
    auto tr = db->trace.trace_query_data(query);
    auto res = cursor::run_data_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries, false);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorSummary> Transaction::query_summary(const Query& query)
{
    auto tr = db->trace.trace_query_summary(query);
    auto res = cursor::run_summary_query(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

void Transaction::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    auto& d = db->station_data();
    d.read_attrs(data_id, dest);
}

void Transaction::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    auto& d = db->data();
    d.read_attrs(data_id, dest);
}

void Transaction::attr_insert_station(int data_id, const Values& attrs)
{
    auto& d = db->station_data();
    d.merge_attrs(data_id, attrs);
}

void Transaction::attr_insert_data(int data_id, const Values& attrs)
{
    auto& d = db->data();
    d.merge_attrs(data_id, attrs);
}

void Transaction::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    if (attrs.empty())
    {
        // Delete all attributes
        char buf[64];
        snprintf(buf, 64, "UPDATE station_data SET attrs=NULL WHERE id=%d", data_id);
        db->conn->execute(buf);
    } else {
        auto& d = db->station_data();
        d.remove_attrs(data_id, attrs);
    }
}

void Transaction::attr_remove_data(int data_id, const db::AttrList& attrs)
{
    if (attrs.empty())
    {
        // Delete all attributes
        char buf[64];
        snprintf(buf, 64, "UPDATE data SET attrs=NULL WHERE id=%d", data_id);
        db->conn->execute(buf);
    } else {
        auto& d = db->data();
        d.remove_attrs(data_id, attrs);
    }
}

int Transaction::obtain_station(v7::State& state, const dballe::Station& st, bool can_add)
{
    v7::Station& s = station();

    // If the station is referenced only by ID, look it up by ID only
    if (st.ana_id != MISSING_INT)
        return st.ana_id;

    // Get the ID for the station
    if (can_add)
        return s.obtain_id(*this, st);
    else
        return s.get_id(*this, st);
}

void Transaction::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    repinfo().update(repinfo_file, added, deleted, updated);
}

void Transaction::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    db->levtr().dump(out);
    db->station_data().dump(out);
    db->data().dump(out);
}

void TestTransaction::commit()
{
    throw std::runtime_error("commit attempted while forbidden during tests");
}

}
}
}
