#include "mysql.h"
#include "querybuf.h"
#include "dballe/types.h"
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

//#define TRACE_MYSQL
#ifdef TRACE_MYSQL
#define trace_query(...) fprintf(stderr, "mysql:" __VA_ARGS__)
#else
#define trace_query(...) do {} while(0)
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

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
    char buf[512];

    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    throw error_mysql(db, buf);
}

namespace mysql {

struct URLParser
{
    const std::string& url;
    ConnectInfo& dest;

    URLParser(const std::string& url, ConnectInfo& dest) : url(url), dest(dest) {}

    void trace(const char* name, std::string::size_type beg, std::string::size_type end) const
    {
        //fprintf(stderr, "TRACE %s: %s %zd--%zd: %.*s\n", url.c_str(), name, beg, end, (int)(end - beg), url.c_str() + beg);
    }

    // Return a substring of url between positions [beg, end)
    std::string cut(std::string::size_type beg, std::string::size_type end)
    {
        trace("cut", beg, end);
        return url.substr(beg, end - beg);
    }

    void parse()
    {
        trace(" *** parse", 0, url.size());
        if (url == "mysql:" || url == "mysql://" || url == "mysql:///")
            return;

        if (url.substr(0, 8) != "mysql://")
            error_consistency::throwf("MySQL connect URL '%s' does not start with mysql://", url.c_str());

        size_t hostport_end = url.find('/', 8);
        if (hostport_end == string::npos)
            error_consistency::throwf("MySQL connect URL '%s' does not end the host:port part with a slash", url.c_str());

        parse_hostport(8, hostport_end);

        size_t qstring_start = url.find('?', hostport_end + 1);
        if (qstring_start == string::npos)
        {
            if (hostport_end + 1 < url.size())
            {
                dest.has_dbname = true;
                dest.dbname = cut(hostport_end + 1, url.size());
            }
        } else {
            if (hostport_end + 1 < qstring_start)
            {
                dest.has_dbname = true;
                dest.dbname = cut(hostport_end + 1, qstring_start);
            }
            parse_qstring(qstring_start, url.size());
        }
    }

    // Parse host:port part of the URL
    void parse_hostport(std::string::size_type beg, std::string::size_type end)
    {
        trace("parse_hostport", beg, end);
        size_t port = url.find(':', beg);
        if (port == string::npos)
            dest.host = cut(beg, end);
        else
        {
            dest.host = cut(beg, port);
            dest.port = stoul(cut(port + 1, end));
        }
    }

    void parse_qstring(std::string::size_type beg, std::string::size_type end)
    {
        trace("parse_qstring", beg, end);
        while (true)
        {
            // Skip leading ? or &
            ++beg;

            size_t next = url.find('&', beg);
            if (next == string::npos)
            {
                // Last element
                parse_qstring_keyval(beg, end);
                break;
            } else {
                parse_qstring_keyval(beg, next);
                beg = next;
            }
        }
    }

    void parse_qstring_keyval(std::string::size_type beg, std::string::size_type end)
    {
        trace("parse_qstring_keyval", beg, end);
        size_t assign = url.find('=', beg);
        if (assign == string::npos)
            handle_keyval(cut(beg, end), string());
        else
            handle_keyval(cut(beg, assign), cut(assign + 1, end));
    }

    void handle_keyval(const std::string& key, const std::string& val)
    {
        if (key == "user")
            dest.user = val;
        else if (key == "password")
        {
            dest.has_passwd = true;
            dest.passwd = val;
        }
    }
};

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

    URLParser parser(url, *this);
    parser.parse();
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
            &res.year, &res.month, &res.day,
            &res.hour, &res.minute, &res.second);
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

std::shared_ptr<MySQLConnection> MySQLConnection::create()
{
    return std::shared_ptr<MySQLConnection>(new MySQLConnection);
}

