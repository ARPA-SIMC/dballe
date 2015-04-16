/*
 * db/mysql/internals - Implementation infrastructure for the MySQL DB connection
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/core/vasprintf.h"
#include "dballe/db/querybuf.h"
#include <regex>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

error_mysql::error_mysql(MYSQL* db, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += mysql_error(db);
}

error_mysql::error_mysql(const std::string& dbmsg, const std::string& msg)
{
    this->msg = msg;
    this->msg += ":";
    this->msg += dbmsg;
}

void error_mysql::throwf(MYSQL* db, const char* fmt, ...)
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
    throw error_mysql(db, msg);
}

namespace mysql {

void ConnectInfo::reset()
{
    host.clear();
    user.clear();
    has_passwd = false;
    passwd.clear();
    has_dbname = false;
    dbname.clear();
    port = 0;
    unix_socket.clear();
}

void ConnectInfo::parse_url(const std::string& url)
{
    // mysql://[host][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...
    reset();

    if (url == "mysql:" || url == "mysql://" || url == "mysql:///")
        return;

    regex re_remote(R"(^mysql://([^:/]+)?(:\d+)?/([^?]+)?(\?.+)?$)", regex_constants::ECMAScript);
    smatch res;
    if (!regex_match(url, res, re_remote))
        throw error_consistency("cannot parse MySQL connect URL '" + url + "'");

    // Host
    if (res[1].matched)
        host = res[1].str();
    // Port
    if (res[2].matched)
        port = stoul(res[2].str().substr(1));
    // DB name
    if (res[3].matched)
    {
        has_dbname = true;
        dbname = res[3].str();
    }
    // Query string
    if (res[4].matched)
    {
        // ?arg[=val]&arg[=val]...
        regex re_qstring(R"([?&]([^=]+)(=[^&]+)?)", regex_constants::ECMAScript);
        string qstring = res[4].str();
        for (auto i = sregex_iterator(qstring.begin(), qstring.end(), re_qstring); i != sregex_iterator(); ++i)
        {
            smatch match = *i;
            if (match[1].str() == "user")
                user = match[2].str().substr(1);
            else if (match[1].str() == "password")
            {
                has_passwd = true;
                passwd = match[2].str().substr(1);
            }
        }
    }
}

std::string ConnectInfo::to_url() const
{
    std::string res = "mysql://";
    if (!user.empty() || !passwd.empty())
    {
        res += user;
        if (has_passwd)
            res += ":" + passwd;
        res += "@";
    }
    res += host;
    if (port != 0)
        res += ":" + to_string(port);
    res += "/" + dbname;

    // TODO: currently unsupported unix_socket;
    return res;
}

Datetime Row::as_datetime(int col) const
{
    Datetime res;
    sscanf(as_cstring(col), "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
            &res.date.year,
            &res.date.month,
            &res.date.day,
            &res.time.hour,
            &res.time.minute,
            &res.time.second);
    return res;
}


Row Result::expect_one_result()
{
    if (mysql_num_rows(res) != 1)
        error_consistency::throwf("query returned %u rows instead of 1", (unsigned)mysql_num_rows(res));
    return Row(res, mysql_fetch_row(res));
}

}

MySQLConnection::MySQLConnection()
{
    db = mysql_init(nullptr);
    // mysql_error does seem to work with nullptr as its argument. I only saw
    // it return empty strings, though, because I have not been able to find a
    // way to make mysql_int fail for real
    if (!db) throw error_mysql(nullptr, "failed to create a MYSQL object");
}

MySQLConnection::~MySQLConnection()
{
    if (db) mysql_close(db);
}

void MySQLConnection::open(const mysql::ConnectInfo& info)
{
    // See http://www.enricozini.org/2012/tips/sa-sqlmode-traditional/
    mysql_options(db, MYSQL_INIT_COMMAND, "SET sql_mode='traditional'");
    // TODO: benchmark with and without compression
    //mysql_options(db, MYSQL_OPT_COMPRESS, 0);
    // Auto-reconnect transparently messes up all assumptions, so we switch it
    // off: see https://dev.mysql.com/doc/refman/5.0/en/auto-reconnect.html
    my_bool reconnect = 0;
    mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect);
    if (!mysql_real_connect(db,
                info.host.empty() ? nullptr : info.host.c_str(),
                info.user.c_str(),
                info.has_passwd ? info.passwd.c_str() : nullptr,
                info.dbname.c_str(),
                info.port,
                info.unix_socket.empty() ? nullptr : info.unix_socket.c_str(),
                CLIENT_REMEMBER_OPTIONS))
        throw error_mysql(db, "cannot open MySQL connection to " + info.to_url());
    init_after_connect();
}

void MySQLConnection::open_url(const std::string& url)
{
    using namespace mysql;
    ConnectInfo info;
    info.parse_url(url);
    open(info);
}

void MySQLConnection::open_test()
{
    const char* envurl = getenv("DBA_DB_MYSQL");
    if (envurl == NULL)
        throw error_consistency("DBA_DB_MYSQL not defined");
    return open_url(envurl);
}

void MySQLConnection::init_after_connect()
{
    server_type = ServerType::MYSQL;
    // autocommit is off by default when inside a transaction
    // set_autocommit(false);
}

std::string MySQLConnection::escape(const char* str)
{
    // Dirty: we write directly inside the resulting std::string storage.
    // It should work in C++11, although not according to its specifications,
    // and if for some reason we discover that it does not work, this can be
    // rewritten with one extra string copy.
    size_t str_len = strlen(str);
    string res(str_len * 2 + 1, 0);
    unsigned long len = mysql_real_escape_string(db, const_cast<char*>(res.data()), str, str_len);
    res.resize(len);
    return res;
}

std::string MySQLConnection::escape(const std::string& str)
{
    // Dirty: we write directly inside the resulting std::string storage.
    // It should work in C++11, although not according to its specifications,
    // and if for some reason we discover that it does not work, this can be
    // rewritten with one extra string copy.
    string res(str.size() * 2 + 1, 0);
    unsigned long len = mysql_real_escape_string(db, const_cast<char*>(res.data()), str.data(), str.size());
    res.resize(len);
    return res;
}

void MySQLConnection::exec_no_data(const char* query)
{
    using namespace mysql;

    if (mysql_query(db, query))
        error_mysql::throwf(db, "cannot execute '%s'", query);

    Result res(mysql_store_result(db));
    if (res)
        error_consistency::throwf("query '%s' returned %u rows instead of zero",
                query, (unsigned)mysql_num_rows(res));
    else if (mysql_errno(db))
        error_mysql::throwf(db, "cannot store result of query '%s'", query);
}

void MySQLConnection::exec_no_data(const std::string& query)
{
    using namespace mysql;

    if (mysql_real_query(db, query.data(), query.size()))
        error_mysql::throwf(db, "cannot execute '%s'", query.c_str());

    Result res(mysql_store_result(db));
    if (res)
        error_consistency::throwf("query '%s' returned %u rows instead of zero",
                query.c_str(), (unsigned)mysql_num_rows(res));
    else if (mysql_errno(db))
        error_mysql::throwf(db, "cannot store result of query '%s'", query.c_str());
}

mysql::Result MySQLConnection::exec_store(const char* query)
{
    using namespace mysql;

    if (mysql_query(db, query))
        error_mysql::throwf(db, "cannot execute '%s'", query);
    Result res(mysql_store_result(db));
    if (res) return res;

    if (mysql_errno(db))
        error_mysql::throwf(db, "cannot store result of query '%s'", query);
    else
        error_consistency::throwf("query '%s' returned no data", query);
}

mysql::Result MySQLConnection::exec_store(const std::string& query)
{
    using namespace mysql;

    if (mysql_real_query(db, query.data(), query.size()))
        error_mysql::throwf(db, "cannot execute '%s'", query.c_str());
    Result res(mysql_store_result(db));
    if (res) return res;

    if (mysql_errno(db))
        error_mysql::throwf(db, "cannot store result of query '%s'", query.c_str());
    else
        error_consistency::throwf("query '%s' returned no data", query.c_str());
}

void MySQLConnection::exec_use(const char* query, std::function<void(const mysql::Row&)> dest)
{
    using namespace mysql;

    if (mysql_query(db, query))
        error_mysql::throwf(db, "cannot execute '%s'", query);
    Result res(mysql_use_result(db));
    if (!res)
    {
        if (mysql_errno(db))
            error_mysql::throwf(db, "cannot store result of query '%s'", query);
        else
            error_consistency::throwf("query '%s' returned no data", query);
    }
    send_result(move(res), dest);
}

void MySQLConnection::exec_use(const std::string& query, std::function<void(const mysql::Row&)> dest)
{
    using namespace mysql;

    if (mysql_real_query(db, query.data(), query.size()))
        error_mysql::throwf(db, "cannot execute '%s'", query.c_str());
    Result res(mysql_use_result(db));
    if (!res)
    {
        if (mysql_errno(db))
            error_mysql::throwf(db, "cannot store result of query '%s'", query.c_str());
        else
            error_consistency::throwf("query '%s' returned no data", query.c_str());
    }
    send_result(move(res), dest);
}

void MySQLConnection::send_result(mysql::Result&& res, std::function<void(const mysql::Row&)> dest)
{
    using namespace mysql;

    while (Row row = res.fetch())
    {
        try {
            dest(row);
        } catch (...) {
            // If dest throws an exception, we still need to flush the inbound
            // rows before rethrowing it: this is not done automatically when
            // closing the result, and not doing it will break the next
            // queries.
            //
            // fetch() will not throw exceptions (see its documentation)
            // because mysql_fetch_row has no usable error reporting.
            // If there is a network connectivity problem, we will never know it.
            //
            // See: https://dev.mysql.com/doc/refman/5.0/en/mysql-use-result.html
            //    «When using mysql_use_result(), you must execute
            //    mysql_fetch_row() until a NULL value is returned, otherwise,
            //    the unfetched rows are returned as part of the result set for
            //    your next query. The C API gives the error 'Commands out of
            //    sync; you can't run this command now' if you forget to do
            //    this!»
            while (res.fetch()) ;
            throw;
        }
    }
}

#if 0
void MySQLConnection::wrap_sqlite3_exec(const std::string& query)
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

void MySQLConnection::wrap_sqlite3_exec_nothrow(const std::string& query) noexcept
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
#endif

#if 0
struct MySQLTransaction : public Transaction
{
    MySQLConnection& conn;
    bool fired = false;

    MySQLTransaction(MySQLConnection& conn) : conn(conn)
    {
    }
    ~MySQLTransaction() { if (!fired) rollback_nothrow(); }

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
#endif

std::unique_ptr<Transaction> MySQLConnection::transaction()
{
    throw error_unimplemented("mysql transaction");
#if 0
    wrap_sqlite3_exec("BEGIN");
    return unique_ptr<Transaction>(new MySQLTransaction(*this));
#endif
}

#if 0
std::unique_ptr<MySQLStatement> MySQLConnection::sqlitestatement(const std::string& query)
{
    return unique_ptr<MySQLStatement>(new MySQLStatement(*this, query));
}
#endif

void MySQLConnection::drop_table_if_exists(const char* name)
{
    exec_no_data(string("DROP TABLE IF EXISTS ") + name);
}

int MySQLConnection::get_last_insert_id()
{
    return mysql_insert_id(db);
}

bool MySQLConnection::has_table(const std::string& name)
{
    using namespace mysql;
    string query = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='" + name + "'";
    Result res(exec_store(query));
    Row row = res.expect_one_result();
    return row.as_unsigned(0) > 0;
}

std::string MySQLConnection::get_setting(const std::string& key)
{
    using namespace mysql;
    if (!has_table("dballe_settings"))
        return string();

    string query = "SELECT value FROM dballe_settings WHERE `key`='";
    query += escape(key);
    query += '\'';

    Result res(exec_store(query));
    if (res.rowcount() == 0)
        return string();
    if (res.rowcount() > 1)
        error_consistency::throwf("got %d results instead of 1 executing %s", res.rowcount(), query.c_str());
    Row row = res.fetch();
    return row.as_string(0);
}

void MySQLConnection::set_setting(const std::string& key, const std::string& value)
{
    if (!has_table("dballe_settings"))
        exec_no_data("CREATE TABLE dballe_settings (`key` CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");

    string query = "INSERT INTO dballe_settings (`key`, value) VALUES ('";
    query += escape(key);
    query += "', '";
    query += escape(value);
    query += "') ON DUPLICATE KEY UPDATE value=VALUES(value)";
    exec_no_data(query);
}

void MySQLConnection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

void MySQLConnection::add_datetime(Querybuf& qb, const int* dt) const
{
    qb.appendf("'%04d-%02d-%02d %02d:%02d:%02d'", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
}

#if 0
int MySQLConnection::changes()
{
    throw error_unimplemented("mysql changes");
    //return sqlite3_changes(db);
}
#endif

#if 0
MySQLStatement::MySQLStatement(MySQLConnection& conn, const std::string& query)
    : conn(conn)
{
    // From http://www.sqlite.org/c3ref/prepare.html:
    // If the caller knows that the supplied string is nul-terminated, then
    // there is a small performance advantage to be gained by passing an nByte
    // parameter that is equal to the number of bytes in the input string
    // including the nul-terminator bytes as this saves MySQL from having to
    // make a copy of the input string.
    int res = sqlite3_prepare_v2(conn, query.c_str(), query.size() + 1, &stm, nullptr);
    if (res != SQLITE_OK)
        error_sqlite::throwf(conn, "cannot compile query '%s'", query.c_str());
}

MySQLStatement::~MySQLStatement()
{
    // Invoking sqlite3_finalize() on a NULL pointer is a harmless no-op.
    sqlite3_finalize(stm);
}

Datetime MySQLStatement::column_datetime(int col)
{
    Datetime res;
    string dt = column_string(col);
    sscanf(dt.c_str(), "%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu",
            &res.date.year,
            &res.date.month,
            &res.date.day,
            &res.time.hour,
            &res.time.minute,
            &res.time.second);
    return res;
}

void MySQLStatement::execute(std::function<void()> on_row)
{
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
                try {
                    on_row();
                } catch (...) {
                    wrap_sqlite3_reset_nothrow();
                    throw;
                }
                break;
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void MySQLStatement::execute_one(std::function<void()> on_row)
{
    bool has_result = false;
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
                if (has_result)
                {
                    wrap_sqlite3_reset();
                    throw error_consistency("query result has more than the one expected row");
                }
                on_row();
                has_result = true;
                break;
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void MySQLStatement::execute()
{
    while (true)
    {
        switch (sqlite3_step(stm))
        {
            case SQLITE_ROW:
            case SQLITE_DONE:
                wrap_sqlite3_reset();
                return;
            case SQLITE_BUSY:
            case SQLITE_MISUSE:
            default:
                reset_and_throw("cannot execute the query");
        }
    }
}

void MySQLStatement::bind_null_val(int idx)
{
    if (sqlite3_bind_null(stm, idx) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a NULL input column");
}

void MySQLStatement::bind_val(int idx, int val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void MySQLStatement::bind_val(int idx, unsigned val)
{
    if (sqlite3_bind_int64(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int64 input column");
}

void MySQLStatement::bind_val(int idx, unsigned short val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void MySQLStatement::bind_val(int idx, const Datetime& val)
{
    char* buf;
    int size = asprintf(&buf, "%04d-%02d-%02d %02d:%02d:%02d",
            val.date.year, val.date.month, val.date.day,
            val.time.hour, val.time.minute, val.time.second);
    if (sqlite3_bind_text(stm, idx, buf, size, free) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a text (from Datetime) input column");
}

void MySQLStatement::bind_val(int idx, const char* val)
{
    if (sqlite3_bind_text(stm, idx, val, -1, SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void MySQLStatement::bind_val(int idx, const std::string& val)
{
    if (sqlite3_bind_text(stm, idx, val.data(), val.size(), SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void MySQLStatement::wrap_sqlite3_reset()
{
    if (sqlite3_reset(stm) != SQLITE_OK)
        throw error_sqlite(conn, "cannot reset the query");
}

void MySQLStatement::wrap_sqlite3_reset_nothrow() noexcept
{
    sqlite3_reset(stm);
}

void MySQLStatement::reset_and_throw(const std::string& errmsg)
{
    std::string sqlite_errmsg(sqlite3_errmsg(conn));
    wrap_sqlite3_reset_nothrow();
    throw error_sqlite(sqlite_errmsg, errmsg);
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

#if 0
void ODBCStatement::close_cursor()
{
    int sqlres = SQLCloseCursor(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing cursor");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

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
