#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/db/v6/driver.h"
#include "dballe/db/v6/repinfo.h"
#include "dballe/db/v6/station.h"
#include "dballe/db/v6/levtr.h"
#include "dballe/db/v6/datav6.h"
#include "dballe/db/v6/attrv6.h"
#include "cursor.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/values.h"
#include "dballe/types.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <limits.h>
#include <unistd.h>

using namespace std;
using namespace wreport;
using dballe::sql::Connection;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v6 {

struct TestTransaction : public Transaction
{
    using Transaction::Transaction;

    void commit() override
    {
        throw std::runtime_error("commit attempted while forbidden during tests");
    }
};

Transaction::Transaction(std::shared_ptr<v6::DB> db, std::unique_ptr<dballe::Transaction> sql_transaction)
    : db(db), sql_transaction(sql_transaction.release()) {}
Transaction::~Transaction()
{
    // FIXME: use a nothrow version
    rollback();
    delete sql_transaction;
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
    if (db->m_lev_tr_cache)
        db->m_lev_tr_cache->invalidate();
    db->repinfo().read_cache();
}

void Transaction::remove_all()
{
    auto tr = db->trace.trace_remove_all();
    db->driver().remove_all_v6();
    clear_cached_state();
    tr->done();
}

std::unique_ptr<db::CursorStation> Transaction::query_stations(const Query& query)
{
    auto tr = db->trace.trace_query_stations(query);
    auto res = cursor::run_station_query(dynamic_pointer_cast<v6::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorStationData> Transaction::query_station_data(const Query& query)
{
    auto tr = db->trace.trace_query_station_data(query);
    auto res = cursor::run_station_data_query(dynamic_pointer_cast<v6::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorData> Transaction::query_data(const Query& query)
{
    auto tr = db->trace.trace_query_data(query);
    auto res = cursor::run_data_query(dynamic_pointer_cast<v6::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

std::unique_ptr<db::CursorSummary> Transaction::query_summary(const Query& query)
{
    auto tr = db->trace.trace_query_summary(query);
    auto res = cursor::run_summary_query(dynamic_pointer_cast<v6::Transaction>(shared_from_this()), core::Query::downcast(query), db->explain_queries);
    tr->done();
    return move(res);
}

void Transaction::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    v6::AttrV6& a = db->attr();
    a.read(data_id, dest);
}

void Transaction::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    v6::AttrV6& a = db->attr();
    a.read(data_id, dest);
}

void Transaction::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    v6::Repinfo& ri = db->repinfo();
    v6::DataV6& d = db->data();

    v6::bulk::InsertV6 vars;
    // Insert the station data, and get the ID
    vars.id_station = vals.info.ana_id = db->obtain_station(vals.info, station_can_add);
    // Get the ID of the report
    vars.id_report = ri.obtain_id(vals.info.report.c_str());

    // Hardcoded values for station variables
    vars.datetime = Datetime(1000, 1, 1, 0, 0, 0);

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var, -1);

    // Do the insert
    d.insert(*this, vars, can_replace ? v6::DataV6::UPDATE : v6::DataV6::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.id_data);
}

void Transaction::insert_data(DataValues& vals, bool can_replace, bool station_can_add)
{
    /* Check for the existance of non-lev_tr data, otherwise it's all
     * useless.  Not inserting data is fine in case of setlev_trana */
    if (vals.values.empty())
        throw error_notfound("no variables found in input record");

    v6::Repinfo& ri = db->repinfo();
    v6::DataV6& d = db->data();

    v6::bulk::InsertV6 vars;
    // Insert the station data, and get the ID
    vars.id_station = vals.info.ana_id = db->obtain_station(vals.info, station_can_add);
    // Get the ID of the report
    vars.id_report = ri.obtain_id(vals.info.report.c_str());
    // Set the date from the record contents
    vars.datetime = vals.info.datetime;
    // Insert the lev_tr data, and get the ID
    int id_levtr = db->lev_tr().obtain_id(vals.info.level, vals.info.trange);

    // Add all the variables we find
    for (auto& i: vals.values)
        vars.add(i.second.var, id_levtr);

    // Do the insert
    d.insert(*this, vars, can_replace ? v6::DataV6::UPDATE : v6::DataV6::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        vals.values.add_data_id(v.var->code(), v.id_data);
}

void Transaction::remove_station_data(const Query& query)
{
    auto tr = db->trace.trace_remove_station_data(query);
    cursor::run_delete_query(*db, core::Query::downcast(query), true, db->explain_queries);
    tr->done();
}

void Transaction::remove(const Query& query)
{
    auto tr = db->trace.trace_remove(query);
    cursor::run_delete_query(*db, core::Query::downcast(query), false, db->explain_queries);
    tr->done();
}

void Transaction::attr_insert_station(int data_id, const Values& attrs)
{
    v6::AttrV6& a = db->attr();
    v6::bulk::InsertAttrsV6 iattrs;
    for (const auto& i : attrs)
        iattrs.add(i.second.var, data_id);
    if (iattrs.empty()) return;

    // Insert all the attributes we found
    a.insert(*this, iattrs, v6::AttrV6::UPDATE);
}

void Transaction::attr_insert_data(int data_id, const Values& attrs)
{
    v6::AttrV6& a = db->attr();
    v6::bulk::InsertAttrsV6 iattrs;
    for (const auto& i : attrs)
        iattrs.add(i.second.var, data_id);
    if (iattrs.empty()) return;

    // Insert all the attributes we found
    a.insert(*this, iattrs, v6::AttrV6::UPDATE);
}

void Transaction::attr_remove_station(int data_id, const db::AttrList& qcs)
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
    db->conn->execute(query);
}

void Transaction::attr_remove_data(int data_id, const db::AttrList& qcs)
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
    db->conn->execute(query);
}

void Transaction::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    db->repinfo().update(repinfo_file, added, deleted, updated);
}

void Transaction::dump(FILE* out)
{
    db->repinfo().dump(out);
    db->station().dump(out);
    db->lev_tr().dump(out);
    db->lev_tr_cache().dump(out);
    db->data().dump(out);
    db->attr().dump(out);
}


// First part of initialising a dba_db
DB::DB(unique_ptr<Connection> conn)
    : conn(conn.release()),
      m_driver(v6::Driver::create(*this->conn).release()),
      m_repinfo(0), m_station(0), m_lev_tr(0), m_lev_tr_cache(0),
      m_data(0), m_attr(0)
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
    delete m_lev_tr_cache;
    delete m_lev_tr;
    delete m_station;
    delete m_repinfo;
    delete m_driver;
    delete conn;
}

