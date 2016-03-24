#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/db/v7/attr.h"
#include "cursor.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/values.h"
#include "dballe/types.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <unistd.h>

using namespace std;
using namespace wreport;
using dballe::sql::Connection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v7 {

// First part of initialising a dba_db
DB::DB(unique_ptr<Connection> conn)
    : conn(conn.release()),
      m_driver(v7::Driver::create(*this->conn).release())
{
    init_after_connect();

    if (getenv("DBA_EXPLAIN") != NULL)
        explain_queries = true;

    auto tr = trace.trace_connect(this->conn->get_url());

    /* Set the connection timeout */
    /* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */

    tr->done();
}

DB::~DB()
{
    delete m_attr;
    delete m_data;
    delete m_station_attr;
    delete m_station_data;
    delete m_lev_tr;
    delete m_station;
    delete m_repinfo;
    delete m_driver;
    delete conn;
}

v7::Driver& DB::driver()
{
    return *m_driver;
}

v7::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = m_driver->create_repinfo().release();
    return *m_repinfo;
}

v7::Station& DB::station()
{
    if (m_station == NULL)
        m_station = m_driver->create_station().release();
    return *m_station;
}

v7::LevTr& DB::lev_tr()
{
    if (m_lev_tr == NULL)
        m_lev_tr = m_driver->create_levtr().release();
    return *m_lev_tr;
}

v7::StationData& DB::station_data()
{
    if (m_station_data == NULL)
        m_station_data = m_driver->create_station_data().release();
    return *m_station_data;
}

v7::Attr& DB::station_attr()
{
    if (m_station_attr == NULL)
        m_station_attr = m_driver->create_station_attr().release();
    return *m_station_attr;
}

v7::Data& DB::data()
{
    if (m_data == NULL)
        m_data = m_driver->create_data().release();
    return *m_data;
}

v7::Attr& DB::attr()
{
    if (m_attr == NULL)
        m_attr = m_driver->create_attr().release();
    return *m_attr;
}

void DB::init_after_connect()
{
}

std::unique_ptr<dballe::Transaction> DB::transaction()
{
    auto res = conn->transaction();
    return unique_ptr<dballe::Transaction>(new v7::Transaction(move(res)));
}

void DB::delete_tables()
{
    m_driver->delete_tables_v7();
}

void DB::disappear()
{
    m_driver->delete_tables_v7();

    // Invalidate the repinfo cache if we have a repinfo structure active
    if (m_repinfo)
    {
        delete m_repinfo;
        m_repinfo = 0;
    }
}

void DB::reset(const char* repinfo_file)
{
    auto tr = trace.trace_reset(repinfo_file);
    disappear();
    m_driver->create_tables_v7();

    // Populate the tables with values
    int added, deleted, updated;
    update_repinfo(repinfo_file, &added, &deleted, &updated);
    tr->done();
}

void DB::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    auto t = conn->transaction();
    repinfo().update(repinfo_file, added, deleted, updated);
    t->commit();
}

std::map<std::string, int> DB::get_repinfo_priorities()
{
    return repinfo().get_priorities();
}

int DB::rep_cod_from_memo(const char* memo)
{
    return repinfo().obtain_id(memo);
}

v7::stations_t::iterator DB::obtain_station(v7::State& state, const dballe::Station& st, bool can_add)
{
    v7::Station& s = station();

    // If the station is referenced only by ID, look it up by ID only
    if (st.ana_id != MISSING_INT && st.coords.is_missing())
        return s.lookup_id(state, st.ana_id);

    v7::Repinfo& ri = repinfo();

    StationDesc sd;
    sd.rep = ri.obtain_id(st.report.c_str());
    sd.coords = st.coords;
    sd.ident = st.ident;

    // Get the ID for the station
    if (can_add)
        return s.obtain_id(state, sd);
    else
        return s.get_id(state, sd);
}

void DB::insert_station_data(dballe::Transaction& transaction, StationValues& vals, bool can_replace, bool station_can_add)
{
    auto& t = v7::Transaction::downcast(transaction);

    // Insert the station data, and get the ID
    v7::stations_t::iterator si = obtain_station(t.state, vals.info, station_can_add);

    v7::bulk::InsertStationVars vars;
    vars.station = si->second;
    vals.info.ana_id = si->second.id;

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var);

    // Do the insert
    v7::StationData& d = station_data();
    d.insert(t, vars, can_replace ? v7::bulk::UPDATE : v7::bulk::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.id_data);
}

