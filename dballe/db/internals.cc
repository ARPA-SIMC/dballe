/*
 * db/internals - Internal support infrastructure for the DB
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

#include "internals.h"

#include <sql.h>
#include <sqlext.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include "dballe/core/vasprintf.h"

#include <iostream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

error_odbc::error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& msg)
{
    static const int strsize = 200;
    char stat[10], sqlmsg[strsize];
    SQLINTEGER err;
    SQLSMALLINT mlen;

    SQLGetDiagRec(handleType, handle, 1, (unsigned char*)stat, &err, (unsigned char*)sqlmsg, strsize, &mlen);
    if (mlen > strsize) mlen = strsize;

    this->msg = msg;
    this->msg += ": ";
    this->msg += sqlmsg;
}

void error_odbc::throwf(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...)
{
    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    char* cmsg;
    vasprintf(&cmsg, fmt, ap);
    va_end(ap);

    // Convert to string
    std::string msg(cmsg);
    free(cmsg);
    throw error_odbc(handleType, handle, msg);
}

Environment::Environment()
{
    // Allocate ODBC environment handle and register version 
    int res = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &od_env);
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_ENV, od_env, "Allocating main environment handle");

    res = SQLSetEnvAttr(od_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
    if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
        error_odbc e(SQL_HANDLE_ENV, od_env, "Asking for ODBC version 3");
        SQLFreeHandle(SQL_HANDLE_ENV, od_env);
        throw e;
    }
}

Environment::~Environment()
{
    SQLFreeHandle(SQL_HANDLE_ENV, od_env);
}

Environment& Environment::get()
{
    static Environment* env = NULL;
    if (!env) env = new Environment;
    return *env;
}

Connection::Connection()
    : connected(false), server_quirks(0), stm_last_insert_id(0)
{
    /* Allocate the ODBC connection handle */
    Environment& env = Environment::get();
    int sqlres = SQLAllocHandle(SQL_HANDLE_DBC, env.od_env, &od_conn);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_DBC, od_conn, "Allocating new connection handle");
}

Connection::~Connection()
{
    if (stm_last_insert_id) delete stm_last_insert_id;
    if (connected)
    {
        // FIXME: It was commit with no reason, setting it to rollback,
        // needs checking it doesn't cause trouble
        SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_ROLLBACK);
        SQLDisconnect(od_conn);
    }
    SQLFreeHandle(SQL_HANDLE_DBC, od_conn);
}

void Connection::connect(const char* dsn, const char* user, const char* password)
{
    /* Connect to the DSN */
    int sqlres = SQLConnect(od_conn,
                (SQLCHAR*)dsn, SQL_NTS,
                (SQLCHAR*)user, SQL_NTS,
                (SQLCHAR*)(password == NULL ? "" : password), SQL_NTS);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "Connecting to DSN %s as user %s", dsn, user);
    connected = true;
    init_after_connect();
}

void Connection::connect_file(const std::string& fname)
{
    // Access sqlite file directly
    string buf;
    if (fname[0] != '/')
    {
        char cwd[PATH_MAX];
        buf = "Driver=SQLite3;Database=";
        buf += getcwd(cwd, PATH_MAX);
        buf += "/";
        buf += fname;
        buf += ";";
    }
    else
    {
        buf = "Driver=SQLite3;Database=";
        buf += fname;
        buf += ";";
    }
    driver_connect(buf.c_str());
}

void Connection::driver_connect(const char* config)
{
    /* Connect to the DSN */
    char sdcout[1024];
    SQLSMALLINT outlen;
    int sqlres = SQLDriverConnect(od_conn, NULL,
                    (SQLCHAR*)config, SQL_NTS,
                    (SQLCHAR*)sdcout, 1024, &outlen,
                    SQL_DRIVER_NOPROMPT);

    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "Connecting to DB using configuration %s", config);
    connected = true;
    init_after_connect();
}

