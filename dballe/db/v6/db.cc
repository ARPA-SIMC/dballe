/*
 * dballe/v6/db - Archive for point-based meteorological data, db layout version 6
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "db.h"
#include "dballe/db/sql.h"
#include "dballe/db/sql/driver.h"
#include "dballe/db/sql/repinfo.h"
#include "dballe/db/sql/station.h"
#include "dballe/db/sql/levtr.h"
#include "dballe/db/sql/datav6.h"
#include "dballe/db/sql/attrv6.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/querybuf.h"
#include "cursor.h"

#include <dballe/core/record.h>
#include <dballe/core/defs.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <unistd.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

// First part of initialising a dba_db
DB::DB(unique_ptr<Connection> conn)
    : conn(conn.release()),
      m_driver(sql::Driver::create(*this->conn).release()),
      m_repinfo(0), m_station(0), m_lev_tr(0), m_lev_tr_cache(0),
      m_data(0), m_attr(0),
      _last_station_id(0)
{
    init_after_connect();

    /* Set the connection timeout */
    /* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */
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

sql::Driver& DB::driver()
{
    return *m_driver;
}

sql::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = m_driver->create_repinfov6().release();
    return *m_repinfo;
}

sql::Station& DB::station()
{
    if (m_station == NULL)
        m_station = m_driver->create_stationv6().release();
    return *m_station;
}

sql::LevTr& DB::lev_tr()
{
    if (m_lev_tr == NULL)
        m_lev_tr = m_driver->create_levtrv6().release();
    return *m_lev_tr;
}

sql::LevTrCache& DB::lev_tr_cache()
{
    if (m_lev_tr_cache == NULL)
        m_lev_tr_cache = sql::LevTrCache::create(lev_tr()).release();
    return *m_lev_tr_cache;
}

sql::DataV6& DB::data()
{
    if (m_data == NULL)
        m_data = m_driver->create_datav6().release();
    return *m_data;
}

sql::AttrV6& DB::attr()
{
    if (m_attr == NULL)
        m_attr = m_driver->create_attrv6().release();
    return *m_attr;
}

void DB::init_after_connect()
{
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
    disappear();
    m_driver->create_tables_v6();

    // Populate the tables with values
    int added, deleted, updated;
    update_repinfo(repinfo_file, &added, &deleted, &updated);
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

int DB::get_rep_cod(const Query& rec)
{
    sql::Repinfo& ri = repinfo();
    if (const char* memo = rec.key_peek_value(DBA_KEY_REP_MEMO))
        return ri.get_id(memo);
    else
        throw error_notfound("input record has neither rep_cod nor rep_memo");
}

int DB::rep_cod_from_memo(const char* memo)
{
    return repinfo().obtain_id(memo);
}

// Normalise longitude values to the [-180..180[ interval
static inline int normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}

int DB::obtain_station(const Query& rec, bool can_add)
{
    // Look if the record already knows the ID
    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_ID))
        return strtol(val, 0, 10);

    sql::Station& s = station();

    // Look for the key data in the record
    int lat;
    if (const Var* var = rec.key_peek(DBA_KEY_LAT))
        lat = var->enqi();
    else
        throw error_notfound("no latitude in record when trying to insert a station in the database");

    int lon;
    if (const Var* var = rec.key_peek(DBA_KEY_LON))
        lon = normalon(var->enqi());
    else
        throw error_notfound("no longitude in record when trying to insert a station in the database");

    const char* ident = rec.key_peek_value(DBA_KEY_IDENT);

    // Get the ID for the station
    if (can_add)
        return s.obtain_id(lat, lon, ident);
    else
        return s.get_id(lat, lon, ident);
}

int DB::obtain_lev_tr(const Query& rec)
{
    if (rec.is_ana_context())
        return -1;

    return lev_tr().obtain_id(rec);
}

