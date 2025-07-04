/** @file
 * PostgreSQL DB connector
 */
#ifndef DBALLE_SQL_POSTGRESQL_H
#define DBALLE_SQL_POSTGRESQL_H

#include <arpa/inet.h>
#include <dballe/sql/sql.h>
#include <functional>
#include <libpq-fe.h>
#include <unordered_set>
#include <vector>

namespace dballe {
namespace sql {

/**
 * Report an PostgreSQL error
 */
struct error_postgresql : public error_db
{
    std::string msg;

    error_postgresql(PGconn* db, const std::string& msg);
    error_postgresql(PGresult* db, const std::string& msg);
    error_postgresql(const std::string& dbmsg, const std::string& msg);
    ~error_postgresql() throw() {}

    const char* what() const noexcept override { return msg.c_str(); }

    static void throwf(PGconn* db, const char* fmt, ...)
        WREPORT_THROWF_ATTRS(2, 3);
    static void throwf(PGresult* db, const char* fmt, ...)
        WREPORT_THROWF_ATTRS(2, 3);
};

namespace postgresql {

int64_t encode_datetime(const Datetime& arg);
int64_t encode_int64_t(int64_t arg);

/// Argument list for PQexecParams built at compile time
template <typename... ARGS> struct Params
{
    static const int count = sizeof...(ARGS);
    const char* args[sizeof...(ARGS)];
    int lengths[sizeof...(ARGS)];
    int formats[sizeof...(ARGS)];
    void* local[sizeof...(ARGS)];

    Params(const ARGS&... args) { _add(0, args...); }
    ~Params()
    {
        for (auto& i : local)
            free(i);
    }

    Params(const Params&)             = delete;
    Params(const Params&&)            = delete;
    Params& operator=(const Params&)  = delete;
    Params& operator=(const Params&&) = delete;

protected:
    /// Terminating condition for compile-time arg expansion
    void _add(unsigned pos) {}

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, std::nullptr_t arg, const REST&... rest)
    {
        local[pos]   = nullptr;
        args[pos]    = nullptr;
        lengths[pos] = 0;
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, int32_t arg, const REST&... rest)
    {
        local[pos]            = malloc(sizeof(int32_t));
        *(int32_t*)local[pos] = (int32_t)htonl((uint32_t)arg);
        args[pos]             = (const char*)local[pos];
        lengths[pos]          = sizeof(int32_t);
        formats[pos]          = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, uint64_t arg, const REST&... rest)
    {
        local[pos]            = malloc(sizeof(int64_t));
        *(int64_t*)local[pos] = encode_int64_t(arg);
        args[pos]             = (const char*)local[pos];
        lengths[pos]          = sizeof(int64_t);
        formats[pos]          = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, const char* arg, const REST&... rest)
    {
        local[pos]   = nullptr;
        args[pos]    = arg;
        lengths[pos] = 0;
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, const std::string& arg, const REST&... rest)
    {
        local[pos]   = nullptr;
        args[pos]    = arg.data();
        lengths[pos] = arg.size();
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, const std::vector<uint8_t>& arg,
              const REST&... rest)
    {
        local[pos]   = nullptr;
        args[pos]    = (const char*)arg.data();
        lengths[pos] = arg.size();
        formats[pos] = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template <typename... REST>
    void _add(unsigned pos, const Datetime& arg, const REST&... rest)
    {
        local[pos]            = malloc(sizeof(int64_t));
        *(int64_t*)local[pos] = encode_datetime(arg);
        args[pos]             = (const char*)local[pos];
        lengths[pos]          = sizeof(int64_t);
        formats[pos]          = 1;
        _add(pos + 1, rest...);
    }
};

/// Wrap a PGresult, taking care of its memory management
struct Result
{
    PGresult* res;

    Result() : res(nullptr) {}
    Result(PGresult* res) : res(res) {}
    ~Result() { PQclear(res); }

    /// Implement move
    Result(Result&& o) : res(o.res) { o.res = nullptr; }
    Result& operator=(Result&& o)
    {
        if (this == &o)
            return *this;
        PQclear(res);
        res   = o.res;
        o.res = nullptr;
        return *this;
    }

