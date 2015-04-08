/*
 * db/sqlite/internals - Implementation infrastructure for the PostgreSQL DB connection
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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "dballe/core/vasprintf.h"
#include "dballe/db/querybuf.h"
#include <cstdlib>
#include <arpa/inet.h>
#define _BSD_SOURCE             /* See feature_test_macros(7) */
#include <endian.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

namespace postgresql {

// From http://libpqtypes.esilo.com/browse_source.html?file=datetime.c
static const int EPOCH_JDATE = 2451545; // == 2000-01-01

int64_t encode_int64_t(int64_t arg)
{
    return htobe64(arg);
}

int64_t encode_datetime(const Datetime& arg)
{
    int64_t encoded = arg.date.to_julian() - EPOCH_JDATE;
    encoded *= 86400;
    encoded += arg.time.hour * 3600 + arg.time.minute * 60 + arg.time.second;
    encoded *= 1000000;
    return (int64_t)htobe64(encoded);
}

void Result::expect_no_data(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_COMMAND_OK:
            break;
        case PGRES_TUPLES_OK:
            throw error_postgresql("tuples_ok", "data (possibly an empty set) returned by " + query);
        default:
            throw error_postgresql(res, "executing " + query);
    }
}

void Result::expect_result(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK:
            break;
        case PGRES_COMMAND_OK:
            throw error_postgresql("command_ok", "no data returned by " + query);
        default:
            throw error_postgresql(res, "executing " + query);
    }
}

void Result::expect_one_row(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK:
            break;
        case PGRES_COMMAND_OK:
            throw error_postgresql("command_ok", "no data returned by " + query);
        default:
            throw error_postgresql(res, "executing " + query);
    }
    unsigned rows = rowcount();
    if (rows != 1)
        error_consistency::throwf("Got %u results instead of 1 when running %s", rows, query.c_str());
}

void Result::expect_success(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK:
        case PGRES_COMMAND_OK:
            break;
        default:
            throw error_postgresql(res, "executing " + query);
    }
}

/// Return a result value, transmitted in binary as a 4 bit integer
uint64_t Result::get_int8(unsigned row, unsigned col) const
{
    char* val = PQgetvalue(res, row, col);
    return be64toh(*(uint64_t*)val);
}

Datetime Result::get_timestamp(unsigned row, unsigned col) const
{
    // Adapter from http://libpqtypes.esilo.com/browse_source.html?file=datetime.c
    Datetime dt;

    // Decode from big endian
    int64_t decoded = be64toh(*(uint64_t*)PQgetvalue(res, row, col));

    // Convert from microseconds to seconds
    decoded = decoded / 1000000;

    // Decode time
    int time = decoded % 86400;
    dt.time.hour = time / 3600;
    dt.time.minute = (time / 60) % 60;
    dt.time.second = time % 60;

    // Decode date
    int jdate = decoded / 86400;
    jdate += EPOCH_JDATE;
    dt.date.from_julian(jdate);

    return dt;
}

}


error_postgresql::error_postgresql(PGconn* db, const std::string& msg)
{
    this->msg = msg;
    this->msg += ": ";
    this->msg += PQerrorMessage(db);
}

error_postgresql::error_postgresql(PGresult* res, const std::string& msg)
{
    this->msg = msg;
    this->msg += ": ";
    this->msg += PQresultErrorMessage(res);
}

error_postgresql::error_postgresql(const std::string& dbmsg, const std::string& msg)
{
    this->msg = msg;
    this->msg += ": ";
    this->msg += dbmsg;
}

void error_postgresql::throwf(PGconn* db, const char* fmt, ...)
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
    throw error_postgresql(db, msg);
}

void error_postgresql::throwf(PGresult* res, const char* fmt, ...)
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
    throw error_postgresql(res, msg);
}

PostgreSQLConnection::PostgreSQLConnection()
{
}

PostgreSQLConnection::~PostgreSQLConnection()
{
    if (db) PQfinish(db);
}

void PostgreSQLConnection::open(const std::string& connection_string)
{
    db = PQconnectdb(connection_string.c_str());
    if (PQstatus(db) != CONNECTION_OK)
        throw error_postgresql(db, "opening " + connection_string);
    init_after_connect();
}

void PostgreSQLConnection::init_after_connect()
{
    server_type = ServerType::POSTGRES;
    // Hide warning notices, like "table does not exists" in "DROP TABLE ... IF EXISTS"
    exec_no_data("SET client_min_messages = error");
}

void PostgreSQLConnection::pqexec(const std::string& query)
{
    postgresql::Result res(PQexec(db, query.c_str()));
    res.expect_success(query);
}

void PostgreSQLConnection::pqexec_nothrow(const std::string& query) noexcept
{
    postgresql::Result res(PQexec(db, query.c_str()));
    switch (PQresultStatus(res))
    {
        case PGRES_COMMAND_OK:
        case PGRES_TUPLES_OK:
            return;
        default:
            fprintf(stderr, "cannot execute '%s': %s\n", query.c_str(), PQresultErrorMessage(res));
    }
}

