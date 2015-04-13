/*
 * dballe/db - Archive for point-based meteorological data
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
#include "config.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/sql/driver.h"
#include "dballe/db/sql/repinfo.h"
#include "dballe/db/sql/station.h"
#include "context.h"
#include "dballe/db/sql/datav5.h"
#include "dballe/db/sql/attrv5.h"
#ifdef HAVE_ODBC
#include "dballe/db/odbc/internals.h"
#endif
#include "cursor.h"

#include <dballe/core/record.h>
#include <dballe/core/defs.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <sql.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace v5 {

/*
 * Database init queries
 */

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
    "CREATE SEQUENCE seq_context",
    "CREATE TABLE context ("
    "   id          SERIAL PRIMARY KEY,"
    "   id_ana      INTEGER NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   datetime    TIMESTAMP NOT NULL,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_ana) REFERENCES station (id) ON DELETE CASCADE,"
    "   FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE UNIQUE INDEX co_uniq ON context(id_ana, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2, id_report)",
    "CREATE INDEX co_ana ON context(id_ana)",
    "CREATE INDEX co_report ON context(id_report)",
    "CREATE INDEX co_dt ON context(datetime)",
    "CREATE INDEX co_lt ON context(ltype1, l1, ltype2, l2)",
    "CREATE INDEX co_pt ON context(ptype, p1, p2)",
    "CREATE TABLE data ("
    "   id_context  INTEGER NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_context) REFERENCES context (id) ON DELETE CASCADE"
#endif
    ") ",
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
    "CREATE INDEX da_co ON data(id_context)",
    "CREATE UNIQUE INDEX da_uniq ON data(id_var, id_context)",
    "CREATE TABLE attr ("
    "   id_context  INTEGER NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   type        INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_context, id_var) REFERENCES data (id_context, id_var) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE INDEX at_da ON attr(id_context, id_var)",
    "CREATE UNIQUE INDEX at_uniq ON attr(id_context, id_var, type)",
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
    "CREATE TABLE context ("
    "   id          INTEGER PRIMARY KEY,"
    "   id_ana      INTEGER NOT NULL,"
    "   id_report   INTEGER NOT NULL,"
    "   datetime    TEXT NOT NULL,"
    "   ltype1      INTEGER NOT NULL,"
    "   l1          INTEGER NOT NULL,"
    "   ltype2      INTEGER NOT NULL,"
    "   l2          INTEGER NOT NULL,"
    "   ptype       INTEGER NOT NULL,"
    "   p1          INTEGER NOT NULL,"
    "   p2          INTEGER NOT NULL,"
    "   UNIQUE (id_ana, datetime, ltype1, l1, ltype2, l2, ptype, p1, p2, id_report)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_ana) REFERENCES station (id) ON DELETE CASCADE,"
    "   FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE INDEX co_ana ON context(id_ana)",
    "CREATE INDEX co_report ON context(id_report)",
    "CREATE INDEX co_dt ON context(datetime)",
    "CREATE INDEX co_lt ON context(ltype1, l1, ltype2, l2)",
    "CREATE INDEX co_pt ON context(ptype, p1, p2)",
    "CREATE TABLE data ("
    "   id_context  INTEGER NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE (id_var, id_context)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_context) REFERENCES context (id) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE INDEX da_co ON data(id_context)",
    "CREATE TABLE attr ("
    "   id_context  INTEGER NOT NULL,"
    "   id_var      INTEGER NOT NULL,"
    "   type        INTEGER NOT NULL,"
    "   value       VARCHAR(255) NOT NULL,"
    "   UNIQUE (id_context, id_var, type)"
#ifdef USE_REF_INT
    "   , FOREIGN KEY (id_context, id_var) REFERENCES data (id_context, id_var) ON DELETE CASCADE"
#endif
    ") ",
    "CREATE INDEX at_da ON attr(id_context, id_var)",
};


/*
 * DB implementation
 */


// First part of initialising a dba_db
DB::DB(unique_ptr<ODBCConnection> conn)
    : conn(conn.release()), m_driver(sql::Driver::create(*this->conn).release())
{
    init_after_connect();

    /* Set the connection timeout */
    /* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */
}

DB::~DB()
{
    delete m_attr;
    delete m_data;
    delete m_context;
    delete m_station;
    delete m_repinfo;
    delete m_driver;
    delete conn;
}