void DB::insert(const Query& rec, bool can_replace, bool station_can_add)
{
    sql::DataV6& d = data();

    /* Check for the existance of non-lev_tr data, otherwise it's all
     * useless.  Not inserting data is fine in case of setlev_trana */
    const char* s_year;
    if (rec.vars().empty() && !(((s_year = rec.key_peek_value(DBA_KEY_YEAR)) != NULL) && strcmp(s_year, "1000") == 0))
        throw error_notfound("no variables found in input record");

    auto t = conn->transaction();

    sql::bulk::InsertV6 vars;
    // Insert the station data, and get the ID
    vars.id_station = obtain_station(rec, station_can_add);
    // Get the ID of the report
    vars.id_report = get_rep_cod(rec);
    // Set the date from the record contents
    vars.datetime = rec.get_datetime();

    // Insert the lev_tr data, and get the ID
    int id_levtr = obtain_lev_tr(rec);

    // Reset the variable ID store
    last_insert_varids.clear();

    // Add all the variables we find
    for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        vars.add(*i, id_levtr);

    // Do the insert
    d.insert(*t, vars, can_replace ? sql::DataV6::UPDATE : sql::DataV6::ERROR);

    // Read the IDs from the results
    for (const auto& v: vars)
        if (v.inserted())
            last_insert_varids.push_back(VarID(v.var->code(), v.id_data));
    t->commit();

    _last_station_id = vars.id_station;
}

int DB::last_station_id() const
{
    return _last_station_id;
}

void DB::remove(const Query& query)
{
    auto t = conn->transaction();
    Cursor::run_delete_query(*this, query);
    t->commit();
}

void DB::remove_all()
{
    auto t = conn->transaction();
    driver().remove_all_v6();
    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
    t->commit();
}

void DB::vacuum()
{
    auto t = conn->transaction();
    driver().vacuum_v6();
    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
    t->commit();
}

std::unique_ptr<db::Cursor> DB::query_stations(const Query& query)
{
    return Cursor::run_station_query(*this, query);
}

std::unique_ptr<db::Cursor> DB::query_data(const Query& query)
{
    return Cursor::run_data_query(*this, query);
}

std::unique_ptr<db::Cursor> DB::query_summary(const Query& query)
{
    return Cursor::run_summary_query(*this, query);
}

void DB::query_attrs(int id_data, wreport::Varcode id_var,
        std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    // Create the query
    sql::AttrV6& a = attr();
    a.read(id_data, dest);
}

void DB::attr_insert(wreport::Varcode id_var, const Record& attrs)
{
    // Find the data id for id_var
    for (vector<VarID>::const_iterator i = last_insert_varids.begin();
            i != last_insert_varids.end(); ++i)
        if (i->code == id_var)
        {
            attr_insert(i->id, id_var, attrs);
            return;
        }
    error_notfound::throwf("variable B%02d%03d was not involved in the last insert operation", WR_VAR_X(id_var), WR_VAR_Y(id_var));
}

void DB::attr_insert(int id_data, wreport::Varcode id_var, const Record& attrs)
{
    sql::AttrV6& a = attr();

    // Begin the transaction
    auto t = conn->transaction();

    /* Insert all found variables */
    a.add(id_data, attrs);

    t->commit();
}

void DB::attr_remove(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs)
{
    Querybuf query(500);
    if (qcs.empty())
        // Delete all attributes
        query.appendf("DELETE FROM attr WHERE id_data=%d", id_data);
    else {
        // Delete only the attributes in qcs
        query.appendf("DELETE FROM attr WHERE id_data=%d AND type IN (", id_data);
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }
    driver().exec_no_data(query);
    // dba_verbose(DBA_VERB_DB_SQL, "Performing query %s for id %d,B%02d%03d\n", query, id_lev_tr, DBA_VAR_X(id_var), DBA_VAR_Y(id_var));
}

void DB::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    lev_tr().dump(out);
    lev_tr_cache().dump(out);
    data().dump(out);
    attr().dump(out);
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