struct PostgreSQLTransaction : public Transaction
{
    PostgreSQLConnection& conn;
    bool fired = false;

    PostgreSQLTransaction(PostgreSQLConnection& conn) : conn(conn)
    {
    }
    ~PostgreSQLTransaction() { if (!fired) rollback_nothrow(); }

    void commit() override
    {
        conn.exec_no_data("COMMIT");
        fired = true;
    }
    void rollback() override
    {
        conn.exec_no_data("ROLLBACK");
        fired = true;
    }
    void rollback_nothrow()
    {
        conn.pqexec_nothrow("ROLLBACK");
        fired = true;
    }
};

std::unique_ptr<Transaction> PostgreSQLConnection::transaction()
{
    exec_no_data("BEGIN");
    return unique_ptr<Transaction>(new PostgreSQLTransaction(*this));
}

void PostgreSQLConnection::prepare(const std::string& name, const std::string& query)
{
    using namespace postgresql;
    Result res(PQprepare(db, name.c_str(), query.c_str(), 0, nullptr));
    res.expect_no_data("prepare:" + query);
}

#if 0
std::unique_ptr<PostgreSQLCompiledQuery> PostgreSQLConnection::pqstatement(const std::string& name, const std::string& query)
{
    return unique_ptr<PostgreSQLCompiledQuery>(new PostgreSQLCompiledQuery(*this, name, query));
}
#endif

void PostgreSQLConnection::impl_exec_void(const std::string& query)
{
    pqexec(query);
}

void PostgreSQLConnection::impl_exec_void_string(const std::string& query, const std::string& arg1)
{
    exec(query, arg1);
}

void PostgreSQLConnection::impl_exec_void_string_string(const std::string& query, const std::string& arg1, const std::string& arg2)
{
    exec(query, arg1, arg2);
}

void PostgreSQLConnection::drop_table_if_exists(const char* name)
{
    exec_no_data(string("DROP TABLE IF EXISTS ") + name);
}

void PostgreSQLConnection::drop_sequence_if_exists(const char* name)
{
    // We do not use sequences with PostgreSQL
}

void PostgreSQLConnection::cancel_running_query_nothrow() noexcept
{
    PGcancel* c = PQgetCancel(db);
    if (!c) return;

    char errbuf[256];
    if (!PQcancel(c, errbuf, 256))
        fprintf(stderr, "cannot send cancellation request: %s\n", errbuf);

    PQfreeCancel(c);
}

void PostgreSQLConnection::discard_all_input_nothrow() noexcept
{
    using namespace postgresql;

    while (true)
    {
        Result res(PQgetResult(db));
        if (!res) break;
        switch (PQresultStatus(res))
        {
            case PGRES_TUPLES_OK: break;
            case PGRES_COMMAND_OK:
                fprintf(stderr, "flushing input from PostgreSQL server returned an empty result\n");
                break;
            default:
                fprintf(stderr, "flushing input from PostgreSQL server returned an error: %s\n", PQresultErrorMessage(res));
                break;
        }
    }
}

int PostgreSQLConnection::get_last_insert_id()
{
    throw error_unimplemented("last insert id for postgres");
}

bool PostgreSQLConnection::has_table(const std::string& name)
{
    using namespace postgresql;

    const char* query = R"(
        SELECT EXISTS (
                SELECT 1
                  FROM pg_catalog.pg_class c
                  JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
                 WHERE n.nspname = 'public'
                   AND c.relname = $1::text
        ))";

    Result res = exec_one_row(query, name);
    return res.get_bool(0, 0);
}

std::string PostgreSQLConnection::get_setting(const std::string& key)
{
    using namespace postgresql;

    if (!has_table("dballe_settings"))
        return string();

    const char* query = "SELECT value FROM dballe_settings WHERE \"key\"=$1::text";
    Result res = exec(query, key);
    if (res.rowcount() == 0)
        return string();
    if (res.rowcount() > 1)
        error_consistency::throwf("got %d results instead of 1 executing %s", res.rowcount(), query);
    return res.get_string(0, 0);
}

void PostgreSQLConnection::set_setting(const std::string& key, const std::string& value)
{
    if (!has_table("dballe_settings"))
        exec_no_data("CREATE TABLE dballe_settings (\"key\" TEXT NOT NULL PRIMARY KEY, value TEXT NOT NULL)");

    auto trans = transaction();
    exec_no_data("LOCK TABLE dballe_settings IN EXCLUSIVE MODE");
    auto s = exec_one_row("SELECT EXISTS (SELECT 1 FROM dballe_settings WHERE \"key\"=$1::text)", key);
    if (s.get_bool(0, 0))
        exec_no_data("UPDATE dballe_settings SET value=$2::text WHERE \"key\"=$1::text", key, value);
    else
        exec_no_data("INSERT INTO dballe_settings (\"key\", value) VALUES ($1::text, $2::text)", key, value);
    trans->commit();
}

