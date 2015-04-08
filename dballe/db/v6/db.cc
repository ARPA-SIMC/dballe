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
#include "dballe/db/modifiers.h"
#include "dballe/db/querybuf.h"
#include "cursor.h"
#include "internals.h"

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

/*
 * Database init queries
 */

static const char* init_tables[] = {
    // Delete 'context' to clean up pre-5.0 databases
    "attr", "data", "context", "lev_tr", "repinfo", "pseudoana"
};
static const char* init_sequences[] = {
    // Delete 'seq_pseudoana' to clean up pre-5.0 databases
    // Delete 'seq_context' to clean up pre-6.0 databases
    "seq_data", "seq_lev_tr", "seq_context", "seq_pseudoana"
};
#if 0
static const char* init_functions[] = {
/*  "identity (val anyelement, val1 anyelement)", */
};
#endif

/* TODO:
 * - Controllare che gli indici UNIQUE abbiano un senso, altrimenti tenerli solo
 *   per debug: ad ogni modo io in fase di insert devo fare una select, quindi...
 * - Controllare che l'indice unique di pseudoana abbia un senso quando ident is
 *   null, altrimenti si può pensare di toglierlo
 */

#define TABLETYPE "ENGINE=InnoDB;"
static const char* init_queries_mysql[] = {
    "CREATE TABLE repinfo ("
    "   id           SMALLINT PRIMARY KEY,"
    "   memo         VARCHAR(20) NOT NULL,"
    "   description  VARCHAR(255) NOT NULL,"
    "   prio         INTEGER NOT NULL,"
    "   descriptor   CHAR(6) NOT NULL,"
    "   tablea       INTEGER NOT NULL,"
    "   UNIQUE INDEX (prio),"
    "   UNIQUE INDEX (memo)"
    ") " TABLETYPE,
    "CREATE TABLE lev_tr ("
    "   id          INTEGER auto_increment PRIMARY KEY,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL,"
    "   UNIQUE INDEX (ltype1, l1, ltype2, l2, ptype, p1, p2)"
    ") " TABLETYPE,
    "CREATE TABLE data ("
    "   id          INTEGER auto_increment PRIMARY KEY,"
    "   id_station  SMALLINT NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   id_lev_tr   INTEGER NOT NULL,"
    "   datetime    DATETIME NOT NULL,"
    "   id_var      SMALLINT NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE INDEX(id_station, datetime, id_lev_tr, id_report, id_var),"
    "   INDEX(datetime),"
    "   INDEX(id_lev_tr)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_station) REFERENCES station (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_lev_tr) REFERENCES lev_tr (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
    ") " TABLETYPE,
    "CREATE TABLE attr ("
    "   id_data     INTEGER NOT NULL,"
    "   type        SMALLINT NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE INDEX (id_data, type)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_data) REFERENCES data (id) ON DELETE CASCADE"
#endif
    ") " TABLETYPE,
};

static const char* init_queries_postgres[] = {
    "CREATE TABLE repinfo ("
    "   id           INTEGER PRIMARY KEY,"
    "   memo         VARCHAR(30) NOT NULL,"
    "   description  VARCHAR(255) NOT NULL,"
    "   prio         INTEGER NOT NULL,"
    "   descriptor   CHAR(6) NOT NULL,"
    "   tablea       INTEGER NOT NULL"
    ") ",
    "CREATE UNIQUE INDEX ri_memo_uniq ON repinfo(memo)",
    "CREATE UNIQUE INDEX ri_prio_uniq ON repinfo(prio)",
    "CREATE TABLE lev_tr ("
    "   id          SERIAL PRIMARY KEY,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL"
    ") ",
    "CREATE UNIQUE INDEX lev_tr_uniq ON lev_tr(ltype1, l1, ltype2, l2, ptype, p1, p2)",
    "CREATE TABLE data ("
    "   id          SERIAL PRIMARY KEY,"
    "   id_station  INTEGER NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   id_lev_tr   INTEGER NOT NULL,"
    "   datetime    TIMESTAMP NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_station) REFERENCES station (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_lev_tr) REFERENCES lev_tr (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE UNIQUE INDEX data_uniq on data(id_station, datetime, id_lev_tr, id_report, id_var)",
    "CREATE INDEX data_ana ON data(id_station)",
    "CREATE INDEX data_report ON data(id_report)",
    "CREATE INDEX data_dt ON data(datetime)",
    "CREATE INDEX data_lt ON data(id_lev_tr)",
    "CREATE TABLE attr ("
    "   id_data     INTEGER NOT NULL,"
    "   type        INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_data) REFERENCES data (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE UNIQUE INDEX attr_uniq ON attr(id_data, type)",
/*
 * Not a good idea: it works on ALL inserts, even on those that should fail
    "CREATE RULE data_insert_or_update AS "
    " ON INSERT TO data "
    " WHERE (new.id_context, new.id_var) IN ( "
    " SELECT id_context, id_var "
    " FROM data "
    " WHERE id_context=new.id_context AND id_var=new.id_var) "
    " DO INSTEAD "
    " UPDATE data SET value=new.value "
    " WHERE id_context=new.id_context AND id_var=new.id_var",
*/
    /*"CREATE FUNCTION identity (val anyelement, val1 anyelement, OUT val anyelement) AS 'select $2' LANGUAGE sql STRICT",
    "CREATE AGGREGATE anyval ( basetype=anyelement, sfunc='identity', stype='anyelement' )",*/
};