#if 0
void dba_db_delete(dba_db db)
{
    assert(db);

    if (db->attr != NULL)
        dba_db_attr_delete(db->attr);
    if (db->data != NULL)
        dba_db_data_delete(db->data);
    if (db->context != NULL)
        dba_db_context_delete(db->context);
    if (db->station != NULL)
        dba_db_station_delete(db->station);
}
#endif

sql::Repinfo& DB::repinfo()
{
    if (m_repinfo == NULL)
        m_repinfo = m_driver->create_repinfov5().release();
    return *m_repinfo;
}

sql::Station& DB::station()
{
    if (m_station == NULL)
        m_station = m_driver->create_stationv5().release();
    return *m_station;
}

Context& DB::context()
{
    if (m_context == NULL)
    {
        ODBCConnection* c = dynamic_cast<ODBCConnection*>(conn);
        if (!c) throw error_unimplemented("v5 DB Context only works with ODBC connectors");
        m_context = new Context(*c);
    }
    return *m_context;
}

sql::DataV5& DB::data()
{
    if (m_data == NULL)
        m_data = m_driver->create_datav5().release();
    return *m_data;
}

sql::AttrV5& DB::attr()
{
    if (m_attr == NULL)
        m_attr = m_driver->create_attrv5().release();
    return *m_attr;
}

void DB::init_after_connect()
{
}

#define DBA_ODBC_MISSING_FUNCTION_POSTGRES "42883"

void DB::delete_tables()
{
    m_driver->delete_tables_v5();
}


void DB::disappear()
{
    m_driver->delete_tables_v5();

    /* Invalidate the repinfo cache if we have a repinfo structure active */
    if (m_repinfo)
    {
        delete m_repinfo;
        m_repinfo = nullptr;
    }
}

