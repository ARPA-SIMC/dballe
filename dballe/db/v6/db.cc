/*
 * dballe/v6/db - Archive for point-based meteorological data, db layout version 6
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/db/internals.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/v6/repinfo.h"
#include "dballe/db/v5/station.h"
#include "lev_tr.h"
#include "data.h"
#include "cursor.h"
#include "attr.h"

#include <dballe/core/record.h>
#include <dballe/core/defs.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits.h>
#include <unistd.h>

#include <sql.h>

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


#ifdef DBA_USE_TRANSACTIONS
#define TABLETYPE "ENGINE=InnoDB;"
#else
#define TABLETYPE ";"
#endif
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
DB::DB(auto_ptr<Connection>& conn)
    : conn(conn.release()),
      m_repinfo(0), m_station(0), m_lev_tr(0), m_lev_tr_cache(0),
      m_data(0), m_attr(0),
      seq_lev_tr(0), seq_data(0), _last_station_id(0)
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
    if (seq_data) delete seq_data;
    if (seq_lev_tr) delete seq_lev_tr;
    if (conn) delete conn;
}

v6::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = new Repinfo(conn);
    return *m_repinfo;
}

v5::Station& DB::station()
{
    if (m_station == NULL)
        m_station = new v5::Station(*this->conn);
    return *m_station;
}

LevTr& DB::lev_tr()
{
    if (m_lev_tr == NULL)
        m_lev_tr = new v6::LevTr(*this);
    return *m_lev_tr;
}

LevTrCache& DB::lev_tr_cache()
{
    if (m_lev_tr_cache == NULL)
        m_lev_tr_cache = v6::LevTrCache::create(lev_tr()).release();
    return *m_lev_tr_cache;
}

Data& DB::data()
{
    if (m_data == NULL)
        m_data = new v6::Data(*this);
    return *m_data;
}

Attr& DB::attr()
{
    if (m_attr == NULL)
        m_attr = new v6::Attr(*conn);
    return *m_attr;
}

void DB::init_after_connect()
{
#ifdef DBA_USE_TRANSACTIONS
    /* Set manual commit */
    if (conn->server_type == db::SQLITE)
    {
        if (getenv("DBA_INSECURE_SQLITE") != NULL)
        {
            run_sql("PRAGMA synchronous = OFF");
            run_sql("PRAGMA journal_mode = OFF");
            run_sql("PRAGMA legacy_file_format = 0");
        } else {
            run_sql("PRAGMA journal_mode = MEMORY");
            run_sql("PRAGMA legacy_file_format = 0");
        }
    }
    conn->set_autocommit(false);
#endif
}

void DB::run_sql(const char* query)
{
    db::Statement stm(*conn);
    stm.exec_direct_and_close(query);
}

#define DBA_ODBC_MISSING_FUNCTION_POSTGRES "42883"

void DB::delete_tables()
{
    /* Drop existing tables */
    for (size_t i = 0; i < sizeof(init_tables) / sizeof(init_tables[0]); ++i)
        conn->drop_table_if_exists(init_tables[i]);

    /* Drop existing sequences */
    for (size_t i = 0; i < sizeof(init_sequences) / sizeof(init_sequences[0]); ++i)
        conn->drop_sequence_if_exists(init_sequences[i]);

#if 0
    /* Allocate statement handle */
    DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

    /* Drop existing functions */
    for (i = 0; i < sizeof(init_functions) / sizeof(init_functions[0]); i++)
    {
        char buf[200];
        int len;

        switch (db->server_type)
        {
            case MYSQL:
            case SQLITE:
            case ORACLE:
                /* No functions used by MySQL, SQLite and Oracle */
                break;
            case POSTGRES:
                len = snprintf(buf, 100, "DROP FUNCTION %s CASCADE", init_functions[i]);
                res = SQLExecDirect(stm, (unsigned char*)buf, len);
                if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
                {
                    err = dba_db_error_odbc_except(DBA_ODBC_MISSING_FUNCTION_POSTGRES, SQL_HANDLE_STMT, stm,
                            "Removing old function %s", init_functions[i]);
                    if (err != DBA_OK)
                        goto cleanup;
                }
                DBA_RUN_OR_GOTO(cleanup, dba_db_commit(db));
                break;
            default:
                /* No sequences in unknown databases */
                break;
        }
    }
#endif
}

void DB::disappear()
{
    v5::Station::reset_db(*conn);

    // Drop existing tables
    delete_tables();

    // Invalidate the repinfo cache if we have a repinfo structure active
    if (m_repinfo)
        m_repinfo->invalidate_cache();

    conn->drop_settings();
}

