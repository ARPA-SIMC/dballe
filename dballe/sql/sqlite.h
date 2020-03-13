/** @file
 * SQLite DB connector
 */
#ifndef DBALLE_SQL_SQLITE_H
#define DBALLE_SQL_SQLITE_H

#include <dballe/core/error.h>
#include <dballe/sql/sql.h>
#include <sqlite3.h>
#include <vector>
#include <functional>

namespace dballe {
namespace sql {
struct SQLiteStatement;

/**
 * Report an SQLite error
 */
struct error_sqlite : public dballe::error_db
{
    std::string msg;

    error_sqlite(sqlite3* db, const std::string& msg);
    error_sqlite(const std::string& dbmsg, const std::string& msg);
    ~error_sqlite() noexcept {}

    const char* what() const noexcept override { return msg.c_str(); }

    static void throwf(sqlite3* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
};

/// Database connection
class SQLiteConnection : public Connection
{
protected:
    /// Connection pathname
    std::string pathname;
    /// Connection flags
    int flags = 0;
    /// Database connection
    sqlite3* db = nullptr;
    /// Marker to catch attempts to reuse connections in forked processes
    bool forked = false;

    void init_after_connect();
    static void on_sqlite3_profile(void* arg, const char* query, sqlite3_uint64 usecs);

    SQLiteConnection();

    void fork_prepare() override;
    void fork_parent() override;
    void fork_child() override;

    void check_connection();

    void reopen();

public:
    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection(const SQLiteConnection&&) = delete;
    ~SQLiteConnection();

    static std::shared_ptr<SQLiteConnection> create();

    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    operator sqlite3*() { return db; }

    void open_file(const std::string& pathname, int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    void open_memory(int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    void open_private_file(int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    std::unique_ptr<Transaction> transaction(bool readonly=false) override;
    std::unique_ptr<SQLiteStatement> sqlitestatement(const std::string& query);

    bool has_table(const std::string& name) override;
    std::string get_setting(const std::string& key) override;
    void set_setting(const std::string& key, const std::string& value) override;
    void drop_settings() override;
    void execute(const std::string& query) override;
    void explain(const std::string& query, FILE* out) override;

    /**
     * Delete a table in the database if it exists, otherwise do nothing.
     */
    void drop_table_if_exists(const char* name);

    /**
     * Return LAST_INSERT_ID or LAST_INSER_ROWID or whatever is appropriate for
     * the current database, if supported.
     *
     * If not supported, an exception is thrown.
     */
    int get_last_insert_id();

    /// Count the number of rows modified by the last query that was run
    int changes();

    /// Wrap sqlite3_exec, without a callback
    void exec(const std::string& query);
    void exec_nothrow(const std::string& query) noexcept;

#if SQLITE_VERSION_NUMBER >= 3014000
    /**
     * Enable/change/disable SQLite tracing.
     *
     * See sqlite3_trace_v2 docmentation for values for mask use 0 to disable.
     */
    void trace(unsigned mask=SQLITE_TRACE_STMT);
#endif
};

/// SQLite statement
struct SQLiteStatement
{
    SQLiteConnection& conn;
    std::string query;
    sqlite3_stmt *stm = nullptr;

    SQLiteStatement(SQLiteConnection& conn, const std::string& query);
    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement(const SQLiteStatement&&) = delete;
    ~SQLiteStatement();
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    /**
     * Bind all the arguments in a single invocation.
     *
     * Note that the parameter positions are used as bind column numbers, so
     * calling this function twice will re-bind columns instead of adding new
     * ones.
     */
    template<typename... Args> void bind(const Args& ...args)
    {
        bindn<sizeof...(args)>(args...);
    }

    void bind_null_val(int idx);
    void bind_val(int idx, int val);
    void bind_val(int idx, unsigned val);
    void bind_val(int idx, unsigned short val);
    void bind_val(int idx, const Datetime& val);
    void bind_val(int idx, const char* val); // Warning: SQLITE_STATIC is used
    void bind_val(int idx, const std::string& val); // Warning: SQLITE_STATIC is used
    void bind_val(int idx, const std::vector<uint8_t>& val); // Warning: SQLITE_STATIC is used

    /// Run the query, ignoring all results
    void execute();

    /**
     * Run the query, calling on_row for every row in the result.
     *
     * At the end of the function, the statement is reset, even in case an
     * exception is thrown.
     */
    void execute(std::function<void()> on_row);

    /**
     * Run the query, raising an error if there is more than one row in the
     * result
     */
    void execute_one(std::function<void()> on_row);

    /// Read the int value of a column in the result set (0-based)
    int column_int(int col) { return sqlite3_column_int(stm, col); }

    /// Read the int value of a column in the result set (0-based)
    sqlite3_int64 column_int64(int col) { return sqlite3_column_int64(stm, col); }

    /// Read the double value of a column in the result set (0-based)
    double column_double(int col) { return sqlite3_column_double(stm, col); }

    /// Read the string value of a column in the result set (0-based)
    const char* column_string(int col) { return (const char*)sqlite3_column_text(stm, col); }

    /// Read the string value of a column in the result set (0-based)
    std::vector<uint8_t> column_blob(int col) {
        int size = sqlite3_column_bytes(stm, col);
        const uint8_t* val = (const uint8_t*)sqlite3_column_blob(stm, col);
        return std::vector<uint8_t>(val, val + size);
    }

    /// Read the string value of a column and parse it as a Datetime
    Datetime column_datetime(int col);

    /// Check if a column has a NULL value (0-based)
    bool column_isnull(int col) { return sqlite3_column_type(stm, col) == SQLITE_NULL; }

    void wrap_sqlite3_reset();
    void wrap_sqlite3_reset_nothrow() noexcept;
    /**
     * Get the current error message, reset the statement and throw
     * error_sqlite
     */
    [[noreturn]] void reset_and_throw(const std::string& errmsg);

    operator sqlite3_stmt*() { return stm; }
#if 0
    /// @return SQLExecute's result
    int execute();
    /// @return SQLExecute's result
    int exec_direct(const char* query);
    /// @return SQLExecute's result
    int exec_direct(const char* query, int qlen);

    /// @return SQLExecute's result
    int execute_and_close();
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query);
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query, int qlen);

    /**
     * @return the number of columns in the result set (or 0 if the statement
     * did not return columns)
     */
    int columns_count();
    bool fetch();
    bool fetch_expecting_one();
    void close_cursor();
    void close_cursor_if_needed();
    /// Row count for select operations
    size_t select_rowcount();
    /// Row count for insert, delete and other non-select operations
    size_t rowcount();
#endif

private:
    // Implementation of variadic bind: terminating condition
    template<size_t total> void bindn() {}
    // Implementation of variadic bind: recursive iteration over the parameter pack
    template<size_t total, typename ...Args, typename T> void bindn(const T& first, const Args& ...args)
    {
        bind_val(total - sizeof...(args), first);
        bindn<total>(args...);
    }
};

}
}
#endif