void DB::reset(const char* repinfo_file)
{
    disappear();
    m_driver->create_tables_v5();

    /* Populate the tables with values */
    {
        int added, deleted, updated;
        repinfo().update(repinfo_file, &added, &deleted, &updated);
    }
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

#if 0
static dba_err update_station_extra_info(dba_db db, dba_record rec, int id_ana)
{
    dba_var var;
    
    /* Don't do anything if rec doesn't have any extra data */
    if (    dba_record_key_peek_value(rec, DBA_KEY_HEIGHT) == NULL
        &&  dba_record_key_peek_value(rec, DBA_KEY_HEIGHTBARO) == NULL
        &&  dba_record_key_peek_value(rec, DBA_KEY_NAME) == NULL
        &&  dba_record_key_peek_value(rec, DBA_KEY_BLOCK) == NULL
        &&  dba_record_key_peek_value(rec, DBA_KEY_STATION) == NULL)
        return dba_error_ok();

    /* Get the id of the ana context */
    db->context->id_ana = id_ana;
    db->context->id_report = -1;
    DBA_RUN_OR_RETURN(dba_db_context_obtain_ana(db->context, &(db->data->id_context)));

    /* Insert or update the data that we find in record */
    if ((var = dba_record_key_peek(rec, DBA_KEY_BLOCK)) != NULL)
    {
        db->data->id_var = DBA_VAR(0, 1, 1);
        dba_db_data_set_value(db->data, dba_var_value(var));
        DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
    }
    if ((var = dba_record_key_peek(rec, DBA_KEY_STATION)) != NULL)
    {
        db->data->id_var = DBA_VAR(0, 1, 2);
        dba_db_data_set_value(db->data, dba_var_value(var));
        DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
    }
    if ((var = dba_record_key_peek(rec, DBA_KEY_NAME)) != NULL)
    {
        db->data->id_var = DBA_VAR(0, 1, 19);
        dba_db_data_set_value(db->data, dba_var_value(var));
        DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
    }
    if ((var = dba_record_key_peek(rec, DBA_KEY_HEIGHT)) != NULL)
    {
        db->data->id_var = DBA_VAR(0, 7, 1);
        dba_db_data_set_value(db->data, dba_var_value(var));
        DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
    }
    if ((var = dba_record_key_peek(rec, DBA_KEY_HEIGHTBARO)) != NULL)
    {
        db->data->id_var = DBA_VAR(0, 7, 31);
        dba_db_data_set_value(db->data, dba_var_value(var));
        DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
    }

    return dba_error_ok();
}
#endif

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

int DB::obtain_context(const Query& rec)
{
    // Look if the record already knows the ID
    if (const char* val = rec.key_peek_value(DBA_KEY_CONTEXT_ID))
        return strtol(val, 0, 10);

    Context& c = context();

    /* Retrieve data */
    c.id_station = obtain_station(rec, false);

    /* Get the ID of the report */
    c.id_report = get_rep_cod(rec);

    /* Also input the seconds, defaulting to 0 if not found */
    const Var* year = rec.key_peek(DBA_KEY_YEAR);
    const Var* month = rec.key_peek(DBA_KEY_MONTH);
    const Var* day = rec.key_peek(DBA_KEY_DAY);
    const Var* hour = rec.key_peek(DBA_KEY_HOUR);
    const Var* min = rec.key_peek(DBA_KEY_MIN);
    const Var* sec = rec.key_peek(DBA_KEY_SEC);
    /* Datetime needs to be computed */
    if (year && month && day && hour && min)
    {
        c.date.year = year->enqi();
        c.date.month = month->enqi();
        c.date.day = day->enqi();
        c.date.hour = hour->enqi();
        c.date.minute = min->enqi();
        c.date.second = sec ? sec->enqi() : 0;
        c.date.fraction = 0;
    }
    else
        throw error_notfound("datetime informations not found among context information");

    c.ltype1 = rec.get(DBA_KEY_LEVELTYPE1, MISSING_INT);
    c.l1 = rec.get(DBA_KEY_L1, MISSING_INT);
    c.ltype2 = rec.get(DBA_KEY_LEVELTYPE2, MISSING_INT);
    c.l2 = rec.get(DBA_KEY_L2, MISSING_INT);
    c.pind = rec.get(DBA_KEY_PINDICATOR, MISSING_INT);
    c.p1 = rec.get(DBA_KEY_P1, MISSING_INT);
    c.p2 = rec.get(DBA_KEY_P2, MISSING_INT);

    // Check for an existing context with these data
    int id = c.get_id();

    /* If there is an existing record, use its ID and don't do an INSERT */
    if (id == -1)
        id = c.insert();

    return id;
}

void DB::insert(const Query& rec, bool can_replace, bool station_can_add)
{
    sql::DataV5& d = data();

    /* Check for the existance of non-context data, otherwise it's all
     * useless.  Not inserting data is fine in case of setcontextana */
    const char* s_year;
    if (rec.vars().empty() && !(((s_year = rec.key_peek_value(DBA_KEY_YEAR)) != NULL) && strcmp(s_year, "1000") == 0))
        throw error_notfound("no variables found in input record");

    auto t = conn->transaction();

    // Insert the station data, and get the ID
    int id_station = obtain_station(rec, station_can_add);

    // Insert the context data, and get the ID
    int id_context = obtain_context(rec);
    d.set_context_id(id_context);

    // Insert all the variables we find
    for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
    {
        /* Datum to be inserted, linked to id_station and all the other IDs */
        d.set(**i);
        if (can_replace)
            d.insert_or_overwrite();
        else
            d.insert_or_fail();
    }

    t->commit();

    last_context_id = id_context;
    _last_station_id = id_station;
}

int DB::last_station_id() const
{
    return _last_station_id;
}

void DB::remove(const Query& rec)
{
    ODBCConnection* c = dynamic_cast<ODBCConnection*>(conn);
    if (!c) throw error_unimplemented("v5 remove only works on ODBC connectors");

    auto t(c->transaction());
    db::v5::Cursor cur(*this);

    // Compile the DELETE query for the data
    auto stmd = c->odbcstatement("DELETE FROM data WHERE id_context=? AND id_var=?");
    stmd->bind_in(1, cur.out_context_id);
    stmd->bind_in(2, cur.out_varcode);

    // Compile the DELETE query for the attributes
    auto stma = c->odbcstatement("DELETE FROM attr WHERE id_context=? AND id_var=?");
    stma->bind_in(1, cur.out_context_id);
    stma->bind_in(2, cur.out_varcode);

    // Get the list of data to delete
    cur.query(rec,
            DBA_DB_WANT_CONTEXT_ID | DBA_DB_WANT_VAR_NAME,
            DBA_DB_MODIFIER_UNSORTED | DBA_DB_MODIFIER_STREAM);

    /* Iterate all the results, deleting them */
    while (cur.next())
    {
        stmd->execute_and_close();
        stma->execute_and_close();
    }
    t->commit();
}

void DB::remove_all()
{
    m_driver->remove_all_v5();
}

void DB::vacuum()
{
    m_driver->vacuum_v5();
}

#if 0
#ifdef DBA_USE_DELETE_USING
dba_err dba_db_remove(dba_db db, dba_record rec)
{
    const char* query =
        "DELETE FROM d, a"
        " USING station AS pa, context AS c, repinfo AS ri, data AS d"
        "  LEFT JOIN attr AS a ON a.id_context = d.id_context AND a.id_var = d.id_var"
        " WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
    dba_err err;
    SQLHSTMT stm;
    int res;
    int pseq = 1;

    assert(db);

    /* Allocate statement handle */
    DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

    /* Write the SQL query */

    /* Initial query */
    dba_querybuf_reset(db->querybuf);
    DBA_RUN_OR_GOTO(dba_delete_failed, dba_querybuf_append(db->querybuf, query));

    /* Bind select fields */
    DBA_RUN_OR_GOTO(dba_delete_failed, dba_db_prepare_select(db, rec, stm, &pseq));

    /*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

    /* Perform the query */
    res = SQLExecDirect(stm, (unsigned char*)dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
        goto dba_delete_failed;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stm);
    return dba_error_ok();

    /* Exit point with cleanup after error */
dba_delete_failed:
    SQLFreeHandle(SQL_HANDLE_STMT, stm);
    return err;
}
#else
dba_err dba_db_remove(dba_db db, dba_record rec)
{
    const char* query =
        "SELECT d.id FROM station AS pa, context AS c, data AS d, repinfo AS ri"
        " WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
    dba_err err = DBA_OK;
    SQLHSTMT stm;
    SQLHSTMT stm1;
    SQLHSTMT stm2;
    SQLINTEGER id;
    int res;

    assert(db);

    /* Allocate statement handles */
    DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

    res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm1);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stm);
        return dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "Allocating new statement handle");
    }
    res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm2);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        SQLFreeHandle(SQL_HANDLE_STMT, stm);
        SQLFreeHandle(SQL_HANDLE_STMT, stm1);
        return dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "Allocating new statement handle");
    }

    /* Write the SQL query */

    /* Initial query */
    dba_querybuf_reset(db->querybuf);
    DBA_RUN_OR_GOTO(cleanup, dba_querybuf_append(db->querybuf, query));

    /* Bind select fields */
    DBA_RUN_OR_GOTO(cleanup, dba_db_prepare_select(db, rec, stm));

    /* Bind output field */
    SQLBindCol(stm, 1, SQL_C_SLONG, &id, sizeof(id), NULL);

    /*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

    /* Perform the query */
    TRACE("Performing query %s\n", dba_querybuf_get(db->querybuf));
    res = SQLExecDirect(stm, dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
        goto cleanup;
    }

    /* Compile the DELETE query for the data */
    res = SQLPrepare(stm1, (unsigned char*)"DELETE FROM data WHERE id=?", SQL_NTS);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        err = dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "compiling query to delete data entries");
        goto cleanup;
    }
    /* Bind parameters */
    SQLBindParameter(stm1, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, 0);

    /* Compile the DELETE query for the associated QC */
    res = SQLPrepare(stm2, (unsigned char*)"DELETE FROM attr WHERE id_data=?", SQL_NTS);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        err = dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "compiling query to delete entries related to QC data");
        goto cleanup;
    }
    /* Bind parameters */
    SQLBindParameter(stm2, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, 0);

    /* Fetch the IDs and delete them */
    while (SQLFetch(stm) != SQL_NO_DATA)
    {
        /*fprintf(stderr, "Deleting %d\n", id);*/
        res = SQLExecute(stm1);
        if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
        {
            err = dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "deleting entry %d from the 'data' table", id);
            goto cleanup;
        }
        res = SQLExecute(stm2);
        if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
        {
            err = dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "deleting QC data related to 'data' entry %d", id);
            goto cleanup;
        }
    }

    /* Exit point with cleanup after error */