    operator bool() const { return res != nullptr; }
    operator PGresult*() { return res; }
    operator const PGresult*() const { return res; }

    /// Check that the result successfully returned no data
    void expect_no_data(const std::string& query);

    /// Check that the result successfully returned some (possibly empty) data
    void expect_result(const std::string& query);

    /// Check that the result successfully returned one row of data
    void expect_one_row(const std::string& query);

    /// Check that the result was successful
    void expect_success(const std::string& query);

    /// Get the number of rows in the result
    unsigned rowcount() const { return PQntuples(res); }

    /// Check if a result value is null
    bool is_null(unsigned row, unsigned col) const
    {
        return PQgetisnull(res, row, col);
    }

    /// Return a result value, transmitted in binary as a byte (?)
    bool get_bool(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return *val;
    }

    /// Return a result value, transmitted in binary as a 2 bit integer
    uint16_t get_int2(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return ntohs(*(uint16_t*)val);
    }

    /// Return a result value, transmitted in binary as a 4 bit integer
    uint32_t get_int4(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return ntohl(*(uint32_t*)val);
    }

    /// Return a result value, transmitted in binary as an 8 bit integer
    uint64_t get_int8(unsigned row, unsigned col) const;

    /// Return a result value, transmitted in binary as an 8 bit integer
    std::vector<uint8_t> get_bytea(unsigned row, unsigned col) const;

    /// Return a result value, transmitted as a string
    const char* get_string(unsigned row, unsigned col) const
    {
        return PQgetvalue(res, row, col);
    }

    /// Return a result value, transmitted as a timestamp without timezone
    Datetime get_timestamp(unsigned row, unsigned col) const;

    // Prevent copy
    Result(const Result&)            = delete;
    Result& operator=(const Result&) = delete;
};

} // namespace postgresql

/// Database connection
class PostgreSQLConnection : public Connection
{
protected:
    /// Database connection
    PGconn* db = nullptr;
    std::unordered_set<std::string> prepared_names;
    /// Marker to catch attempts to reuse connections in forked processes
    bool forked = false;

protected:
    void init_after_connect();

    PostgreSQLConnection();

    void fork_prepare() override;
    void fork_parent() override;
    void fork_child() override;

    void check_connection();

public:
    PostgreSQLConnection(const PostgreSQLConnection&)  = delete;
    PostgreSQLConnection(const PostgreSQLConnection&&) = delete;
    ~PostgreSQLConnection();

    PostgreSQLConnection& operator=(const PostgreSQLConnection&) = delete;

    static std::shared_ptr<PostgreSQLConnection> create();

    operator PGconn*() { return db; }

    /**
     * Connect to PostgreSQL using a connection URI
     *
     * The syntax is described at:
     * http://www.postgresql.org/docs/9.4/static/libpq-connect.html#AEN41094
     */
    void open_url(const std::string& connection_string);
    void open_test();

    std::unique_ptr<Transaction> transaction(bool readonly = false) override;

    /// Precompile a query
    void prepare(const std::string& name, const std::string& query);

    postgresql::Result exec_unchecked(const char* query)
    {
        check_connection();
        auto res =
            PQexecParams(db, query, 0, nullptr, nullptr, nullptr, nullptr, 1);
        if (!res)
            throw error_postgresql(db, std::string("cannot execute query ") +
                                           query);
        return res;
    }

    postgresql::Result exec_unchecked(const std::string& query)
    {
        check_connection();
        auto res = PQexecParams(db, query.c_str(), 0, nullptr, nullptr, nullptr,
                                nullptr, 1);
        if (!res)
            throw error_postgresql(db, "cannot execute query " + query);
        return res;
    }

    template <typename STRING> void exec_no_data(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_no_data(query);
    }

    template <typename STRING> postgresql::Result exec(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_result(query);
        return res;
    }

    template <typename STRING> postgresql::Result exec_one_row(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_one_row(query);
        return res;
    }

    template <typename... ARGS>
    postgresql::Result exec_unchecked(const char* query, ARGS... args)
    {
        check_connection();
        postgresql::Params<ARGS...> params(args...);
        auto res = PQexecParams(db, query, params.count, nullptr, params.args,
                                params.lengths, params.formats, 1);
        if (!res)
            throw error_postgresql(db, std::string("cannot execute query ") +
                                           query);
        return res;
    }