v6::Driver& DB::driver()
{
    return *m_driver;
}

v6::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = m_driver->create_repinfov6().release();
    return *m_repinfo;
}

v6::Station& DB::station()
{
    if (m_station == NULL)
        m_station = m_driver->create_stationv6().release();
    return *m_station;
}

v6::LevTr& DB::lev_tr()
{
    if (m_lev_tr == NULL)
        m_lev_tr = m_driver->create_levtrv6().release();
    return *m_lev_tr;
}

v6::LevTrCache& DB::lev_tr_cache()
{
    if (m_lev_tr_cache == NULL)
        m_lev_tr_cache = v6::LevTrCache::create(lev_tr()).release();
    return *m_lev_tr_cache;
}

v6::DataV6& DB::data()
{
    if (m_data == NULL)
        m_data = m_driver->create_datav6().release();
    return *m_data;
}

v6::AttrV6& DB::attr()
{
    if (m_attr == NULL)
        m_attr = m_driver->create_attrv6().release();
    return *m_attr;
}

void DB::init_after_connect()
{
}

std::shared_ptr<dballe::db::Transaction> DB::transaction()
{
    auto res = conn->transaction();
    return make_shared<v6::Transaction>(dynamic_pointer_cast<v6::DB>(shared_from_this()), move(res));
}

std::shared_ptr<dballe::db::Transaction> DB::test_transaction()
{
    auto res = conn->transaction();
    return make_shared<v6::TestTransaction>(dynamic_pointer_cast<v6::DB>(shared_from_this()), move(res));
}

void DB::delete_tables()
{
    m_driver->delete_tables_v6();
}

void DB::disappear()
{
    m_driver->delete_tables_v6();

    // Invalidate the repinfo cache if we have a repinfo structure active
    if (m_repinfo)
    {
        delete m_repinfo;
        m_repinfo = 0;
    }
    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
}

void DB::reset(const char* repinfo_file)
{
    auto trc = trace.trace_reset(repinfo_file);
    disappear();
    m_driver->create_tables_v6();

    // Populate the tables with values
    auto tr = transaction();
    int added, deleted, updated;
    tr->update_repinfo(repinfo_file, &added, &deleted, &updated);
    tr->commit();

    trc->done();
}

std::map<std::string, int> DB::get_repinfo_priorities()
{
    return repinfo().get_priorities();
}

int DB::rep_cod_from_memo(const char* memo)
{
    return repinfo().obtain_id(memo);
}

int DB::obtain_station(const dballe::Station& st, bool can_add)
{
    // Look if the record already knows the ID
    if (st.ana_id != MISSING_INT)
        return st.ana_id;

    v6::Station& s = station();

    // Get the ID for the station
    if (can_add)
        return s.obtain_id(st.coords.lat, st.coords.lon, st.ident.get());
    else
        return s.get_id(st.coords.lat, st.coords.lon, st.ident.get());
}

void DB::vacuum()
{
    auto tr = trace.trace_vacuum();
    auto t = conn->transaction();
    driver().vacuum_v6();
    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
    t->commit();
    tr->done();
}

#if 0
    {
        /* List DSNs */
        char dsn[100], desc[100];
        short int len_dsn, len_desc, next;

        for (next = SQL_FETCH_FIRST;
                SQLDataSources(pc.od_env, next, dsn, sizeof(dsn),
                    &len_dsn, desc, sizeof(desc), &len_desc) == SQL_SUCCESS;
                next = SQL_FETCH_NEXT)
            printf("DSN %s (%s)\n", dsn, desc);
    }
#endif

}
}
}