void Connection::init_after_connect()
{
    /* Find out what kind of database we are working with */
    string name = driver_name();

    if (name.substr(0, 9) == "libmyodbc" || name.substr(0, 6) == "myodbc")
        server_type = MYSQL;
    else if (name.substr(0, 6) == "sqlite")
    {
        string version = driver_version();
        server_type = SQLITE;
        if (version < "0.99")
            server_quirks = DBA_DB_QUIRK_NO_ROWCOUNT_IN_DIAG;
    }
    else if (name.substr(0, 5) == "SQORA")
        server_type = ORACLE;
    else if (name.substr(0, 11) == "libpsqlodbc" || name.substr(0, 8) == "psqlodbc")
        server_type = POSTGRES;
    else
    {
        fprintf(stderr, "ODBC driver %s is unsupported: assuming it's similar to Postgres", name.c_str());
        server_type = POSTGRES;
    }
}

std::string Connection::driver_name()
{
    char drivername[50];
    SQLSMALLINT len;
    int sqlres = SQLGetInfo(od_conn, SQL_DRIVER_NAME, (SQLPOINTER)drivername, 50, &len);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_DBC, od_conn, "Getting ODBC driver name");
    return string(drivername, len);
}

std::string Connection::driver_version()
{
    char driverver[50];
    SQLSMALLINT len;
    int sqlres = SQLGetInfo(od_conn, SQL_DRIVER_VER, (SQLPOINTER)driverver, 50, &len);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_DBC, od_conn, "Getting ODBC driver version");
    return string(driverver, len);
}

void Connection::set_autocommit(bool val)
{
    int sqlres = SQLSetConnectAttr(od_conn, SQL_ATTR_AUTOCOMMIT, (void*)(val ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF), 0);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "%s ODBC autocommit", val ? "Enabling" : "Disabling");
}

#ifdef DBA_USE_TRANSACTIONS
void Connection::commit()
{
    int sqlres = SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_COMMIT);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_DBC, od_conn, "Committing a transaction");
}

void Connection::rollback()
{
    int sqlres = SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_ROLLBACK);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_DBC, od_conn, "Rolling back a transaction");
}
#else
// TODO: lock and unlock tables instead
void Connection::commit() {}
void Connection::rollback() {}
#endif

#define DBA_ODBC_MISSING_TABLE_POSTGRES "42P01"
#define DBA_ODBC_MISSING_TABLE_MYSQL "42S01"
#define DBA_ODBC_MISSING_TABLE_SQLITE "HY000"
#define DBA_ODBC_MISSING_TABLE_ORACLE "42S02"

void Connection::drop_table_if_exists(const char* name)
{
    db::Statement stm(*this);
    char buf[100];
    int len;

    if (server_type == db::MYSQL)
    {
        len = snprintf(buf, 100, "DROP TABLE IF EXISTS %s", name);
        stm.exec_direct_and_close(buf, len);
    } else {
        switch (server_type)
        {
            case db::MYSQL: stm.ignore_error = DBA_ODBC_MISSING_TABLE_MYSQL; break;
            case db::SQLITE: stm.ignore_error = DBA_ODBC_MISSING_TABLE_SQLITE; break;
            case db::ORACLE: stm.ignore_error = DBA_ODBC_MISSING_TABLE_ORACLE; break;
            case db::POSTGRES: stm.ignore_error = DBA_ODBC_MISSING_TABLE_POSTGRES; break;
            default: stm.ignore_error = DBA_ODBC_MISSING_TABLE_POSTGRES; break;
        }

        len = snprintf(buf, 100, "DROP TABLE %s", name);
        stm.exec_direct_and_close(buf, len);
    }
    commit();
}

#define DBA_ODBC_MISSING_SEQUENCE_ORACLE "HY000"
#define DBA_ODBC_MISSING_SEQUENCE_POSTGRES "42P01"
void Connection::drop_sequence_if_exists(const char* name)
{
    const char* ignore_code;

    switch (server_type)
    {
        case db::ORACLE: ignore_code = DBA_ODBC_MISSING_SEQUENCE_ORACLE; break;
        case db::POSTGRES: ignore_code = DBA_ODBC_MISSING_SEQUENCE_POSTGRES; break;
        default:
            // No sequences in MySQL, SQLite or unknown databases
            return;
    }

    db::Statement stm(*this);
    stm.ignore_error = ignore_code;

    char buf[100];
    int len = snprintf(buf, 100, "DROP SEQUENCE %s", name);
    stm.exec_direct_and_close(buf, len);

    commit();
}

