/*
 * db/sqlite/internals - Implementation infrastructure for the SQLite DB connection
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#if 0
#include <cstring>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include "dballe/core/vasprintf.h"
#include "dballe/core/verbose.h"
#include "dballe/db/querybuf.h"

#include <iostream>
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

namespace {

}


error_sqlite::error_sqlite(sqlite3* db, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += sqlite3_errmsg(db);
}

error_sqlite::error_sqlite(const std::string& dbmsg, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += dbmsg;
}

void error_sqlite::throwf(sqlite3* db, const char* fmt, ...)
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
    throw error_sqlite(db, msg);
}

SQLiteConnection::SQLiteConnection()
{
}

SQLiteConnection::~SQLiteConnection()
{
    if (db) sqlite3_close(db);
}

void SQLiteConnection::open_file(const std::string& pathname, int flags)
{
    int res = sqlite3_open_v2(pathname.c_str(), &db, flags, nullptr);
    if (res != SQLITE_OK)
    {
        // From http://www.sqlite.org/c3ref/open.html
        // Whether or not an error occurs when it is opened, resources
        // associated with the database connection handle should be
        // released by passing it to sqlite3_close() when it is no longer
        // required.
        std::string errmsg(sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        throw error_sqlite(errmsg, "opening " + pathname);
    }
    init_after_connect();
}

void SQLiteConnection::open_memory(int flags)
{
    open_file(":memory:", flags);
}

void SQLiteConnection::open_private_file(int flags)
{
    open_file("", flags);
}

void SQLiteConnection::init_after_connect()
{
    server_type = ServerType::SQLITE;
    // autocommit is off by default when inside a transaction
    // set_autocommit(false);
}

void SQLiteConnection::wrap_sqlite3_exec(const std::string& query)
{
    char* errmsg;
    int res = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK && errmsg)
    {
        // http://www.sqlite.org/c3ref/exec.html
        //
        // If the 5th parameter to sqlite3_exec() is not NULL then any error
        // message is written into memory obtained from sqlite3_malloc() and
        // passed back through the 5th parameter. To avoid memory leaks, the
        // application should invoke sqlite3_free() on error message strings
        // returned through the 5th parameter of of sqlite3_exec() after the
        // error message string is no longer needed.·
        std::string msg(errmsg);
        sqlite3_free(errmsg);
        throw error_sqlite(errmsg, "executing " + query);
    }
}

void SQLiteConnection::wrap_sqlite3_exec_nothrow(const std::string& query) noexcept
{
    char* errmsg;
    int res = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK && errmsg)
    {
        // http://www.sqlite.org/c3ref/exec.html
        //
        // If the 5th parameter to sqlite3_exec() is not NULL then any error
        // message is written into memory obtained from sqlite3_malloc() and
        // passed back through the 5th parameter. To avoid memory leaks, the
        // application should invoke sqlite3_free() on error message strings
        // returned through the 5th parameter of of sqlite3_exec() after the
        // error message string is no longer needed.·
        fprintf(stderr, "cannot execute '%s': %s\n", query.c_str(), errmsg);
        sqlite3_free(errmsg);
    }
}

struct SQLiteTransaction : public Transaction
{
    SQLiteConnection& conn;
    bool fired = false;

    SQLiteTransaction(SQLiteConnection& conn) : conn(conn)
    {
    }
    ~SQLiteTransaction() { if (!fired) rollback_nothrow(); }

    void commit() override
    {
        conn.wrap_sqlite3_exec("COMMIT");
        fired = true;
    }
    void rollback() override
    {
        conn.wrap_sqlite3_exec("ROLLBACK");
        fired = true;
    }
    void rollback_nothrow()
    {
        conn.wrap_sqlite3_exec_nothrow("ROLLBACK");
        fired = true;
    }
};

std::unique_ptr<Transaction> SQLiteConnection::transaction()
{
    wrap_sqlite3_exec("BEGIN");
    return unique_ptr<Transaction>(new SQLiteTransaction(*this));
}

std::unique_ptr<Statement> SQLiteConnection::statement()
{
#if 0
    return unique_ptr<Statement>(new SQLiteStatement(*this));
#endif
}

std::unique_ptr<SQLiteStatement> SQLiteConnection::sqlitestatement()
{
#if 0
    return unique_ptr<SQLiteStatement>(new SQLiteStatement(*this));
#endif
}

void SQLiteConnection::impl_exec_noargs(const std::string& query)
{
    wrap_sqlite3_exec(query);
}

#define DBA_ODBC_MISSING_TABLE_SQLITE "HY000"

void SQLiteConnection::drop_table_if_exists(const char* name)
{
#if 0
    switch (server_type)
    {
        case ServerType::MYSQL:
        case ServerType::POSTGRES:
        case ServerType::SQLITE:
            exec(string("DROP TABLE IF EXISTS ") + name);
            break;
        case ServerType::ORACLE:
        {
            auto stm = odbcstatement();
            char buf[100];
            int len;
            stm->ignore_error = DBA_ODBC_MISSING_TABLE_ORACLE;
            len = snprintf(buf, 100, "DROP TABLE %s", name);
            stm->exec_direct_and_close(buf, len);
            break;
        }
    }
#endif
}

void SQLiteConnection::drop_sequence_if_exists(const char* name)
{
#if 0
    switch (server_type)
    {
        case ServerType::POSTGRES:
            exec(string("DROP SEQUENCE IF EXISTS ") + name);
            break;
        case ServerType::ORACLE:
        {
            auto stm = odbcstatement();
            char buf[100];
            int len;
            stm->ignore_error = DBA_ODBC_MISSING_SEQUENCE_ORACLE;
            len = snprintf(buf, 100, "DROP SEQUENCE %s", name);
            stm->exec_direct_and_close(buf, len);
            break;
        }
        default:
            break;
    }
#endif
}

int SQLiteConnection::get_last_insert_id()
{
#if 0
    // Compile the query on demand
    if (!stm_last_insert_id)
    {
        switch (server_type)
        {
            case ServerType::MYSQL:
                stm_last_insert_id = odbcstatement().release();
                stm_last_insert_id->bind_out(1, m_last_insert_id);
                stm_last_insert_id->prepare("SELECT LAST_INSERT_ID()");
                break;
            case ServerType::SQLITE:
                stm_last_insert_id = odbcstatement().release();
                stm_last_insert_id->bind_out(1, m_last_insert_id);
                stm_last_insert_id->prepare("SELECT LAST_INSERT_ROWID()");
                break;
            case ServerType::POSTGRES:
                stm_last_insert_id = odbcstatement().release();
                stm_last_insert_id->bind_out(1, m_last_insert_id);
                stm_last_insert_id->prepare("SELECT LASTVAL()");
                break;
            default:
                throw error_consistency("get_last_insert_id called on a database that does not support it");
        }
    }

    stm_last_insert_id->execute();
    if (!stm_last_insert_id->fetch_expecting_one())
        throw error_consistency("no last insert ID value returned from database");
    return m_last_insert_id;
#endif
}

bool SQLiteConnection::has_table(const std::string& name)
{
#if 0
    auto stm = odbcstatement();
    int count;

    switch (server_type)
    {
        case ServerType::MYSQL:
            stm->prepare("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name=?");
            stm->bind_in(1, name.data(), name.size());
            stm->bind_out(1, count);
            break;
        case ServerType::SQLITE:
            stm->prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?");
            stm->bind_in(1, name.data(), name.size());
            stm->bind_out(1, count);
            break;
        case ServerType::ORACLE:
            stm->prepare("SELECT COUNT(*) FROM user_tables WHERE table_name=UPPER(?)");
            stm->bind_in(1, name.data(), name.size());
            stm->bind_out(1, count);
            break;
        case ServerType::POSTGRES:
            stm->prepare("SELECT COUNT(*) FROM information_schema.tables WHERE table_name=?");
            stm->bind_in(1, name.data(), name.size());
            stm->bind_out(1, count);
            break;
    }
    stm->execute();
    stm->fetch_expecting_one();
    return count > 0;
#endif
}

std::string SQLiteConnection::get_setting(const std::string& key)
{
#if 0
    if (!has_table("dballe_settings"))
        return string();

    char result[64];
    SQLLEN result_len;

    auto stm = odbcstatement();
    if (server_type == ServerType::MYSQL)
        stm->prepare("SELECT value FROM dballe_settings WHERE `key`=?");
    else
        stm->prepare("SELECT value FROM dballe_settings WHERE \"key\"=?");
    stm->bind_in(1, key.data(), key.size());
    stm->bind_out(1, result, 64, result_len);
    stm->execute();
    string res;
    while (stm->fetch())
        res = string(result, result_len);
    // rtrim string
    size_t n = res.substr(0, 63).find_last_not_of(' ');
    if (n != string::npos)
        res.erase(n+1);
    return res;
#endif
}

void SQLiteConnection::set_setting(const std::string& key, const std::string& value)
{
#if 0
    auto stm = odbcstatement();

    if (!has_table("dballe_settings"))
    {
        if (server_type == ServerType::MYSQL)
            exec("CREATE TABLE dballe_settings (`key` CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");
        else
            exec("CREATE TABLE dballe_settings (\"key\" CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");
    }

    // Remove if it exists
    if (server_type == ServerType::MYSQL)
        stm->prepare("DELETE FROM dballe_settings WHERE `key`=?");
    else
        stm->prepare("DELETE FROM dballe_settings WHERE \"key\"=?");
    stm->bind_in(1, key.data(), key.size());
    stm->execute_and_close();

    // Then insert it
    if (server_type == ServerType::MYSQL)
        stm->prepare("INSERT INTO dballe_settings (`key`, value) VALUES (?, ?)");
    else
        stm->prepare("INSERT INTO dballe_settings (\"key\", value) VALUES (?, ?)");
    SQLLEN key_size = key.size();
    SQLLEN value_size = value.size();
    stm->bind_in(1, key.data(), key_size);
    stm->bind_in(2, value.data(), value_size);
    stm->execute_and_close();
#endif
}

void SQLiteConnection::drop_settings()
{
#if 0
    drop_table_if_exists("dballe_settings");
#endif
}

void SQLiteConnection::add_datetime(Querybuf& qb, const int* dt) const
{
#if 0
    qb.appendf("{ts '%04d-%02d-%02d %02d:%02d:%02d'}", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
#endif
}


SQLiteStatement::SQLiteStatement(SQLiteConnection& conn)
    : conn(conn)
{
#if 0
    int sqlres = SQLAllocHandle(SQL_HANDLE_STMT, conn.od_conn, &stm);
    if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
        throw error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
#endif
}

SQLiteStatement::~SQLiteStatement()
{
#if 0
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
#endif
}

#if 0
bool ODBCStatement::error_is_ignored()
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

bool ODBCStatement::is_error(int sqlres)
{
    return (sqlres != SQL_SUCCESS)
        && (sqlres != SQL_SUCCESS_WITH_INFO)
        && (sqlres != SQL_NO_DATA)
        && !error_is_ignored();
}
#endif

void SQLiteStatement::bind_in(int idx, const int& val)
{
#if 0
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<true, sizeof(const int)>(), SQL_INTEGER, 0, 0, (int*)&val, 0, 0);
#endif
}
#if 0
void SQLiteStatement::bind_in(int idx, const int& val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<true, sizeof(const int)>(), SQL_INTEGER, 0, 0, (int*)&val, 0, (SQLLEN*)&ind);
}
#endif

void SQLiteStatement::bind_in(int idx, const unsigned& val)
{
#if 0
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<false, sizeof(const unsigned)>(), SQL_INTEGER, 0, 0, (unsigned*)&val, 0, 0);
#endif
}
#if 0
void SQLiteStatement::bind_in(int idx, const unsigned& val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<false, sizeof(const unsigned)>(), SQL_INTEGER, 0, 0, (unsigned*)&val, 0, (SQLLEN*)&ind);
}
#endif

void SQLiteStatement::bind_in(int idx, const unsigned short& val)
{
#if 0
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<false, sizeof(const unsigned short)>(), SQL_INTEGER, 0, 0, (unsigned short*)&val, 0, 0);
#endif
}
#if 0
void SQLiteStatement::bind_in(int idx, const unsigned short& val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, get_odbc_integer_type<false, sizeof(const unsigned short)>(), SQL_INTEGER, 0, 0, (unsigned short*)&val, 0, (SQLLEN*)&ind);
}
#endif

void SQLiteStatement::bind_in(int idx, const char* val)
{
#if 0
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)val, 0, 0);
#endif
}
#if 0
void SQLiteStatement::bind_in(int idx, const char* val, const SQLLEN& ind)
{
    // cast away const because the ODBC API is not const-aware
    SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)val, 0, (SQLLEN*)&ind);
}
#endif

#if 0
void SQLiteStatement::bind_in(int idx, const SQL_TIMESTAMP_STRUCT& val)
{
    // cast away const because the ODBC API is not const-aware
    //if (conn.server_type == POSTGRES || conn.server_type == SQLITE)
        SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, (SQL_TIMESTAMP_STRUCT*)&val, 0, 0);
    //else
        //SQLBindParameter(stm, idx, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_DATETIME, 0, 0, (SQL_TIMESTAMP_STRUCT*)&val, 0, 0);
}
#endif

#if 0
bool ODBCStatement::fetch()
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

bool ODBCStatement::fetch_expecting_one()
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

size_t ODBCStatement::select_rowcount()
{
    if (conn.server_quirks & DBA_DB_QUIRK_NO_ROWCOUNT_IN_DIAG)
        return rowcount();

    SQLLEN res;
    int sqlres = SQLGetDiagField(SQL_HANDLE_STMT, stm, 0, SQL_DIAG_CURSOR_ROW_COUNT, &res, 0, NULL);
    // SQLRowCount is broken in newer sqlite odbc
    //int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}

size_t ODBCStatement::rowcount()
{
    SQLLEN res;
    int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}
#endif

void SQLiteStatement::set_cursor_forward_only()
{
#if 0
    int sqlres = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_TYPE,
        (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY, SQL_IS_INTEGER);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "setting SQL_CURSOR_FORWARD_ONLY");
#endif
}

#if 0
void ODBCStatement::set_cursor_static()
{
    int sqlres = SQLSetStmtAttr(stm, SQL_ATTR_CURSOR_TYPE,
        (SQLPOINTER)SQL_CURSOR_STATIC, SQL_IS_INTEGER);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "setting SQL_CURSOR_STATIC");
}

void ODBCStatement::close_cursor()
{
    int sqlres = SQLCloseCursor(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing cursor");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

void SQLiteStatement::prepare(const char* query)
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

void ODBCStatement::prepare(const char* query, int qlen)
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_query = string(query, qlen);
#endif
    // Casting out 'const' because ODBC API is not const-conscious
    if (is_error(SQLPrepare(stm, (unsigned char*)query, qlen)))
        error_odbc::throwf(SQL_HANDLE_STMT, stm, "compiling query \"%.*s\"", qlen, query);
}
#endif

void SQLiteStatement::prepare(const std::string& query)
{
#if 0
    prepare(query.data(), query.size());
#endif
}

#if 0
int ODBCStatement::execute()
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

int ODBCStatement::exec_direct(const char* query)
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

int ODBCStatement::exec_direct(const char* query, int qlen)
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

void ODBCStatement::close_cursor_if_needed()
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
#endif

void SQLiteStatement::execute_ignoring_results()
{
#if 0
    execute_and_close();
#endif
}

#if 0
int ODBCStatement::execute_and_close()
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

int ODBCStatement::exec_direct_and_close(const char* query)
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

int ODBCStatement::exec_direct_and_close(const char* query, int qlen)
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

int ODBCStatement::columns_count()
{
    SQLSMALLINT res;
    int sqlres = SQLNumResultCols(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "querying number of columns in the result set");
    return res;
}

Sequence::Sequence(ODBCConnection& conn, const char* name)
    : ODBCStatement(conn)
{
    char qbuf[100];
    int qlen;

    bind_out(1, out);
    if (conn.server_type == ServerType::ORACLE)
        qlen = snprintf(qbuf, 100, "SELECT %s.CurrVal FROM dual", name);
    else
        qlen = snprintf(qbuf, 100, "SELECT last_value FROM %s", name);
    prepare(qbuf, qlen);
}

Sequence::~Sequence() {}

const int& Sequence::read()
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

std::ostream& operator<<(std::ostream& o, const SQL_TIMESTAMP_STRUCT& t)
{
    char buf[20];
    snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d.%d", t.year, t.month, t.day, t.hour, t.minute, t.second, t.fraction);
    o << buf;
    return o;
}
#endif

}
}