void MySQLConnection::open(const mysql::ConnectInfo& info)
{
    // See http://www.enricozini.org/2012/tips/sa-sqlmode-traditional/
    mysql_options(db, MYSQL_INIT_COMMAND, "SET sql_mode='traditional'");
    mysql_options(db, MYSQL_SET_CHARSET_NAME, "utf8mb4");
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
    using namespace dballe::sql::mysql;
    trace_query("open_url: %s\n", url.c_str());
    this->url = url;
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

std::string MySQLConnection::escape(const std::vector<uint8_t>& buf)
{
    // Dirty: we write directly inside the resulting std::string storage.
    // It should work in C++11, although not according to its specifications,
    // and if for some reason we discover that it does not work, this can be
    // rewritten with one extra string copy.
    string res(buf.size() * 2 + 1, 0);
    unsigned long len = mysql_hex_string(const_cast<char*>(res.data()), (const char*)buf.data(), buf.size());
    res.resize(len);
    return res;
}

void MySQLConnection::exec_no_data_nothrow(const char* query) noexcept
{
    using namespace dballe::sql::mysql;
    trace_query("exec_no_data_nothrow: %s\n", query);

    if (mysql_query(db, query))
    {
        fprintf(stderr, "cannot execute '%s': %s", query, mysql_error(db));
        return;
    }

    MYSQL_RES* res = mysql_store_result(db);
    if (res != nullptr)
    {
        fprintf(stderr, "query '%s' returned %u rows instead of zero",
                query, (unsigned)mysql_num_rows(res));
        mysql_free_result(res);
        return;
    }
    if (mysql_errno(db))
    {
        fprintf(stderr, "cannot store result of query '%s': %s", query, mysql_error(db));
        mysql_free_result(res);
        return;
    }
}

void MySQLConnection::exec_no_data(const char* query)
{
    using namespace dballe::sql::mysql;
    trace_query("exec_no_data: %s\n", query);

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
    using namespace dballe::sql::mysql;
    trace_query("exec_no_data: %s\n", query.c_str());

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
    using namespace dballe::sql::mysql;
    trace_query("exec_store: %s\n", query);

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
    using namespace dballe::sql::mysql;
    trace_query("exec_store: %s\n", query.c_str());

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
    using namespace dballe::sql::mysql;
    trace_query("exec_use: %s\n", query);

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
    using namespace dballe::sql::mysql;
    trace_query("exec_use: %s\n", query.c_str());

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
    using namespace dballe::sql::mysql;

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
        conn.exec_no_data("COMMIT");
        fired = true;
    }
    void rollback() override
    {
        conn.exec_no_data("ROLLBACK");
        fired = true;
    }
    void rollback_nothrow() noexcept override
    {
        conn.exec_no_data_nothrow("ROLLBACK");
        fired = true;
    }
    void lock_table(const char* name) override
    {
        // https://dev.mysql.com/doc/refman/5.0/en/lock-tables-and-transactions.html
        //   LOCK TABLES is not transaction-safe and implicitly commits any active transaction before attempting to lock the tables.
        //
        // So we do nothing here, and pray that MySQL's default transaction
        // isolation is enough to prevent most concurrency problems.
    }
};

std::unique_ptr<Transaction> MySQLConnection::transaction(bool readonly)
{
    // The default MySQL isolation level is REPEATABLE READ
    // https://dev.mysql.com/doc/refman/5.7/en/innodb-transaction-isolation-levels.html
    if (readonly)
        exec_no_data("START TRANSACTION READ ONLY");
    else
        exec_no_data("START TRANSACTION READ WRITE");
    return unique_ptr<Transaction>(new MySQLTransaction(*this));
}

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
    using namespace dballe::sql::mysql;
    string query = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='" + name + "'";
    Result res(exec_store(query));
    Row row = res.expect_one_result();
    return row.as_unsigned(0) > 0;
}

std::string MySQLConnection::get_setting(const std::string& key)
{
    using namespace dballe::sql::mysql;
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

void MySQLConnection::execute(const std::string& query)
{
    exec_no_data(query);
}

void MySQLConnection::explain(const std::string& query, FILE* out)
{
    using namespace dballe::sql::mysql;
    string explain_query = "EXPLAIN EXTENDED ";
    explain_query += query;

    fprintf(out, "%s\n", explain_query.c_str());
    fprintf(out, "sid\tstype\ttable\ttype\tpos_ks\tkey\tkeylen\tref\trows\textra\n");
    exec_use(explain_query, [&](const Row& row) {
        fprintf(out, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                row.as_cstring(0),
                row.as_cstring(1),
                row.as_cstring(2),
                row.as_cstring(3),
                row.as_cstring(4),
                row.as_cstring(5),
                row.as_cstring(6),
                row.as_cstring(7),
                row.as_cstring(8),
                row.as_cstring(9));

// 0 id: 1
// 1 select_type: PRIMARY
// 2 table: t1
// 3 type: index
// 4 possible_keys: NULL
// 5 key: PRIMARY
// 6 key_len: 4
// 7 ref: NULL
// 8 rows: 4
// 9 Extra: Using index
    });

    fprintf(out, "level\tcode\tmessage\n");
    exec_use("SHOW WARNINGS", [&](const Row& row) {
        fprintf(out, "%s\t%s\t%s\n",
                row.as_cstring(0),
                row.as_cstring(1),
                row.as_cstring(2));
    });
}

}
}