void DB::reset(const char* repinfo_file)
{
    disappear();

    /* Allocate statement handle */
    db::Statement stm(*conn);

    const char** queries = NULL;
    int query_count = 0;
    switch (conn->server_type)
    {
        case db::MYSQL:
            queries = init_queries_mysql;
            query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
        case db::SQLITE:
            queries = init_queries_sqlite;
            query_count = sizeof(init_queries_sqlite) / sizeof(init_queries_sqlite[0]); break;
        case db::ORACLE:
            queries = init_queries_oracle;
            query_count = sizeof(init_queries_oracle) / sizeof(init_queries_oracle[0]); break;
        case db::POSTGRES:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
        default:
            queries = init_queries_postgres;
            query_count = sizeof(init_queries_postgres) / sizeof(init_queries_postgres[0]); break;
    }
    /* Create tables */
    for (int i = 0; i < query_count; i++)
        stm.exec_direct_and_close(queries[i]);

    /* Populate the tables with values */
    {
        int added, deleted, updated;
        repinfo().update(repinfo_file, &added, &deleted, &updated);
    }
    conn->set_setting("version", "V6");
    conn->commit();
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

int DB::get_rep_cod(const Record& rec)
{
    v6::Repinfo& ri = repinfo();
    if (const char* memo = rec.key_peek_value(DBA_KEY_REP_MEMO))
        return ri.get_id(memo);
    else
        throw error_notfound("input record has neither rep_cod nor rep_memo");
}

int DB::rep_cod_from_memo(const char* memo)
{
    return repinfo().obtain_id(memo);
}

bool DB::check_rep_cod(int rep_cod)
{
    return repinfo().has_id(rep_cod);
}

int DB::last_lev_tr_insert_id()
{
    if (seq_lev_tr)
        return seq_lev_tr->read();
    else
        return conn->get_last_insert_id();
}

int DB::last_data_insert_id()
{
    if (seq_data)
        return seq_data->read();
    else
        return conn->get_last_insert_id();
}

// Normalise longitude values to the [-180..180[ interval
static inline int normalon(int lon)
{
    return ((lon + 18000000) % 36000000) - 18000000;
}

int DB::obtain_station(const Record& rec, bool can_add)
{
    // Look if the record already knows the ID
    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_ID))
        return strtol(val, 0, 10);

    v5::Station& s = station();

    // Look for the key data in the record
    if (const Var* var = rec.key_peek(DBA_KEY_LAT))
        s.lat = var->enqi();
    else
        throw error_notfound("no latitude in record when trying to insert a station in the database");

    if (const Var* var = rec.key_peek(DBA_KEY_LON))
        s.lon = normalon(var->enqi());
    else
        throw error_notfound("no longitude in record when trying to insert a station in the database");

    if (const char* val = rec.key_peek_value(DBA_KEY_IDENT))
        s.set_ident(val);
    else
        s.set_ident(NULL);

    // Check for an existing station with these data
    int id = s.get_id();

    /* If not found, insert a new one */
    if (id == -1)
    {
        if (can_add)
            id = s.insert();
        else
            throw error_consistency("trying to insert a station entry when it is forbidden");
    }

    return id;
}

int DB::obtain_lev_tr(const Record& rec)
{
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE1))
        if (var->enqi() == 257)
            return -1;

    LevTr& c = lev_tr();

    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE1))
        c.ltype1 = var->enqi();
    else
        c.ltype1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L1))
        c.l1 = var->enqi();
    else
        c.l1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_LEVELTYPE2))
        c.ltype2 = var->enqi();
    else
        c.ltype2 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_L2))
        c.l2 = var->enqi();
    else
        c.l2 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_PINDICATOR))
        c.pind = var->enqi();
    else
        c.pind = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P1))
        c.p1 = var->enqi();
    else
        c.p1 = MISSING_INT;
    if (const Var* var = rec.key_peek(DBA_KEY_P2))
        c.p2 = var->enqi();
    else
        c.p2 = MISSING_INT;

    // Check for an existing lev_tr with these data
    int id = c.get_id();

    /* If there is an existing record, use its ID and don't do an INSERT */
    if (id == -1)
        id = c.insert();

    return id;
}

void DB::insert(const Record& rec, bool can_replace, bool station_can_add)
{
    v6::Data& d = data();

    /* Check for the existance of non-lev_tr data, otherwise it's all
     * useless.  Not inserting data is fine in case of setlev_trana */
    const char* s_year;
    if (rec.vars().empty() && !(((s_year = rec.key_peek_value(DBA_KEY_YEAR)) != NULL) && strcmp(s_year, "1000") == 0))
        throw error_notfound("no variables found in input record");

    db::Transaction t(*conn);

    // Insert the station data, and get the ID
    d.id_station = obtain_station(rec, station_can_add);

    /* Get the ID of the report */
    d.id_report = get_rep_cod(rec);

    // Insert the lev_tr data, and get the ID
    d.id_lev_tr = obtain_lev_tr(rec);

    // Set the date from the record contents
    d.set_date(rec);

    // Reset the variable ID store
    last_insert_varids.clear();

    // Insert all the variables we find
    for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
    {
        /* Datum to be inserted, linked to id_station and all the other IDs */
        d.set(**i);
        if (can_replace)
            d.insert_or_overwrite(true);
        else
            d.insert_or_fail(true);
        last_insert_varids.push_back(VarID((*i)->code(), d.id));
    }
    t.commit();

    _last_station_id = d.id_station;
}

int DB::last_station_id() const
{
    return _last_station_id;
}