int Connection::get_last_insert_id()
{
    // Compile the query on demand
    if (!stm_last_insert_id)
    {
        switch (server_type)
        {
            case db::MYSQL:
                stm_last_insert_id = new db::Statement(*this);
                stm_last_insert_id->bind_out(1, m_last_insert_id);
                stm_last_insert_id->prepare("SELECT LAST_INSERT_ID()");
                break;
            case db::SQLITE:
                stm_last_insert_id = new db::Statement(*this);
                stm_last_insert_id->bind_out(1, m_last_insert_id);
                stm_last_insert_id->prepare("SELECT LAST_INSERT_ROWID()");
                break;
        }
    }
    if (!stm_last_insert_id)
        throw error_consistency("get_last_insert_id called on a database that does not support it");

    stm_last_insert_id->execute();
    if (!stm_last_insert_id->fetch_expecting_one())
        throw error_consistency("no last insert ID value returned from database");
    return m_last_insert_id;
}

bool Connection::has_table(const std::string& name)
{
    Statement stm(*this);
    DBALLE_SQL_C_SINT_TYPE count;

    switch (server_type)
    {
        case db::MYSQL:
            stm.prepare("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name=?");
            stm.bind_in(1, name.data(), name.size());
            stm.bind_out(1, count);
            break;
        case db::SQLITE:
            stm.prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?");
            stm.bind_in(1, name.data(), name.size());
            stm.bind_out(1, count);
            break;
        case db::ORACLE:
            stm.prepare("SELECT COUNT(*) FROM user_tables WHERE table_name=UPPER(?)");
            stm.bind_in(1, name.data(), name.size());
            stm.bind_out(1, count);
            break;
        case db::POSTGRES:
            stm.prepare("SELECT COUNT(*) FROM information_schema.tables WHERE table_name=?");
            stm.bind_in(1, name.data(), name.size());
            stm.bind_out(1, count);
            break;
    }
    stm.execute();
    stm.fetch_expecting_one();
    return count > 0;
}

std::string Connection::get_setting(const std::string& key)
{
    if (!has_table("dballe_settings"))
        return string();

    char result[64];
    SQLLEN result_len;

    Statement stm(*this);
    stm.prepare("SELECT value FROM dballe_settings WHERE `key`=?");
    stm.bind_in(1, key.data(), key.size());
    stm.bind_out(1, result, 64, result_len);
    stm.execute();
    string res;
    while (stm.fetch())
        res = string(result, result_len);
    return res;
}

void Connection::set_setting(const std::string& key, const std::string& value)
{
    Statement stm(*this);

    if (!has_table("dballe_settings"))
        stm.exec_direct("CREATE TABLE dballe_settings (`key` CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");

    // Remove if it exists
    stm.prepare("DELETE FROM dballe_settings WHERE `key`=?");
    stm.bind_in(1, key.data(), key.size());
    stm.execute_and_close();

    // Then insert it
    stm.prepare("INSERT INTO dballe_settings (`key`, value) VALUES (?, ?)");
    stm.bind_in(1, key.data(), key.size());
    stm.bind_in(2, value.data(), value.size());
    stm.execute_and_close();
}

void Connection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

Statement::Statement(Connection& conn)
    : conn(conn), stm(NULL), ignore_error(NULL)
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
      , debug_reached_completion(true)
#endif
{
    int sqlres = SQLAllocHandle(SQL_HANDLE_STMT, conn.od_conn, &stm);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
}

Statement::~Statement()
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    if (!debug_reached_completion)
    {
        string msg("Statement " + debug_query + " destroyed before reaching completion");
        fprintf(stderr, "-- %s\n", msg.c_str());
        //throw error_consistency(msg)
        SQLCloseCursor(stm);
    }
#endif
    SQLFreeHandle(SQL_HANDLE_STMT, stm);
}

bool Statement::error_is_ignored()
{
    if (!ignore_error) return false;

    // Retrieve the current error code
    char stat[10];
    SQLINTEGER err;
    SQLSMALLINT mlen;
    SQLGetDiagRec(SQL_HANDLE_STMT, stm, 1, (unsigned char*)stat, &err, NULL, 0, &mlen);

    // Ignore the given SQL error
    return memcmp(stat, ignore_error, 5) == 0;
}

bool Statement::is_error(int sqlres)
{
    return (sqlres != SQL_SUCCESS)
        && (sqlres != SQL_SUCCESS_WITH_INFO)
        && (sqlres != SQL_NO_DATA)
        && !error_is_ignored();
}

