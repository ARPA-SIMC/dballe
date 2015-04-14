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

#ifndef DBALLE_DB_MYSQL_INTERNALS_H
#define DBALLE_DB_MYSQL_INTERNALS_H

#include <dballe/db/db.h>
#include <dballe/db/sql.h>
#include <cstdlib>
#include <mysql.h>

namespace dballe {
namespace db {
struct MySQLStatement;

/**
 * Report a MySQL error
 */
struct error_mysql : public db::error
{
    std::string msg;

    /**
     * Copy informations from the ODBC diagnostic record to the dba error
     * report
     */
    error_mysql(MYSQL* db, const std::string& msg);
    error_mysql(const std::string& dbmsg, const std::string& msg);
    ~error_mysql() throw () {}

    wreport::ErrorCode code() const throw () { return wreport::WR_ERR_ODBC; }

    virtual const char* what() const throw () { return msg.c_str(); }

    static void throwf(MYSQL* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
};

namespace mysql {

struct ConnectInfo
{
    std::string host;
    std::string user;
    bool has_passwd = false;
    std::string passwd;
    bool has_dbname = false;
    std::string dbname;
    unsigned port = 0;
    std::string unix_socket;

    // Reset everything to defaults
    void reset();
    void parse_url(const std::string& url);
    // Modeled after http://dev.mysql.com/doc/connector-j/en/connector-j-reference-configuration-properties.html
    std::string to_url() const;
};

struct Row
{
    MYSQL_RES* res = nullptr;
    MYSQL_ROW row = nullptr;

    Row(MYSQL_RES* res, MYSQL_ROW row) : res(res), row(row) {}

    operator bool() const { return row != nullptr; }
    operator MYSQL_ROW() { return row; }
    operator const MYSQL_ROW() const { return row; }

    int as_int(unsigned col) const { return strtol(row[col], 0, 10); }
    unsigned as_unsigned(unsigned col) const { return strtoul(row[col], 0, 10); }
    const char* as_cstring(unsigned col) const { return row[col]; }
    std::string as_string(unsigned col) const { return std::string(row[col], mysql_fetch_lengths(res)[col]); }
    bool isnull(unsigned col) const { return row[col] == nullptr; }
};

struct Result
{
    MYSQL_RES* res = nullptr;

    Result() : res(nullptr) {}
    Result(MYSQL_RES* res) : res(res) {}
    ~Result() { if (res) mysql_free_result(res); }

    /// Implement move
    Result(Result&& o) : res(o.res) { o.res = nullptr; }
    Result& operator=(Result&& o)
    {
        if (this == &o) return *this;
        if (res) mysql_free_result(res);
        res = o.res;
        o.res = nullptr;
        return *this;
    }

    operator bool() const { return res != nullptr; }

    operator MYSQL_RES*() { return res; }
    operator const MYSQL_RES*() const { return res; }

    unsigned rowcount() const { return mysql_num_rows(res); }

    /// Check that the function returned only one row, and return that row.
    Row expect_one_result();

    /**
     * Fetch one row.
     *
     * Note: mysql_fetch_row does not reset the error indicator, so there is no
     * way to tell the end of the iteration from an error. As a consequence,
     * this is only safe to call after a mysql_store_result.
     *
     * Accessing mysql_use_result results is implemented using execute_fetch.
     */
    Row fetch() { return Row(res, mysql_fetch_row(res)); }

    // Prevent copy
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
};

}


/// Database connection
class MySQLConnection : public Connection
{
protected:
    /// Database connection
    MYSQL* db = nullptr;

protected:
    void init_after_connect();

public:
    MySQLConnection();
    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection(const MySQLConnection&&) = delete;
    ~MySQLConnection();
    MySQLConnection& operator=(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&&) = delete;

    operator MYSQL*() { return db; }

    // See https://dev.mysql.com/doc/refman/5.0/en/mysql-real-connect.html
    void open(const mysql::ConnectInfo& info);
    // See http://dev.mysql.com/doc/connector-j/en/connector-j-reference-configuration-properties.html
    void open_url(const std::string& url);
    void open_test();

    /// Escape a C string
    std::string escape(const char* str);
    /// Escape a string
    std::string escape(const std::string& str);

    // Run a query, checking that it is successful and it gives no results
    void exec_no_data(const char* query);
    // Run a query, checking that it is successful and it gives no results
    void exec_no_data(const std::string& query);
    // Run a query, with a locally stored result
    mysql::Result exec_store(const char* query);
    // Run a query, with a locally stored result
    mysql::Result exec_store(const std::string& query);
#if 0
    // Run a query, with a remotely fetched result
    void execute_fetch(const char* query, std::function<void, mysql::Row> dest);
    // Run a query, with a remotely fetched result
    void execute_fetch(const std::string& query, std::function<void, mysql::Row> dest);
#endif

    std::unique_ptr<Transaction> transaction() override;
#if 0
    std::unique_ptr<MySQLStatement> mysqlstatement(const std::string& query);
#endif

    /// Check if the database contains a table
    bool has_table(const std::string& name) override;

    /**
     * Get a value from the settings table.
     *
     * Returns the empty string if the table does not exist.
     */
    std::string get_setting(const std::string& key) override;

    /**
     * Set a value in the settings table.
     *
     * The table is created if it does not exist.
     */
    void set_setting(const std::string& key, const std::string& value) override;

    /// Drop the settings table
    void drop_settings() override;

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
    //int changes();

    void add_datetime(Querybuf& qb, const int* dt) const override;

#if 0
    /// Wrap sqlite3_exec, without a callback
    void wrap_sqlite3_exec(const std::string& query);
    void wrap_sqlite3_exec_nothrow(const std::string& query) noexcept;
#endif
};

#if 0
/// MySQL statement
struct MySQLStatement
{
    MySQLConnection& conn;
    sqlite3_stmt *stm = nullptr;

    MySQLStatement(MySQLConnection& conn, const std::string& query);
    MySQLStatement(const MySQLStatement&) = delete;
    MySQLStatement(const MySQLStatement&&) = delete;
    ~MySQLStatement();
    MySQLStatement& operator=(const MySQLStatement&) = delete;

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
#endif

}
}
#endif

