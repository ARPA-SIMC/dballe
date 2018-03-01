#include "transaction.h"
#include "db.h"
#include "driver.h"
#include "levtr.h"
#include "cursor.h"
#include "dballe/core/query.h"
#include "dballe/sql/sql.h"
#include <cassert>

using namespace wreport;

namespace dballe {
namespace db {
namespace v7 {

Transaction& Transaction::downcast(dballe::Transaction& transaction)
{
    v7::Transaction* t = dynamic_cast<v7::Transaction*>(&transaction);
    assert(t);
    return *t;
}

void Transaction::remove_all()
{
    auto tr = db.trace.trace_remove_all();
    db.driver().remove_all_v7();
    clear_cached_state();
    tr->done();
}

void Transaction::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    // Insert the station data, and get the ID
    v7::stations_t::iterator si = db.obtain_station(state, vals.info, station_can_add);

    v7::bulk::InsertStationVars vars(state, si);
    vals.info.ana_id = si->second.id;

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var);

    // Do the insert
    v7::StationData& d = db.station_data();
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
    v7::stations_t::iterator si = db.obtain_station(state, vals.info, station_can_add);

    v7::bulk::InsertVars vars(state, si, vals.info.datetime);
    vals.info.ana_id = si->second.id;

    // Insert the lev_tr data, and get the ID
    auto ltri = db.levtr().obtain_id(state, LevTrDesc(vals.info.level, vals.info.trange));

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var, ltri->second);

    // Do the insert
    v7::Data& d = db.data();
    d.insert(*this, vars, can_replace ? v7::bulk::UPDATE : v7::bulk::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.cur->second.id);
}

void Transaction::remove_station_data(const Query& query)
{
    auto tr = db.trace.trace_remove_station_data(query);
    cursor::run_delete_query(db, core::Query::downcast(query), true, db.explain_queries);
    tr->done();
}

void Transaction::remove(const Query& query)
{
    auto tr = db.trace.trace_remove(query);
    cursor::run_delete_query(db, core::Query::downcast(query), false, db.explain_queries);
    tr->done();
}

void Transaction::attr_insert_station(int data_id, const Values& attrs)
{
    auto& d = db.station_data();
    d.merge_attrs(data_id, attrs);
}

void Transaction::attr_insert_data(int data_id, const Values& attrs)
{
    auto& d = db.data();
    d.merge_attrs(data_id, attrs);
}

void Transaction::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    if (attrs.empty())
    {
        // Delete all attributes
        char buf[64];
        snprintf(buf, 64, "UPDATE station_data SET attrs=NULL WHERE id=%d", data_id);
        db.conn->execute(buf);
    } else {
        auto& d = db.station_data();
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
        db.conn->execute(buf);
    } else {
        auto& d = db.data();
        d.remove_attrs(data_id, attrs);
    }
}

}
}
}
