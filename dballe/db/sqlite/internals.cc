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
#include "dballe/core/vasprintf.h"
#include "dballe/db/querybuf.h"
#include <cstdlib>
#if 0
#include <cstring>
#include <limits.h>
#include <unistd.h>
#include "dballe/core/verbose.h"
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

    exec("PRAGMA foreign_keys = ON");

    if (getenv("DBA_INSECURE_SQLITE") != NULL)
    {
        exec("PRAGMA synchronous = OFF");
        exec("PRAGMA journal_mode = OFF");
        exec("PRAGMA legacy_file_format = 0");
    } else {
        exec("PRAGMA journal_mode = MEMORY");
        exec("PRAGMA legacy_file_format = 0");
    }
}

void SQLiteConnection::exec(const std::string& query)
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

void SQLiteConnection::exec_nothrow(const std::string& query) noexcept
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
        conn.exec("COMMIT");
        fired = true;
    }
    void rollback() override
    {
        conn.exec("ROLLBACK");
        fired = true;
    }
    void rollback_nothrow()
    {
        conn.exec_nothrow("ROLLBACK");
        fired = true;
    }
    void lock_table(const char* name) override
    {
        // Nothing to do: SQLite locks the whole database, so table locking
        // is not needed
    }
};

std::unique_ptr<Transaction> SQLiteConnection::transaction()
{
    exec("BEGIN");
    return unique_ptr<Transaction>(new SQLiteTransaction(*this));
}

std::unique_ptr<SQLiteStatement> SQLiteConnection::sqlitestatement(const std::string& query)
{
    return unique_ptr<SQLiteStatement>(new SQLiteStatement(*this, query));
}

void SQLiteConnection::drop_table_if_exists(const char* name)
{
    exec(string("DROP TABLE IF EXISTS ") + name);
}

int SQLiteConnection::get_last_insert_id()
{
    int res = sqlite3_last_insert_rowid(db);
    if (res == 0)
        throw error_consistency("no last insert ID value returned from database");
    return res;
}

bool SQLiteConnection::has_table(const std::string& name)
{
    auto stm = sqlitestatement("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?");
    stm->bind(name);

    bool found = false;
    int count = 0;;
    stm->execute_one([&]() {
        found = true;
        count += stm->column_int(0);
    });

    return count > 0;
}

std::string SQLiteConnection::get_setting(const std::string& key)
{
    if (!has_table("dballe_settings"))
        return string();

    auto stm = sqlitestatement("SELECT value FROM dballe_settings WHERE \"key\"=?");
    stm->bind(key);
    string res;
    stm->execute_one([&]() {
        res = stm->column_string(0);
    });

    return res;
}

void SQLiteConnection::set_setting(const std::string& key, const std::string& value)
{
    if (!has_table("dballe_settings"))
        exec("CREATE TABLE dballe_settings (\"key\" CHAR(64) NOT NULL PRIMARY KEY, value CHAR(64) NOT NULL)");

    auto stm = sqlitestatement("INSERT OR REPLACE INTO dballe_settings (\"key\", value) VALUES (?, ?)");
    stm->bind(key, value);
    stm->execute();
}

void SQLiteConnection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

void SQLiteConnection::add_datetime(Querybuf& qb, const int* dt) const
{
    qb.appendf("'%04d-%02d-%02d %02d:%02d:%02d'", dt[0], dt[1], dt[2], dt[3], dt[4], dt[5]);
}

int SQLiteConnection::changes()
{
    return sqlite3_changes(db);
}

SQLiteStatement::SQLiteStatement(SQLiteConnection& conn, const std::string& query)
    : conn(conn)
{
    // From http://www.sqlite.org/c3ref/prepare.html:
    // If the caller knows that the supplied string is nul-terminated, then
    // there is a small performance advantage to be gained by passing an nByte
    // parameter that is equal to the number of bytes in the input string
    // including the nul-terminator bytes as this saves SQLite from having to
    // make a copy of the input string.
    int res = sqlite3_prepare_v2(conn, query.c_str(), query.size() + 1, &stm, nullptr);
    if (res != SQLITE_OK)
        error_sqlite::throwf(conn, "cannot compile query '%s'", query.c_str());
}

SQLiteStatement::~SQLiteStatement()
{
    // Invoking sqlite3_finalize() on a NULL pointer is a harmless no-op.
    sqlite3_finalize(stm);
}

Datetime SQLiteStatement::column_datetime(int col)
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

void SQLiteStatement::execute(std::function<void()> on_row)
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

void SQLiteStatement::execute_one(std::function<void()> on_row)
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

void SQLiteStatement::execute()
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

void SQLiteStatement::bind_null_val(int idx)
{
    if (sqlite3_bind_null(stm, idx) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a NULL input column");
}

void SQLiteStatement::bind_val(int idx, int val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void SQLiteStatement::bind_val(int idx, unsigned val)
{
    if (sqlite3_bind_int64(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int64 input column");
}

void SQLiteStatement::bind_val(int idx, unsigned short val)
{
    if (sqlite3_bind_int(stm, idx, val) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind an int input column");
}

void SQLiteStatement::bind_val(int idx, const Datetime& val)
{
    char* buf;
    int size = asprintf(&buf, "%04d-%02d-%02d %02d:%02d:%02d",
            val.date.year, val.date.month, val.date.day,
            val.time.hour, val.time.minute, val.time.second);
    if (sqlite3_bind_text(stm, idx, buf, size, free) != SQLITE_OK)
        throw error_sqlite(conn, "cannot bind a text (from Datetime) input column");
}

void SQLiteStatement::bind_val(int idx, const char* val)
{
    if (sqlite3_bind_text(stm, idx, val, -1, SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void SQLiteStatement::bind_val(int idx, const std::string& val)
{
    if (sqlite3_bind_text(stm, idx, val.data(), val.size(), SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a text input column");
}

void SQLiteStatement::wrap_sqlite3_reset()
{
    if (sqlite3_reset(stm) != SQLITE_OK)
        throw error_sqlite(conn, "cannot reset the query");
}

void SQLiteStatement::wrap_sqlite3_reset_nothrow() noexcept
{
    sqlite3_reset(stm);
}

void SQLiteStatement::reset_and_throw(const std::string& errmsg)
{
    std::string sqlite_errmsg(sqlite3_errmsg(conn));
    wrap_sqlite3_reset_nothrow();
    throw error_sqlite(sqlite_errmsg, errmsg);
}

}
}