void DB::remove(const Record& rec)
{
    db::Transaction t(*conn);

    // Get the list of data to delete
    v6::CursorDataIDs c(*this, DBA_DB_MODIFIER_UNSORTED | DBA_DB_MODIFIER_STREAM);
    c.query(rec);

    // Compile the DELETE query for the data
    db::Statement stmd(*conn);
    stmd.bind_in(1, c.sqlrec.out_id_data);
    stmd.prepare("DELETE FROM data WHERE id=?");

    // Compile the DELETE query for the attributes
    db::Statement stma(*conn);
    stma.bind_in(1, c.sqlrec.out_id_data);
    stma.prepare("DELETE FROM attr WHERE id_data=?");

    /* Iterate all the results, deleting them */
    while (c.next())
    {
        stmd.execute_and_close();
        stma.execute_and_close();
    }
    t.commit();
}

void DB::remove_all()
{
    db::Transaction t(*conn);
    db::Statement stm(*conn);
    stm.exec_direct_and_close("DELETE FROM attr");
    stm.exec_direct_and_close("DELETE FROM data");
    stm.exec_direct_and_close("DELETE FROM lev_tr");
    stm.exec_direct_and_close("DELETE FROM station");
    t.commit();
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
        case db::MYSQL: cclean = cclean_mysql; pclean = pclean_mysql; break;
        case db::SQLITE: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        case db::ORACLE: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        case db::POSTGRES: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
        default: cclean = cclean_mysql; pclean = pclean_mysql; break;
    }

    db::Transaction t(*conn);

    // Delete orphan lev_trs
    db::Statement stm(*conn);
    stm.exec_direct_and_close(cclean);

#if 0
    /* Done with lev_tr */
    res = SQLCloseCursor(stm);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
        return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_remove_orphans cursor");
#endif

    // Delete orphan stations
    stm.exec_direct_and_close(pclean);

    t.commit();

    if (m_lev_tr_cache)
        m_lev_tr_cache->invalidate();
}

std::auto_ptr<db::Cursor> DB::query_stations(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query) | DBA_DB_MODIFIER_ANAEXTRA | DBA_DB_MODIFIER_DISTINCT;
    auto_ptr<Cursor> res(new CursorStations(*this, modifiers));
    res->query(query);
    return auto_ptr<db::Cursor>(res.release());
}

std::auto_ptr<db::Cursor> DB::query_data(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    auto_ptr<Cursor> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
        res.reset(new CursorBest(*this, modifiers));
    else
        res.reset(new CursorData(*this, modifiers));
    res->query(query);
    return auto_ptr<db::Cursor>(res.release());
}

std::auto_ptr<db::Cursor> DB::query_summary(const Record& rec)
{
    auto_ptr<Cursor> res(new CursorSummary(*this, 0));
    res->query(rec);
    return auto_ptr<db::Cursor>(res.release());
}

unsigned DB::query_attrs(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs, Record& attrs)
{
    // Create the query
    Querybuf query(200);
    if (qcs.empty())
        /* If qcs is null, query all QC data */
        query.append(
                "SELECT type, value"
                "  FROM attr"
                " WHERE id_data = ?");
    else {
        query.append(
                "SELECT type, value"
                "  FROM attr"
                " WHERE id_data = ? AND type IN (");
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }

    // Perform the query
    DBALLE_SQL_C_SINT_TYPE in_id_data = id_data;
    Varcode out_type;
    char out_value[255];

    db::Statement stm(*conn);
    stm.bind_in(1, in_id_data);
    stm.bind_out(1, out_type);
    stm.bind_out(2, out_value, 255);

    TRACE("attr read query: %s with id_data %d var %01d%02d%03d\n", query.c_str(), id_data,
            WR_VAR_F(id_var), WR_VAR_X(id_var), WR_VAR_Y(id_var));

    stm.exec_direct(query.c_str());

    // Retrieve results
    attrs.clear();

    // Fetch the results
    int count;
    for (count = 0; stm.fetch(); ++count)
        attrs.var(out_type).setc(out_value);

    return count;
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
    v6::Attr& a = attr();

    a.id_data = id_data;

    // Begin the transaction
    db::Transaction t(*conn);

    /* Insert all found variables */
    for (vector<Var*>::const_iterator i = attrs.vars().begin(); i != attrs.vars().end(); ++i)
    {
        a.set(**i);
        a.insert();
    }

    t.commit();
}

void DB::attr_remove(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs)
{
    // Create the query
    Querybuf query(500);
    if (qcs.empty())
        query.append("DELETE FROM attr WHERE id_data = ?");
    else {
        query.append("DELETE FROM attr WHERE id_data = ? AND type IN (");
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }

    // dba_verbose(DBA_VERB_DB_SQL, "Performing query %s for id %d,B%02d%03d\n", query, id_lev_tr, DBA_VAR_X(id_var), DBA_VAR_Y(id_var));

    DBALLE_SQL_C_SINT_TYPE in_id_data = id_data;

    db::Statement stm(*conn);
    stm.bind_in(1, in_id_data);
    stm.exec_direct_and_close(query.c_str());
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