cleanup:
    SQLFreeHandle(SQL_HANDLE_STMT, stm);
    SQLFreeHandle(SQL_HANDLE_STMT, stm1);
    SQLFreeHandle(SQL_HANDLE_STMT, stm2);
    return err == DBA_OK ? dba_error_ok() : err;
}
#endif
#endif

std::unique_ptr<db::Cursor> DB::query(const Query& query, unsigned int wanted, unsigned int modifiers)
{
    unique_ptr<db::v5::Cursor> res(new db::v5::Cursor(*this));
    res->query(query, wanted, modifiers);
    return std::unique_ptr<db::Cursor>(res.release());
}

std::unique_ptr<db::Cursor> DB::query_stations(const Query& rec)
{
    /* Perform the query, limited to station values */
    return query(rec,
            DBA_DB_WANT_ANA_ID | DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT,
            DBA_DB_MODIFIER_ANAEXTRA | DBA_DB_MODIFIER_DISTINCT);
}

std::unique_ptr<db::Cursor> DB::query_data(const Query& rec)
{
    /* Perform the query */
    return query(rec,
                DBA_DB_WANT_ANA_ID | DBA_DB_WANT_CONTEXT_ID |
                DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT | DBA_DB_WANT_LEVEL |
                DBA_DB_WANT_TIMERANGE | DBA_DB_WANT_DATETIME |
                DBA_DB_WANT_VAR_NAME | DBA_DB_WANT_VAR_VALUE |
                DBA_DB_WANT_REPCOD,
                0);
}

