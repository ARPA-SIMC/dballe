#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/querybuf.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
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
    if (getenv("DBA_EXPLAIN") != NULL)
        explain_queries = true;

    auto tr = trace.trace_connect(this->conn->get_url());

    /* Set the connection timeout */
    /* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */

    tr->done();
}

DB::~DB()
{
    delete m_data;
    delete m_station_data;
    delete m_levtr;
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

v7::LevTr& DB::levtr()
{
    if (m_levtr == NULL)
        m_levtr = m_driver->create_levtr().release();
    return *m_levtr;
}

v7::StationData& DB::station_data()
{
    if (m_station_data == NULL)
        m_station_data = m_driver->create_station_data().release();
    return *m_station_data;
}

v7::Data& DB::data()
{
    if (m_data == NULL)
        m_data = m_driver->create_data().release();
    return *m_data;
}

std::unique_ptr<dballe::db::Transaction> DB::transaction()
{
    auto res = conn->transaction();
    return unique_ptr<dballe::db::Transaction>(new v7::Transaction(dynamic_pointer_cast<v7::DB>(shared_from_this()), move(res)));
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

void DB::vacuum()
{
    auto tr = trace.trace_vacuum();
    auto t = conn->transaction();
    driver().vacuum_v7();
    t->commit();
    tr->done();
}

void DB::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    auto& d = station_data();
    d.read_attrs(data_id, dest);
}

void DB::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    // Create the query
    auto& d = data();
    d.read_attrs(data_id, dest);
}

bool DB::is_station_variable(int data_id, wreport::Varcode varcode)
{
    return false;
}

void DB::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    levtr().dump(out);
    station_data().dump(out);
    data().dump(out);
}

}
}
}