static const char* init_queries_sqlite[] = {
    "CREATE TABLE repinfo ("
    "   id           INTEGER PRIMARY KEY,"
    "   memo         VARCHAR(30) NOT NULL,"
    "   description  VARCHAR(255) NOT NULL,"
    "   prio         INTEGER NOT NULL,"
    "   descriptor   CHAR(6) NOT NULL,"
    "   tablea       INTEGER NOT NULL,"
    "   UNIQUE (prio),"
    "   UNIQUE (memo)"
    ") ",
    "CREATE TABLE lev_tr ("
    "   id         INTEGER PRIMARY KEY,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL,"
    "   UNIQUE (ltype1, l1, ltype2, l2, ptype, p1, p2)"
    ") ",
    "CREATE TABLE data ("
    "   id          INTEGER PRIMARY KEY,"
    "   id_station  INTEGER NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   id_lev_tr   INTEGER NOT NULL,"
    "   datetime    TEXT NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE (id_station, datetime, id_lev_tr, id_report, id_var)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_station) REFERENCES station (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_lev_tr) REFERENCES lev_tr (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE INDEX data_report ON data(id_report)",
    "CREATE INDEX data_lt ON data(id_lev_tr)",
    "CREATE TABLE attr ("
    "   id_data     INTEGER NOT NULL,"
    "   type        INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE (id_data, type)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_data) REFERENCES data (id) ON DELETE CASCADE"
#endif
    ") "
};

static const char* init_queries_oracle[] = {
    "CREATE TABLE repinfo ("
    "   id           INTEGER PRIMARY KEY,"
    "   memo         VARCHAR2(30) NOT NULL,"
    "   description  VARCHAR2(255) NOT NULL,"
    "   prio         INTEGER NOT NULL,"
    "   descriptor   CHAR(6) NOT NULL,"
    "   tablea       INTEGER NOT NULL,"
    "   UNIQUE (prio),"
    "   UNIQUE (memo)"
    ") ",
    "CREATE TABLE lev_tr ("
    "   id          INTEGER PRIMARY KEY,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL,"
    ") ",
    "CREATE SEQUENCE seq_lev_tr",
    "CREATE UNIQUE INDEX lev_tr_uniq ON lev_tr(ltype1, l1, ltype2, l2, ptype, p1, p2)"
    "CREATE TABLE data ("
    "   id          SERIAL PRIMARY KEY,"
    "   id_station  INTEGER NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   id_lev_tr   INTEGER NOT NULL,"
    "   datetime    DATE NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_station) REFERENCES station (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
    "   , FOREIGN KEY (id_lev_tr) REFERENCES lev_tr (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE UNIQUE INDEX data_uniq(id_station, datetime, id_lev_tr, id_report, id_var)",
    "CREATE INDEX data_sta ON data(id_station)",
    "CREATE INDEX data_rep ON data(id_report)",
    "CREATE INDEX data_dt ON data(datetime)",
    "CREATE INDEX data_lt ON data(id_lev_tr)",
    "CREATE TABLE attr ("
    "   id_data     INTEGER NOT NULL,"
    "   type        INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_data) REFERENCES data (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE UNIQUE INDEX attr_uniq ON attr(id_data, type)",
};

/*
 * DB implementation
 */


// First part of initialising a dba_db
DB::DB(unique_ptr<Connection> conn)
    : conn(conn.release()),
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
    if (m_attr) delete m_attr;
    if (m_data) delete m_data;
    if (m_lev_tr_cache) delete m_lev_tr_cache;
    if (m_lev_tr) delete m_lev_tr;
    if (m_station) delete m_station;
    if (m_repinfo) delete m_repinfo;
    if (conn) delete conn;
}

sql::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = create_repinfo(*conn).release();
    return *m_repinfo;
}

sql::Station& DB::station()
{
    if (m_station == NULL)
        m_station = create_station(*conn).release();
    return *m_station;
}

sql::LevTr& DB::lev_tr()
{
    if (m_lev_tr == NULL)
        m_lev_tr = create_levtr(*conn).release();
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
        m_data = create_datav6(*conn).release();
    return *m_data;
}

sql::AttrV6& DB::attr()
{
    if (m_attr == NULL)
        m_attr = create_attrv6(*conn).release();
    return *m_attr;
}

