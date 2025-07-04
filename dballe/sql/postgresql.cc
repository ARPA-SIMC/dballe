#include "postgresql.h"
#include "dballe/types.h"
#include "querybuf.h"
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <unistd.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

namespace postgresql {

// From http://libpqtypes.esilo.com/browse_source.html?file=datetime.c
static const int EPOCH_JDATE = 2451545; // == 2000-01-01

int64_t encode_int64_t(int64_t arg) { return htobe64(arg); }

int64_t encode_datetime(const Datetime& arg)
{
    int64_t encoded = arg.to_julian() - EPOCH_JDATE;
    encoded *= 86400;
    encoded += arg.hour * 3600 + arg.minute * 60 + arg.second;
    encoded *= 1000000;
    return (int64_t)htobe64(encoded);
}

void Result::expect_no_data(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_COMMAND_OK: break;
        case PGRES_TUPLES_OK:
            throw error_postgresql("tuples_ok",
                                   "data (possibly an empty set) returned by " +
                                       query);
        default: throw error_postgresql(res, "executing " + query);
    }
}

void Result::expect_result(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK: break;
        case PGRES_COMMAND_OK:
            throw error_postgresql("command_ok",
                                   "no data returned by " + query);
        default: throw error_postgresql(res, "executing " + query);
    }
}

void Result::expect_one_row(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK: break;
        case PGRES_COMMAND_OK:
            throw error_postgresql("command_ok",
                                   "no data returned by " + query);
        default: throw error_postgresql(res, "executing " + query);
    }
    unsigned rows = rowcount();
    if (rows != 1)
        error_consistency::throwf("Got %u results instead of 1 when running %s",
                                  rows, query.c_str());
}

void Result::expect_success(const std::string& query)
{
    switch (PQresultStatus(res))
    {
        case PGRES_TUPLES_OK:
        case PGRES_COMMAND_OK: break;
        default:               throw error_postgresql(res, "executing " + query);
    }
}

/// Return a result value, transmitted in binary as a 4 bit integer
uint64_t Result::get_int8(unsigned row, unsigned col) const
{
    char* val = PQgetvalue(res, row, col);
    return be64toh(*(uint64_t*)val);
}

std::vector<uint8_t> Result::get_bytea(unsigned row, unsigned col) const
{
    int size  = PQgetlength(res, row, col);
    char* val = PQgetvalue(res, row, col);
    return std::vector<uint8_t>(val, val + size);
}

Datetime Result::get_timestamp(unsigned row, unsigned col) const
{
    // Adapter from
    // http://libpqtypes.esilo.com/browse_source.html?file=datetime.c

    // Decode from big endian
    int64_t decoded = be64toh(*(uint64_t*)PQgetvalue(res, row, col));

    // Convert from microseconds to seconds
    decoded = decoded / 1000000;

    // Split date and time
    int time  = decoded % 86400;
    int jdate = decoded / 86400;
    if (time < 0)
    {
        time += 86400;
        jdate -= 1;
    }

    // Decode date
    jdate += EPOCH_JDATE;

    // Decode time
    return Datetime::from_julian(jdate, time / 3600, (time / 60) % 60,
                                 time % 60);
}

} // namespace postgresql

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

error_postgresql::error_postgresql(const std::string& dbmsg,
                                   const std::string& msg)
{
    this->msg = msg;
    this->msg += ": ";
    this->msg += dbmsg;
}

void error_postgresql::throwf(PGconn* db, const char* fmt, ...)
{
    char buf[512];

    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    throw error_postgresql(db, buf);
}

void error_postgresql::throwf(PGresult* res, const char* fmt, ...)
{
    char buf[512];

    // Format the arguments
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 512, fmt, ap);
    va_end(ap);

    throw error_postgresql(res, buf);
}

PostgreSQLConnection::PostgreSQLConnection() {}

PostgreSQLConnection::~PostgreSQLConnection()
{
    if (db)
        PQfinish(db);
}

std::shared_ptr<PostgreSQLConnection> PostgreSQLConnection::create()
{
    auto res = std::shared_ptr<PostgreSQLConnection>(new PostgreSQLConnection);
    res->register_atfork();
    return res;
}

