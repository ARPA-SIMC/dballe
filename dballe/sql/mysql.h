/** @file
 * MySQL DB connector
 */
#ifndef DBALLE_SQL_MYSQL_H
#define DBALLE_SQL_MYSQL_H

#include <dballe/sql/sql.h>
#include <cstdlib>
#include <mysql.h>

namespace dballe {
namespace sql {
struct MySQLStatement;

/**
 * Report a MySQL error
 */
struct error_mysql : public error_db
{
    std::string msg;

    /**
     * Copy informations from the ODBC diagnostic record to the dba error
     * report
     */
    error_mysql(MYSQL* db, const std::string& msg);
    error_mysql(const std::string& dbmsg, const std::string& msg);
    ~error_mysql() throw () {}

    const char* what() const noexcept override { return msg.c_str(); }

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
    Datetime as_datetime(int col) const;
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
    unsigned colcount() const { return mysql_num_fields(res); }

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

    void send_result(mysql::Result&& res, std::function<void(const mysql::Row&)> dest);

protected:
    void init_after_connect();

    // See https://dev.mysql.com/doc/refman/5.0/en/mysql-real-connect.html
    void open(const mysql::ConnectInfo& info);

public:
    MySQLConnection();
    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection(const MySQLConnection&&) = delete;
    ~MySQLConnection();
    MySQLConnection& operator=(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&&) = delete;

    operator MYSQL*() { return db; }

    // See http://dev.mysql.com/doc/connector-j/en/connector-j-reference-configuration-properties.html
    void open_url(const std::string& url);
    void open_test();

    /// Escape a C string
    std::string escape(const char* str);
    /// Escape a string
    std::string escape(const std::string& str);

    /**
     * Run a query throwing no exceptions, warning on stderr if it is not
     * successful or if it gives a nonempty result
     */
    void exec_no_data_nothrow(const char* query) noexcept;
    // Run a query, checking that it is successful and it gives no results
    void exec_no_data(const char* query);
    // Run a query, checking that it is successful and it gives no results
    void exec_no_data(const std::string& query);
    // Run a query, with a locally stored result
    mysql::Result exec_store(const char* query);
    // Run a query, with a locally stored result
    mysql::Result exec_store(const std::string& query);
    // Run a query, with a remotely fetched result
    void exec_use(const char* query, std::function<void(const mysql::Row&)> dest);
    // Run a query, with a remotely fetched result
    void exec_use(const std::string& query, std::function<void(const mysql::Row&)> dest);

    std::unique_ptr<Transaction> transaction() override;
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
};

}
}
#endif