void DB::insert_data(dballe::Transaction& transaction, DataValues& vals, bool can_replace, bool station_can_add)
{
    /* Check for the existance of non-lev_tr data, otherwise it's all
     * useless.  Not inserting data is fine in case of setlev_trana */
    if (vals.values.empty())
        throw error_notfound("no variables found in input record");

    auto& t = v7::Transaction::downcast(transaction);

    // Insert the station data, and get the ID
    v7::stations_t::iterator si = obtain_station(t.state, vals.info, station_can_add);

    v7::bulk::InsertVars vars;
    vars.station = si->second;
    vals.info.ana_id = si->second.id;

    // Set the date from the record contents
    vars.datetime = vals.info.datetime;
    // Insert the lev_tr data, and get the ID
    auto ltri = lev_tr().obtain_id(t.state, LevTrDesc(vals.info.level, vals.info.trange));

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var, ltri->second.id);

    // Do the insert
    v7::Data& d = data();
    d.insert(t, vars, can_replace ? v7::bulk::UPDATE : v7::bulk::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.id_data);
}

void DB::remove_station_data(dballe::Transaction& transaction, const Query& query)
{
    auto tr = trace.trace_remove_station_data(query);
    cursor::run_delete_query(*this, core::Query::downcast(query), true, explain_queries);
    tr->done();
}

void DB::remove(dballe::Transaction& transaction, const Query& query)
{
    auto tr = trace.trace_remove(query);
    cursor::run_delete_query(*this, core::Query::downcast(query), false, explain_queries);
    tr->done();
}

void DB::remove_all(dballe::Transaction& transaction)
{
    auto tr = trace.trace_remove_all();
    driver().remove_all_v7();
    tr->done();
}

void DB::vacuum()
{
    auto tr = trace.trace_vacuum();
    auto t = conn->transaction();
    driver().vacuum_v7();
    t->commit();
    tr->done();
}

std::unique_ptr<db::CursorStation> DB::query_stations(const Query& query)
{
    auto tr = trace.trace_query_stations(query);
    auto res = cursor::run_station_query(*this, core::Query::downcast(query), explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorStationData> DB::query_station_data(const Query& query)
{
    auto tr = trace.trace_query_station_data(query);
    auto res = cursor::run_station_data_query(*this, core::Query::downcast(query), explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorData> DB::query_data(const Query& query)
{
    auto tr = trace.trace_query_data(query);
    auto res = cursor::run_data_query(*this, core::Query::downcast(query), explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorSummary> DB::query_summary(const Query& query)
{
    auto tr = trace.trace_query_summary(query);
    auto res = cursor::run_summary_query(*this, core::Query::downcast(query), explain_queries);
    tr->done();
    return move(res);
}

void DB::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    v7::Attr& a = station_attr();
    a.read(data_id, dest);
}

void DB::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    v7::Attr& a = attr();
    a.read(data_id, dest);
}

void DB::attr_insert_station(dballe::Transaction& transaction, int data_id, const Values& attrs)
{
    v7::Attr& a = station_attr();
    v7::bulk::InsertAttrsV7 iattrs;
    for (const auto& i : attrs)
        iattrs.add(i.second.var, data_id);
    if (iattrs.empty()) return;

    auto& t = v7::Transaction::downcast(transaction);

    // Insert all the attributes we found
    a.insert(t, iattrs, v7::Attr::UPDATE);
}

void DB::attr_insert_data(dballe::Transaction& transaction, int data_id, const Values& attrs)
{
    v7::Attr& a = attr();
    v7::bulk::InsertAttrsV7 iattrs;
    for (const auto& i : attrs)
        iattrs.add(i.second.var, data_id);
    if (iattrs.empty()) return;

    auto& t = v7::Transaction::downcast(transaction);

    // Insert all the attributes we found
    a.insert(t, iattrs, v7::Attr::UPDATE);
}

void DB::attr_remove_station(dballe::Transaction& transaction, int data_id, const db::AttrList& qcs)
{
    Querybuf query(500);
    if (qcs.empty())
        // Delete all attributes
        query.appendf("DELETE FROM station_attr WHERE id_data=%d", data_id);
    else {
        // Delete only the attributes in qcs
        query.appendf("DELETE FROM station_attr WHERE id_data=%d AND type IN (", data_id);
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }
    driver().exec_no_data(query);
}

void DB::attr_remove_data(dballe::Transaction& transaction, int data_id, const db::AttrList& qcs)
{
    Querybuf query(500);
    if (qcs.empty())
        // Delete all attributes
        query.appendf("DELETE FROM attr WHERE id_data=%d", data_id);
    else {
        // Delete only the attributes in qcs
        query.appendf("DELETE FROM attr WHERE id_data=%d AND type IN (", data_id);
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }
    driver().exec_no_data(query);
}

bool DB::is_station_variable(int data_id, wreport::Varcode varcode)
{
    return false;
}

void DB::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    lev_tr().dump(out);
    station_data().dump(out);
    station_attr().dump(out);
    data().dump(out);
    attr().dump(out);
}

}
}
}