std::unique_ptr<db::Cursor> DB::query_summary(const Query& rec)
{
#warning query_summary is not implemented for v5
    throw error_unimplemented("query_summary not implemented on v5 databases");
}

void DB::query_datetime_extremes(const Query& query, Record& result)
{
    db::v5::Cursor cursor(*this);
    cursor.query_datetime_extremes(query, result);
}

void DB::query_attrs(int reference_id, wreport::Varcode id_var,
        std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    ODBCConnection* c = dynamic_cast<ODBCConnection*>(conn);
    if (!c) throw error_unimplemented("v5 query_attrs only works with ODBC connectors");

    // Create the query
    Querybuf query(200);
    query.append(
            "SELECT type, value"
            "  FROM attr"
            " WHERE id_context = ? AND id_var = ?");

    // Perform the query
    Varcode out_type;
    char out_value[255];

    auto stm = c->odbcstatement(query);
    stm->bind_in(1, reference_id);
    stm->bind_in(2, id_var);
    stm->bind_out(1, out_type);
    stm->bind_out(2, out_value, 255);

    TRACE("attr read query: %s with ctx %d var %01d%02d%03d\n", query.c_str(), (int)in_id_context,
            WR_VAR_F(id_var), WR_VAR_X(id_var), WR_VAR_Y(id_var));

    stm->execute();

    // Fetch the results
    while (stm->fetch())
        dest(newvar(out_type, out_value));
}

void DB::attr_insert(wreport::Varcode id_var, const Record& attrs)
{
    attr_insert(last_context_id, id_var, attrs);
}

void DB::attr_insert(int reference_id, wreport::Varcode id_var, const Record& attrs)
{
    sql::AttrV5& a = attr();

    a.set_context(reference_id, id_var);

    // Begin the transaction
    auto t = conn->transaction();

    /* Insert all found variables */
    for (vector<Var*>::const_iterator i = attrs.vars().begin(); i != attrs.vars().end(); ++i)
    {
        a.set(**i);
        a.insert();
    }

    t->commit();
}

void DB::attr_remove(int reference_id, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs)
{
    ODBCConnection* c = dynamic_cast<ODBCConnection*>(conn);
    if (!c) throw error_unimplemented("v5 attr_remove only works with ODBC connectors");

    // Create the query
    Querybuf query(500);
    if (qcs.empty())
        query.append("DELETE FROM attr WHERE id_context = ? AND id_var = ?");
    else {
        query.append("DELETE FROM attr WHERE id_context = ? AND id_var = ? AND type IN (");
        query.start_list(", ");
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            query.append_listf("%hd", *i);
        query.append(")");
    }

    // dba_verbose(DBA_VERB_DB_SQL, "Performing query %s for id %d,B%02d%03d\n", query, id_context, DBA_VAR_X(id_var), DBA_VAR_Y(id_var));

    auto stm = c->odbcstatement(query);
    stm->bind_in(1, reference_id);
    stm->bind_in(2, id_var);
    stm->execute_and_close();
}

void DB::dump(FILE* out)
{
    repinfo().dump(out);
    station().dump(out);
    context().dump(out);
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

#if 0
    for (res = SQLFetch(pc.od_stm); res != SQL_NO_DATA; res = SQLFetch(pc.od_stm))
    {
        printf("Result: %d\n", i);
    }
#endif

} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