void Statement::bind_in(int idx, const DBALLE_SQL_C_SINT_TYPE& val)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, (DBALLE_SQL_C_SINT_TYPE*)&val, 0, 0);
}
void Statement::bind_in(int idx, const DBALLE_SQL_C_SINT_TYPE& val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, DBALLE_SQL_C_SINT, SQL_INTEGER, 0, 0, (DBALLE_SQL_C_SINT_TYPE*)&val, 0, (SQLLEN*)&ind);
}

void Statement::bind_in(int idx, const DBALLE_SQL_C_UINT_TYPE& val)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, DBALLE_SQL_C_UINT, SQL_INTEGER, 0, 0, (DBALLE_SQL_C_UINT_TYPE*)&val, 0, 0);
}
void Statement::bind_in(int idx, const DBALLE_SQL_C_UINT_TYPE& val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, DBALLE_SQL_C_UINT, SQL_INTEGER, 0, 0, (DBALLE_SQL_C_UINT_TYPE*)&val, 0, (SQLLEN*)&ind);
}

void Statement::bind_in(int idx, const unsigned short& val)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, (unsigned short*)&val, 0, 0);
}

void Statement::bind_in(int idx, const char* val)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)val, 0, 0);
}
void Statement::bind_in(int idx, const char* val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)val, 0, (SQLLEN*)&ind);
}

void Statement::bind_in(int idx, const SQL_TIMESTAMP_STRUCT& val)
{
    // cast away const because the ODBC API is not const-aware
    //if (conn.server_type == POSTGRES || conn.server_type == SQLITE)
        SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, (SQL_TIMESTAMP_STRUCT*)&val, 0, 0);
    //else
        //SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_DATETIME, 0, 0, (SQL_TIMESTAMP_STRUCT*)&val, 0, 0);
}


void Statement::bind_out(int idx, DBALLE_SQL_C_SINT_TYPE& val)
{
    SQLBindCol(stm, idx, DBALLE_SQL_C_SINT, &val, sizeof(val), 0);
}
void Statement::bind_out(int idx, DBALLE_SQL_C_SINT_TYPE& val, SQLLEN& ind)
{
    SQLBindCol(stm, idx, DBALLE_SQL_C_SINT, &val, sizeof(val), &ind);
}

void Statement::bind_out(int idx, DBALLE_SQL_C_UINT_TYPE& val)
{
    SQLBindCol(stm, idx, DBALLE_SQL_C_UINT, &val, sizeof(val), 0);
}
void Statement::bind_out(int idx, DBALLE_SQL_C_UINT_TYPE& val, SQLLEN& ind)
{
    SQLBindCol(stm, idx, DBALLE_SQL_C_UINT, &val, sizeof(val), &ind);
}

void Statement::bind_out(int idx, unsigned short& val)
{
    SQLBindCol(stm, idx, SQL_C_USHORT, &val, sizeof(val), 0);
}

void Statement::bind_out(int idx, char* val, SQLLEN buflen)
{
    SQLBindCol(stm, idx, SQL_C_CHAR, val, buflen, 0);
}
void Statement::bind_out(int idx, char* val, SQLLEN buflen, SQLLEN& ind)
{
    SQLBindCol(stm, idx, SQL_C_CHAR, val, buflen, &ind);
}

void Statement::bind_out(int idx, SQL_TIMESTAMP_STRUCT& val)
{
    SQLBindCol(stm, idx, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), 0);
}
void Statement::bind_out(int idx, SQL_TIMESTAMP_STRUCT& val, SQLLEN& ind)
{
    SQLBindCol(stm, idx, SQL_C_TYPE_TIMESTAMP, &val, sizeof(val), &ind);
}

bool Statement::fetch()
{
    int sqlres = SQLFetch(stm);
    if (sqlres == SQL_NO_DATA)
    {
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
        debug_reached_completion = true;
#endif
        return false;
    }
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "fetching data");
    return true;
}

bool Statement::fetch_expecting_one()
{
    if (!fetch()) 
    {
        close_cursor();
        return false;
    }
    if (fetch())
        throw error_consistency("expecting only one result from statement");
    close_cursor();
    return true;
}