void DB::init_after_connect()
{
    /* Set manual commit */
    if (conn->server_type == ServerType::SQLITE)
    {
        if (getenv("DBA_INSECURE_SQLITE") != NULL)
        {
            conn->exec("PRAGMA synchronous = OFF");
            conn->exec("PRAGMA journal_mode = OFF");
            conn->exec("PRAGMA legacy_file_format = 0");
        } else {
            conn->exec("PRAGMA journal_mode = MEMORY");
            conn->exec("PRAGMA legacy_file_format = 0");
        }
    }
}

void DB::delete_tables()
{
    /* Drop existing tables */
    for (size_t i = 0; i < sizeof(init_tables) / sizeof(init_tables[0]); ++i)
        conn->drop_table_if_exists(init_tables[i]);

    /* Drop existing sequences */
    for (size_t i = 0; i < sizeof(init_sequences) / sizeof(init_sequences[0]); ++i)
        conn->drop_sequence_if_exists(init_sequences[i]);
}

void DB::disappear()
{
    sql::Station::reset_db(*conn);

    // Drop existing tables
    delete_tables();

    // Invalidate the repinfo cache if we have a repinfo structure active
    if (m_repinfo)
    {
        delete m_repinfo;
        m_repinfo = 0;
    }

    conn->drop_settings();
}

void DB::reset(const char* repinfo_file)
{
    disappear();

    const char** queries = NULL;
    int query_count = 0;
    switch (conn->server_type)
    {
        case ServerType::MYSQL:
            queries = init_queries_mysql;
            query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
        case ServerType::SQLITE:
            queries = init_queries_sqlite;
            query_count = sizeof(init_queries_sqlite) / sizeof(init_queries_sqlite[0]); break;
        case ServerType::ORACLE:
            queries = init_queries_oracle;
            query_count = sizeof(init_queries_oracle) / sizeof(init_queries_oracle[0]); break;
        case ServerType::POSTGRES:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
        default:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
    }
    /* Create tables */
    for (int i = 0; i < query_count; i++)
        conn->exec(queries[i]);

    /* Populate the tables with values */
    {
        int added, deleted, updated;
        repinfo().update(repinfo_file, &added, &deleted, &updated);
    }
    conn->set_setting("version", "V6");
    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
}

void DB::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    repinfo().update(repinfo_file, added, deleted, updated);
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

    int id_station = obtain_station(rec, station_can_add);

    d.set_context(
            // Insert the station data, and get the ID
            id_station,
            // Get the ID of the report
            get_rep_cod(rec),
            // Insert the lev_tr data, and get the ID
            obtain_lev_tr(rec)
    );

    // Set the date from the record contents
    d.set_date(rec);

    // Reset the variable ID store
    last_insert_varids.clear();

    // Insert all the variables we find
    for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
    {
        int id;
        // Datum to be inserted, linked to id_station and all the other IDs
        if (can_replace)
            d.insert_or_overwrite(**i, &id);
        else
            d.insert_or_fail(**i, &id);
        last_insert_varids.push_back(VarID((*i)->code(), id));
    }
    t->commit();

    _last_station_id = id_station;
}

int DB::last_station_id() const
{
    return _last_station_id;
}

void DB::remove(const Query& query)
{
    Cursor::run_delete_query(*this, query);
}

void DB::remove_all()
{
    auto t = conn->transaction();
    conn->exec("DELETE FROM attr");
    conn->exec("DELETE FROM data");
    conn->exec("DELETE FROM lev_tr");
    conn->exec("DELETE FROM station");
    t->commit();
}

void DB::vacuum()
{
    static const char* cclean_mysql = "delete c from lev_tr c left join data d on d.id_lev_tr = c.id where d.id_lev_tr is NULL";
    static const char* pclean_mysql = "delete p from station p left join data d on d.id_station = p.id where d.id is NULL";
    static const char* cclean_sqlite = "delete from lev_tr where id in (select ltr.id from lev_tr ltr left join data d on d.id_lev_tr = ltr.id where d.id_lev_tr is NULL)";
    static const char* pclean_sqlite = "delete from station where id in (select p.id from station p left join data d on d.id_station = p.id where d.id is NULL)";
    static const char* cclean = NULL;
    static const char* pclean = NULL;

    switch (conn->server_type)
    {
        case ServerType::MYSQL: cclean = cclean_mysql; pclean = pclean_mysql; break;
        case ServerType::SQLITE: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        case ServerType::ORACLE: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        case ServerType::POSTGRES: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        default: cclean = cclean_mysql; pclean = pclean_mysql; break;
    }

    auto t = conn->transaction();

    // Delete orphan lev_trs
    conn->exec(cclean);

    // Delete orphan stations
    conn->exec(pclean);

    t->commit();

    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
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
    conn->exec(query);
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

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
