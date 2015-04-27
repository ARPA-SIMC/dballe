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

    // Split date and time
    int time = decoded % 86400;
    int jdate = decoded / 86400;
    if (time < 0)
    {
        time += 86400;
        jdate -= 1;
    }

    // Decode time
    dt.time.hour = time / 3600;
    dt.time.minute = (time / 60) % 60;
    dt.time.second = time % 60;

    // Decode date
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

void PostgreSQLConnection::open_url(const std::string& connection_string)
{
    db = PQconnectdb(connection_string.c_str());
    if (PQstatus(db) != CONNECTION_OK)
        throw error_postgresql(db, "opening " + connection_string);
    init_after_connect();
}

void PostgreSQLConnection::open_test()
{
    const char* envurl = getenv("DBA_DB_POSTGRESQL");
    if (envurl == NULL)
        throw error_consistency("DBA_DB_POSTGRESQL not defined");
    return open_url(envurl);
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
    void lock_table(const char* name) override
    {
        char buf[256];
        snprintf(buf, 256, "LOCK TABLE %s IN EXCLUSIVE MODE", name);
        conn.pqexec(buf);
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

void PostgreSQLConnection::drop_table_if_exists(const char* name)
{
    exec_no_data(string("DROP TABLE IF EXISTS ") + name);
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

int PostgreSQLConnection::changes()
{
    throw error_unimplemented("changes");
    //return sqlite3_changes(db);
}

}
}