void PostgreSQLConnection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

void PostgreSQLConnection::add_datetime(Querybuf& qb, const int* dt) const
{
    qb.appendf("'%04d-%02d-%02d %02d:%02d:%02d'", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
}

int PostgreSQLConnection::changes()
{
    throw error_unimplemented("changes");
    //return sqlite3_changes(db);
}

#if 0
PostgreSQLCompiledQuery::PostgreSQLCompiledQuery(PostgreSQLConnection& conn, const std::string& name, const std::string& query)
    : conn(conn), name(name), query(query)
{
}

PostgreSQLCompiledQuery::~PostgreSQLCompiledQuery()
{
    // FIXME: is this really needed?
    string query = "DEALLOCATE " + name;
    conn.pqexec_nothrow(query.c_str());
}
#endif

#if 0
Datetime PostgreSQLCompiledQuery::column_datetime(int col)
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

void PostgreSQLCompiledQuery::execute(std::function<void()> on_row)
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

void PostgreSQLCompiledQuery::execute_one(std::function<void()> on_row)
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

void PostgreSQLCompiledQuery::execute()
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

void PostgreSQLCompiledQuery::bind_null_val(int idx)
{
    if (sqlite3_bind_null(stm, idx) != SQLITE_OK)
        throw error_postgresql(conn, "cannot bind a NULL input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, int val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_postgresql(conn, "cannot bind an int input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, unsigned val)
{
    if (sqlite3_bind_int64(stm, idx, val) != SQLITE_OK)
        throw error_postgresql(conn, "cannot bind an int64 input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, unsigned short val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_postgresql(conn, "cannot bind an int input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, const Datetime& val)
{
    char* buf;
    int size = asprintf(&buf, "%04d-%02d-%02d %02d:%02d:%02d",
            val.date.year, val.date.month, val.date.day,
            val.time.hour, val.time.minute, val.time.second);
    if (sqlite3_bind_text(stm, idx, buf, size, free) != SQLITE_OK)
        throw error_postgresql(conn, "cannot bind a text (from Datetime) input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, const char* val)
{
    if (sqlite3_bind_text(stm, idx, val, -1, SQLITE_STATIC))
        throw error_postgresql(conn, "cannot bind a text input column");
}

void PostgreSQLCompiledQuery::bind_val(int idx, const std::string& val)
{
    if (sqlite3_bind_text(stm, idx, val.data(), val.size(), SQLITE_STATIC))
        throw error_postgresql(conn, "cannot bind a text input column");
}

void PostgreSQLCompiledQuery::wrap_sqlite3_reset()
{
    if (sqlite3_reset(stm) != SQLITE_OK)
        throw error_postgresql(conn, "cannot reset the query");
}

void PostgreSQLCompiledQuery::wrap_sqlite3_reset_nothrow() noexcept
{
    sqlite3_reset(stm);
}

void PostgreSQLCompiledQuery::reset_and_throw(const std::string& errmsg)
{
    std::string sqlite_errmsg(sqlite3_errmsg(conn));
    wrap_sqlite3_reset_nothrow();
    throw error_postgresql(sqlite_errmsg, errmsg);
}
#endif

#if 0
bool ODBCCompiledQuery::fetch()
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

bool ODBCCompiledQuery::fetch_expecting_one()
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

size_t ODBCCompiledQuery::select_rowcount()
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

size_t ODBCCompiledQuery::rowcount()
{
    SQLLEN res;
    int sqlres = SQLRowCount(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "reading row count");
    return res;
}
#endif

#if 0
void ODBCCompiledQuery::close_cursor()
{
    int sqlres = SQLCloseCursor(stm);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "closing cursor");
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    debug_reached_completion = true;
#endif
}

int ODBCCompiledQuery::execute()
{
#ifdef DEBUG_WARN_OPEN_TRANSACTIONS
    if (!debug_reached_completion)
    {
        string msg = "CompiledQuery " + debug_query + " restarted before reaching completion";
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

int ODBCCompiledQuery::exec_direct(const char* query)
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

int ODBCCompiledQuery::exec_direct(const char* query, int qlen)
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

void ODBCCompiledQuery::close_cursor_if_needed()
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
int ODBCCompiledQuery::execute_and_close()
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

int ODBCCompiledQuery::exec_direct_and_close(const char* query)
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

int ODBCCompiledQuery::exec_direct_and_close(const char* query, int qlen)
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

int ODBCCompiledQuery::columns_count()
{
    SQLSMALLINT res;
    int sqlres = SQLNumResultCols(stm, &res);
    if (is_error(sqlres))
        throw error_odbc(SQL_HANDLE_STMT, stm, "querying number of columns in the result set");
    return res;
}

Sequence::Sequence(ODBCConnection& conn, const char* name)
    : ODBCCompiledQuery(conn)
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