void PostgreSQLConnection::fork_prepare() {}

void PostgreSQLConnection::fork_parent() {}

void PostgreSQLConnection::fork_child()
{
    if (db)
    {
        // Close socket to avoid sending close commands to the server that
        // would interfere with the parent
        ::close(PQsocket(db));
        PQfinish(db);
        db     = nullptr;
        forked = true;
    }
}

void PostgreSQLConnection::check_connection()
{
    if (forked)
        throw error_postgresql(
            "server connection closed",
            "database connections cannot be used after forking");
}

void PostgreSQLConnection::open_url(const std::string& connection_string)
{
    url = connection_string;
    db  = PQconnectdb(connection_string.c_str());
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
    // Hide warning notices, like "table does not exists" in "DROP TABLE ... IF
    // EXISTS"
    exec_no_data("SET client_min_messages = error");
}

void PostgreSQLConnection::pqexec(const std::string& query)
{
    check_connection();
    postgresql::Result res(PQexec(db, query.c_str()));
    res.expect_success(query);
}

void PostgreSQLConnection::pqexec_nothrow(const std::string& query) noexcept
{
    if (forked)
        return;

    postgresql::Result res(PQexec(db, query.c_str()));
    switch (PQresultStatus(res))
    {
        case PGRES_COMMAND_OK:
        case PGRES_TUPLES_OK:  return;
        default:
            fprintf(stderr, "postgresql cleanup: cannot execute '%s': %s\n",
                    query.c_str(), PQresultErrorMessage(res));
    }
}

struct PostgreSQLTransaction : public Transaction
{
    PostgreSQLConnection& conn;
    bool fired = false;

    PostgreSQLTransaction(PostgreSQLConnection& conn) : conn(conn) {}
    ~PostgreSQLTransaction()
    {
        if (!fired)
            rollback_nothrow();
    }

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

std::unique_ptr<Transaction> PostgreSQLConnection::transaction(bool readonly)
{
    // The default PostgreSQL isolation level is READ COMMITTED
    // https://www.postgresql.org/docs/9.2/static/sql-set-transaction.html
    if (readonly)
        exec_no_data("BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY");
    else
        exec_no_data("BEGIN ISOLATION LEVEL REPEATABLE READ READ WRITE");
    return unique_ptr<Transaction>(new PostgreSQLTransaction(*this));
}

void PostgreSQLConnection::prepare(const std::string& name,
                                   const std::string& query)
{
    using namespace postgresql;
    if (prepared_names.find(name) != prepared_names.end())
        return;
    Result res(PQprepare(db, name.c_str(), query.c_str(), 0, nullptr));
    res.expect_no_data("prepare:" + query);
    prepared_names.insert(name);
}

void PostgreSQLConnection::drop_table_if_exists(const char* name)
{
    exec_no_data(string("DROP TABLE IF EXISTS ") + name + " CASCADE");
}

void PostgreSQLConnection::cancel_running_query_nothrow() noexcept
{
    PGcancel* c = PQgetCancel(db);
    if (!c)
        return;

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
        if (!res)
            break;
        switch (PQresultStatus(res))
        {
            case PGRES_TUPLES_OK: break;
            case PGRES_COMMAND_OK:
                fprintf(stderr, "flushing input from PostgreSQL server "
                                "returned an empty result\n");
                break;
            default:
                fprintf(stderr,
                        "flushing input from PostgreSQL server returned an "
                        "error: %s\n",
                        PQresultErrorMessage(res));
                break;
        }
    }
}