    template <typename... ARGS>
    postgresql::Result exec_unchecked(const std::string& query, ARGS... args)
    {
        check_connection();
        postgresql::Params<ARGS...> params(args...);
        auto res = PQexecParams(db, query.c_str(), params.count, nullptr,
                                params.args, params.lengths, params.formats, 1);
        if (!res)
            throw error_postgresql(db, "cannot execute query " + query);
        return res;
    }

    template <typename STRING, typename... ARGS>
    void exec_no_data(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_no_data(query);
    }

    template <typename STRING, typename... ARGS>
    postgresql::Result exec(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_result(query);
        return res;
    }

    template <typename STRING, typename... ARGS>
    postgresql::Result exec_one_row(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_one_row(query);
        return res;
    }

    postgresql::Result exec_prepared_unchecked(const char* name)
    {
        check_connection();
        auto res = PQexecPrepared(db, name, 0, nullptr, nullptr, nullptr, 1);
        if (!res)
            throw error_postgresql(
                db, std::string("cannot execute prepared query ") + name);
        return res;
    }

    postgresql::Result exec_prepared_unchecked(const std::string& name)
    {
        check_connection();
        auto res =
            PQexecPrepared(db, name.c_str(), 0, nullptr, nullptr, nullptr, 1);
        if (!res)
            throw error_postgresql(db, "cannot execute prepared query " + name);
        return res;
    }

    template <typename STRING> void exec_prepared_no_data(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_no_data(name);
    }

    template <typename STRING> postgresql::Result exec_prepared(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_result(name);
        return res;
    }

    template <typename STRING>
    postgresql::Result exec_prepared_one_row(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_one_row(name);
        return res;
    }

    template <typename... ARGS>
    postgresql::Result exec_prepared_unchecked(const char* name, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecPrepared(db, name, params.count, params.args,
                              params.lengths, params.formats, 1);
    }

    template <typename... ARGS>
    postgresql::Result exec_prepared_unchecked(const std::string& name,
                                               ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecPrepared(db, name.c_str(), params.count, params.args,
                              params.lengths, params.formats, 1);
    }

    template <typename STRING, typename... ARGS>
    void exec_prepared_no_data(STRING name, ARGS... args)
    {
        postgresql::Result res(exec_prepared_unchecked(name, args...));
        res.expect_no_data(name);
    }

    template <typename STRING, typename... ARGS>
    postgresql::Result exec_prepared(STRING name, ARGS... args)
    {
        postgresql::Result res(exec_prepared_unchecked(name, args...));
        res.expect_result(name);
        return res;
    }

    template <typename STRING, typename... ARGS>
    postgresql::Result exec_prepared_one_row(STRING name, ARGS... args)
    {
        postgresql::Result res(exec_prepared_unchecked(name, args...));
        res.expect_one_row(name);
        return res;
    }

    /// Send a cancellation command to the server
    void cancel_running_query_nothrow() noexcept;

    /// Discard all input from an asynchronous request
    void discard_all_input_nothrow() noexcept;

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

    /// Count the number of rows modified by the last query that was run
    int changes();

    /// Wrap PQexec
    void pqexec(const std::string& query);

    /**
     * Wrap PQexec but do not throw an exception in case of errors.
     *
     * This is useful to be called in destructors. Errors will be printed to
     * stderr.
     */
    void pqexec_nothrow(const std::string& query) noexcept;

    /// Retrieve query results in single row mode
    void
    run_single_row_mode(const std::string& query_desc,
                        std::function<void(const postgresql::Result&)> dest);

    /// Escape the string as a literal value and append it to qb
    void append_escaped(Querybuf& qb, const char* str);

    /// Escape the string as a literal value and append it to qb
    void append_escaped(Querybuf& qb, const std::string& str);

    /// Escape the buffer as bytea literal and append it to qb
    void append_escaped(Querybuf& qb, const std::vector<uint8_t>& buf);
};

} // namespace sql
} // namespace dballe
#endif
