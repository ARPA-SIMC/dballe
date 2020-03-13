#include "sqlite.h"
#include "querybuf.h"
#include "dballe/types.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

namespace {

#if SQLITE_VERSION_NUMBER >= 3014000
static int trace_callback(unsigned T, void* C, void* P,void* X)
{
    switch (T)
    {
        case SQLITE_TRACE_STMT:
        {
            sqlite3* db = sqlite3_db_handle((sqlite3_stmt*)P);
            bool is_autocommit = sqlite3_get_autocommit(db);
            fprintf(stderr, "SQLite: %sstarted %s\n", is_autocommit ? "AC " : "", sqlite3_expanded_sql((sqlite3_stmt*)P));
            break;
        }
        case SQLITE_TRACE_PROFILE:
            fprintf(stderr, "SQLite: completed %s in %.9fs\n",
                    sqlite3_expanded_sql((sqlite3_stmt*)P), *(int64_t*)X / 1000000000.0);

            break;
        case SQLITE_TRACE_ROW:
            fprintf(stderr, "SQLite: got a row of result\n");
            break;
        case SQLITE_TRACE_CLOSE:
            fprintf(stderr, "SQLite: connection closed %p\n", P);
            break;
    }
    return 0;
}
#endif

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
    char buf[512];

    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    throw error_sqlite(db, buf);
}

SQLiteConnection::SQLiteConnection()
{
}

SQLiteConnection::~SQLiteConnection()
{
    if (db) sqlite3_close(db);
}

std::shared_ptr<SQLiteConnection> SQLiteConnection::create()
{
    auto res = std::shared_ptr<SQLiteConnection>(new SQLiteConnection);
    res->register_atfork();
    return res;
}

void SQLiteConnection::reopen()
{
    if (db)
    {
        if (sqlite3_close(db) != SQLITE_OK)
            throw error_sqlite("?", "failed to close db");
        db = nullptr;
    }

    size_t qs_begin = pathname.find('?');
    int res;
    if (qs_begin == string::npos)
        res = sqlite3_open_v2(pathname.c_str(), &db, flags, nullptr);
    else
        res = sqlite3_open_v2(pathname.substr(0, qs_begin).c_str(), &db, flags, nullptr);
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

void SQLiteConnection::open_file(const std::string& pathname, int flags)
{
    this->pathname = pathname;
    this->flags = flags;
    url = "sqlite://" + pathname;
    reopen();
}

void SQLiteConnection::open_memory(int flags)
{
    url = "sqlite://:memory:";
    open_file(":memory:", flags);
}

void SQLiteConnection::open_private_file(int flags)
{
    url = "sqlite://";
    open_file("", flags);
}

void SQLiteConnection::fork_prepare()
{
}

void SQLiteConnection::fork_parent()
{
}

void SQLiteConnection::fork_child()
{
    forked = true;
    // TODO: close the underlying file descriptor (how?) instead of leaking it
    db = nullptr;
}

void SQLiteConnection::check_connection()
{
    if (forked)
        throw error_sqlite("sqlite handle not safe", "database connections cannot be used after forking");
}

void SQLiteConnection::on_sqlite3_profile(void* arg, const char* query, sqlite3_uint64 usecs)
{
    fprintf(stderr, "sqlite:%.3fs:%s\n", (double)usecs / 1000000000.0, query);
}

void SQLiteConnection::init_after_connect()
{
    server_type = ServerType::SQLITE;
    // autocommit is off by default when inside a transaction
    // set_autocommit(false);

    exec("PRAGMA foreign_keys = ON");
    exec("PRAGMA journal_mode = MEMORY");
    exec("PRAGMA legacy_file_format = 0");

    if (getenv("DBA_INSECURE_SQLITE") != NULL)
        exec("PRAGMA synchronous = OFF");

    if (getenv("DBA_PROFILE") != nullptr)
        sqlite3_profile(db, on_sqlite3_profile, this);
}

void SQLiteConnection::exec(const std::string& query)
{
    check_connection();
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
        throw error_sqlite(msg, "executing " + query);
    }
}

void SQLiteConnection::exec_nothrow(const std::string& query) noexcept
{
    if (forked)
        return;
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
        fprintf(stderr, "sqlite3 cleanup: cannot execute '%s': %s\n", query.c_str(), errmsg);
        sqlite3_free(errmsg);
    }
}

void SQLiteConnection::execute(const std::string& query)
{
    exec(query);
}

void SQLiteConnection::explain(const std::string& query, FILE* out)
{
    string explain_query = "EXPLAIN QUERY PLAN ";
    explain_query += query;

    fprintf(out, "%s\n", explain_query.c_str());
    auto stm = sqlitestatement(explain_query);
    fprintf(out, "sid\torder\tfrom\tdetail\n");
    stm->execute([&]() {
        int selectid = stm->column_int(0);
        int order = stm->column_int(1);
        int from = stm->column_int(2);
        const char* detail = stm->column_string(3);
        fprintf(out, "%d\t%d\t%d\t%s\n", selectid, order, from, detail);
    });
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
    void rollback_nothrow() noexcept override
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

std::unique_ptr<Transaction> SQLiteConnection::transaction(bool readonly)
{
    // readonly is currently ignored on sqlite
    exec("BEGIN");
    return unique_ptr<Transaction>(new SQLiteTransaction(*this));
}

std::unique_ptr<SQLiteStatement> SQLiteConnection::sqlitestatement(const std::string& query)
{
    check_connection();
    return unique_ptr<SQLiteStatement>(new SQLiteStatement(*this, query));
}

void SQLiteConnection::drop_table_if_exists(const char* name)
{
    exec(string("DROP TABLE IF EXISTS ") + name);
}

int SQLiteConnection::get_last_insert_id()
{
    check_connection();
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

int SQLiteConnection::changes()
{
    return sqlite3_changes(db);
}

#if SQLITE_VERSION_NUMBER >= 3014000
void SQLiteConnection::trace(unsigned mask)
{
    check_connection();
    if (sqlite3_trace_v2(db, mask, trace_callback, nullptr) != SQLITE_OK)
        error_sqlite::throwf(db, "Cannot set up SQLite tracing");
}
#endif

SQLiteStatement::SQLiteStatement(SQLiteConnection& conn, const std::string& query)
    : conn(conn), query(query)
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
            &res.year, &res.month, &res.day,
            &res.hour, &res.minute, &res.second);
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
                reset_and_throw("cannot execute the query " + query);
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
                reset_and_throw("cannot execute the query " + query);
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
                reset_and_throw("cannot execute the query " + query);
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
            val.year, val.month, val.day,
            val.hour, val.minute, val.second);
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

void SQLiteStatement::bind_val(int idx, const std::vector<uint8_t>& val)
{
    if (sqlite3_bind_blob(stm, idx, val.data(), val.size(), SQLITE_STATIC))
        throw error_sqlite(conn, "cannot bind a blob input column");
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