size_t Statement::select_rowcount()
{
    if (conn.server_quirks & DBA_DB_QUIRK_NO_ROWCOUNT_IN_DIAG)
        return rowcount();

    SQLLEN res;
    int sqlres = SQLGetDiagField(SQL_HANDLE_STMT, stm, 0, SQL_DIAG_CURSOR_ROW_COUNT, &res, NULL, NULL);
    // SQLRowCount is broken in newer sqlite odbc
    //int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}

size_t Statement::rowcount()
{
    SQLLEN res;
    int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}

void Statement::set_cursor_forward_only()
{
    int sqlres = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_TYPE,
        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "setting SQL_CURSOR_FORWARD_ONLY");
}

void Statement::set_cursor_static()
{
    int sqlres = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_TYPE,
        (SQLPOINTER)SQL_CURSOR_STATIC, SQL_IS_INTEGER);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "setting SQL_CURSOR_STATIC");
}

void Statement::close_cursor()
{
    int sqlres = SQLCloseCursor(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing cursor");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

void Statement::prepare(const char* query)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    if (!debug_reached_completion)
    {
        string msg = "Statement " + debug_query + " prepare was called before reaching completion";
        fprintf(stderr, "-- %s\n", msg.c_str());
        //throw error_consistency(msg);
    }
#endif
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = query;
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    if (is_error(SQLPrepare(stm, (unsigned char*)query, SQL_NTS)))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "compiling query \"%s\"", query);
}

void Statement::prepare(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    if (is_error(SQLPrepare(stm, (unsigned char*)query, qlen)))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "compiling query \"%.*s\"", qlen, query);
}

int Statement::execute()
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    if (!debug_reached_completion)
    {
        string msg = "Statement " + debug_query + " restarted before reaching completion";
        fprintf(stderr, "-- %s\n", msg.c_str());
        //throw error_consistency(msg);
    }
#endif
    int sqlres = SQLExecute(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "executing precompiled query");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

int Statement::exec_direct(const char* query)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = query;
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, SQL_NTS);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%s\"", query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

int Statement::exec_direct(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, qlen);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%.*s\"", qlen, query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    return sqlres;
}

void Statement::close_cursor_if_needed()
{
    /*
    // If the query raised an error that we are ignoring, closing the cursor
    // would raise invalid cursor state
    if (sqlres != SQL_ERROR)
        close_cursor();
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    else if (sqlres == SQL_ERROR && error_is_ignored())
        debug_reached_completion = true;
#endif
*/
    SQLCloseCursor(stm);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

int Statement::execute_and_close()
{
    int sqlres = SQLExecute(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "executing precompiled query");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int Statement::exec_direct_and_close(const char* query)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = query;
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, SQL_NTS);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%s\"", query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int Statement::exec_direct_and_close(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    int sqlres = SQLExecDirect(stm, (SQLCHAR*)query, qlen);
    if (is_error(sqlres))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%.*s\"", qlen, query);
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = false;
#endif
    close_cursor_if_needed();
    return sqlres;
}

int Statement::columns_count()
{
    SQLSMALLINT res;
    int sqlres = SQLNumResultCols(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "querying number of columns in the result set");
    return res;
}

Sequence::Sequence(Connection& conn, const char* name)
    : Statement(conn)
{
    char qbuf[100];
    int qlen;

    bind_out(1, out);
    if (conn.server_type == ORACLE)
        qlen = snprintf(qbuf, 100, "SELECT %s.CurrVal FROM dual", name);    
    else
        qlen = snprintf(qbuf, 100, "SELECT last_value FROM %s", name);  
    prepare(qbuf, qlen);
}

Sequence::~Sequence() {}

const DBALLE_SQL_C_SINT_TYPE& Sequence::read()
{
    if (is_error(SQLExecute(stm)))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading sequence value");
    /* Get the result */
    if (SQLFetch(stm) == SQL_NO_DATA)
        throw error_notfound("fetching results of sequence value reads");
    if (is_error(SQLCloseCursor(stm)))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing sequence read cursor");
    return out;
}

const char* default_repinfo_file()
{
    const char* repinfo_file = getenv("DBA_REPINFO");
    if (repinfo_file == 0 || repinfo_file[0] == 0)
        repinfo_file = TABLE_DIR "/repinfo.csv";
    return repinfo_file;
}

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