void PostgreSQLConnection::run_single_row_mode(
    const std::string& query_desc,
    std::function<void(const postgresql::Result&)> dest)
{
    using namespace dballe::sql::postgresql;

    // http://www.postgresql.org/docs/9.4/static/libpq-single-row-mode.html
    if (!PQsetSingleRowMode(db))
    {
        string errmsg(PQerrorMessage(db));
        cancel_running_query_nothrow();
        discard_all_input_nothrow();
        throw error_postgresql(errmsg, "cannot set single row mode for query " +
                                           query_desc);
    }

    while (true)
    {
        Result res(PQgetResult(db));
        if (!res)
            break;

        // Note: Even when PQresultStatus indicates a fatal error, PQgetResult
        // should be called until it returns a null pointer to allow libpq to
        // process the error information completely.
        //  (http://www.postgresql.org/docs/9.1/static/libpq-async.html)

        // If we get what we don't want, cancel, flush our input and throw
        if (PQresultStatus(res) == PGRES_SINGLE_TUPLE)
        {
            // Ok, we have a tuple
        }
        else if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            // No more rows will arrive
            continue;
        }
        else
        {
            // An error arrived
            cancel_running_query_nothrow();
            discard_all_input_nothrow();

            switch (PQresultStatus(res))
            {
                case PGRES_COMMAND_OK:
                    throw error_postgresql("command_ok",
                                           "no data returned by query " +
                                               query_desc);
                default: throw error_postgresql(res, "executing " + query_desc);
            }
        }

        try
        {
            dest(res);
        }
        catch (std::exception& e)
        {
            // If we get an exception from downstream, cancel, flush all
            // input and rethrow it
            cancel_running_query_nothrow();
            discard_all_input_nothrow();
            throw;
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

    const char* query =
        "SELECT value FROM dballe_settings WHERE \"key\"=$1::text";
    Result res = exec(query, key);
    if (res.rowcount() == 0)
        return string();
    if (res.rowcount() > 1)
        error_consistency::throwf("got %d results instead of 1 executing %s",
                                  res.rowcount(), query);
    return res.get_string(0, 0);
}

void PostgreSQLConnection::set_setting(const std::string& key,
                                       const std::string& value)
{
    if (!has_table("dballe_settings"))
        exec_no_data("CREATE TABLE dballe_settings (\"key\" TEXT NOT NULL "
                     "PRIMARY KEY, value TEXT NOT NULL)");

    auto trans = transaction();
    exec_no_data("LOCK TABLE dballe_settings IN EXCLUSIVE MODE");
    auto s = exec_one_row(
        "SELECT EXISTS (SELECT 1 FROM dballe_settings WHERE \"key\"=$1::text)",
        key);
    if (s.get_bool(0, 0))
        exec_no_data(
            "UPDATE dballe_settings SET value=$2::text WHERE \"key\"=$1::text",
            key, value);
    else
        exec_no_data("INSERT INTO dballe_settings (\"key\", value) VALUES "
                     "($1::text, $2::text)",
                     key, value);
    trans->commit();
}

void PostgreSQLConnection::drop_settings()
{
    drop_table_if_exists("dballe_settings");
}

int PostgreSQLConnection::changes()
{
    throw error_unimplemented("changes");
    // return sqlite3_changes(db);
}

void PostgreSQLConnection::execute(const std::string& query)
{
    exec_no_data(query);
}

void PostgreSQLConnection::explain(const std::string& query, FILE* out)
{
    using namespace dballe::sql::postgresql;

    string explain_query = "EXPLAIN ";
    explain_query += query;

    fprintf(out, "%s\n", explain_query.c_str());
    Result res = exec(explain_query);
    for (unsigned row = 0; row < res.rowcount(); ++row)
        fprintf(out, "  %s\n", res.get_string(row, 0));
}

void PostgreSQLConnection::append_escaped(Querybuf& qb, const char* str)
{
    char* escaped = PQescapeLiteral(db, str, strlen(str));
    if (!escaped)
        error_postgresql::throwf(db, "cannot escape string '%s'", str);
    qb.append(escaped);
    PQfreemem(escaped);
}

void PostgreSQLConnection::append_escaped(Querybuf& qb, const std::string& str)
{
    char* escaped = PQescapeLiteral(db, str.data(), str.size());
    if (!escaped)
        throw error_postgresql(db, "cannot escape string '" + str + "'");
    qb.append(escaped);
    PQfreemem(escaped);
}

void PostgreSQLConnection::append_escaped(Querybuf& qb,
                                          const std::vector<uint8_t>& buf)
{
    size_t len;
    char* escaped = (char*)PQescapeByteaConn(db, buf.data(), buf.size(), &len);
    qb.append("\'");
    qb.append(escaped, len - 1);
    qb.append("\'");
    PQfreemem(escaped);
}

} // namespace sql
} // namespace dballe
