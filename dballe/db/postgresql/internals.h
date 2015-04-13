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

#ifndef DBALLE_DB_POSTGRESQL_INTERNALS_H
#define DBALLE_DB_POSTGRESQL_INTERNALS_H

#include <dballe/db/db.h>
#include <dballe/db/sql.h>
#include <libpq-fe.h>
#include <arpa/inet.h>

namespace dballe {
namespace db {

/**
 * Report an PostgreSQL error
 */
struct error_postgresql : public db::error
{
    std::string msg;

    /**
     * Copy informations from the ODBC diagnostic record to the dba error
     * report
     */
    error_postgresql(PGconn* db, const std::string& msg);
    error_postgresql(PGresult* db, const std::string& msg);
    error_postgresql(const std::string& dbmsg, const std::string& msg);
    ~error_postgresql() throw () {}

    wreport::ErrorCode code() const throw () { return wreport::WR_ERR_ODBC; }

    virtual const char* what() const throw () { return msg.c_str(); }

    static void throwf(PGconn* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
    static void throwf(PGresult* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
};

namespace postgresql {

int64_t encode_datetime(const Datetime& arg);
int64_t encode_int64_t(int64_t arg);

/// Argument list for PQexecParams built at compile time
template<typename... ARGS> struct Params
{
    int count = sizeof...(ARGS);
    const char* args[sizeof...(ARGS)];
    int lengths[sizeof...(ARGS)];
    int formats[sizeof...(ARGS)];
    void* local[sizeof...(ARGS)];

    Params(ARGS... args)
    {
        _add(0, args...);
    }
    ~Params()
    {
        for (auto& i: local)
            free(i);
    }

    Params(const Params&) = delete;
    Params(const Params&&) = delete;
    Params& operator=(const Params&) = delete;
    Params& operator=(const Params&&) = delete;

protected:
    /// Terminating condition for compile-time arg expansion
    void _add(unsigned pos)
    {
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, std::nullptr_t arg, REST... rest)
    {
        local[pos] = nullptr;
        args[pos] = nullptr;
        lengths[pos] = 0;
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, int32_t arg, REST... rest)
    {
        local[pos] = malloc(sizeof(int32_t));
        *(int32_t*)local[pos] = (int32_t)htonl((uint32_t)arg);
        args[pos] = (const char*)local[pos];
        lengths[pos] = sizeof(int32_t);
        formats[pos] = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, uint64_t arg, REST... rest)
    {
        local[pos] = malloc(sizeof(int64_t));
        *(int64_t*)local[pos] = encode_int64_t(arg);
        args[pos] = (const char*)local[pos];
        lengths[pos] = sizeof(int64_t);
        formats[pos] = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, const char* arg, REST... rest)
    {
        local[pos] = nullptr;
        args[pos] = arg;
        lengths[pos] = 0;
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, const std::string& arg, REST... rest)
    {
        local[pos] = nullptr;
        args[pos] = arg.data();
        lengths[pos] = arg.size();
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, const Datetime& arg, REST... rest)
    {
        local[pos] = malloc(sizeof(int64_t));
        *(int64_t*)local[pos] = encode_datetime(arg);
        args[pos] = (const char*)local[pos];
        lengths[pos] = sizeof(int64_t);
        formats[pos] = 1;
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
        if (this == &o) return *this;
        PQclear(res);
        res = o.res;
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

    /// Return a result value, transmitted as a string
    const char* get_string(unsigned row, unsigned col) const
    {
        return PQgetvalue(res, row, col);
    }

    /// Return a result value, transmitted as a timestamp without timezone
    Datetime get_timestamp(unsigned row, unsigned col) const;

    // Prevent copy
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
};

}


/// Database connection
class PostgreSQLConnection : public Connection
{
protected:
    /// Database connection
    PGconn* db = nullptr;

protected:
    void impl_exec_void(const std::string& query) override;
    void init_after_connect();

public:
    PostgreSQLConnection();
    PostgreSQLConnection(const PostgreSQLConnection&) = delete;
    PostgreSQLConnection(const PostgreSQLConnection&&) = delete;
    ~PostgreSQLConnection();

    PostgreSQLConnection& operator=(const PostgreSQLConnection&) = delete;

    operator PGconn*() { return db; }

    void open_url(const std::string& connection_string);
    void open_test();

    std::unique_ptr<Transaction> transaction() override;

    /// Precompile a query
    void prepare(const std::string& name, const std::string& query);

    postgresql::Result exec_unchecked(const char* query)
    {
        return PQexecParams(db, query, 0, nullptr, nullptr, nullptr, nullptr, 1);
    }

    postgresql::Result exec_unchecked(const std::string& query)
    {
        return PQexecParams(db, query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
    }

    template<typename STRING>
    void exec_no_data(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_no_data(query);
    }

    template<typename STRING>
    postgresql::Result exec(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_result(query);
        return res;
    }

    template<typename STRING>
    postgresql::Result exec_one_row(STRING query)
    {
        postgresql::Result res(exec_unchecked(query));
        res.expect_one_row(query);
        return res;
    }

    template<typename ...ARGS>
    postgresql::Result exec_unchecked(const char* query, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecParams(db, query, params.count, nullptr, params.args, params.lengths, params.formats, 1);
    }

    template<typename ...ARGS>
    postgresql::Result exec_unchecked(const std::string& query, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecParams(db, query.c_str(), params.count, nullptr, params.args, params.lengths, params.formats, 1);
    }

    template<typename STRING, typename ...ARGS>
    void exec_no_data(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_no_data(query);
    }

    template<typename STRING, typename ...ARGS>
    postgresql::Result exec(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_result(query);
        return res;
    }

    template<typename STRING, typename ...ARGS>
    postgresql::Result exec_one_row(STRING query, ARGS... args)
    {
        postgresql::Result res(exec_unchecked(query, args...));
        res.expect_one_row(query);
        return res;
    }

    postgresql::Result exec_prepared_unchecked(const char* name)
    {
        return PQexecPrepared(db, name, 0, nullptr, nullptr, nullptr, 1);
    }

    postgresql::Result exec_prepared_unchecked(const std::string& name)
    {
        return PQexecPrepared(db, name.c_str(), 0, nullptr, nullptr, nullptr, 1);
    }

    template<typename STRING>
    void exec_prepared_no_data(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_no_data(name);
    }

    template<typename STRING>
    postgresql::Result exec_prepared(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_result(name);
        return res;
    }

    template<typename STRING>
    postgresql::Result exec_prepared_one_row(STRING name)
    {
        postgresql::Result res(exec_prepared_unchecked(name));
        res.expect_one_row(name);
        return res;
    }

    template<typename ...ARGS>
    postgresql::Result exec_prepared_unchecked(const char* name, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecPrepared(db, name, params.count, params.args, params.lengths, params.formats, 1);
    }

    template<typename ...ARGS>
    postgresql::Result exec_prepared_unchecked(const std::string& name, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        return PQexecPrepared(db, name.c_str(), params.count, params.args, params.lengths, params.formats, 1);
    }

    template<typename STRING, typename ...ARGS>
    void exec_prepared_no_data(STRING name, ARGS... args)
    {
        postgresql::Result res(exec_prepared_unchecked(name, args...));
        res.expect_no_data(name);
    }

    template<typename STRING, typename ...ARGS>
    postgresql::Result exec_prepared(STRING name, ARGS... args)
    {
        postgresql::Result res(exec_prepared_unchecked(name, args...));
        res.expect_result(name);
        return res;
    }

    template<typename STRING, typename ...ARGS>
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

    /// Count the number of rows modified by the last query that was run
    int changes();

    void add_datetime(Querybuf& qb, const int* dt) const override;

    /// Wrap PQexec
    void pqexec(const std::string& query);
    /**
     * Wrap PQexec but do not throw an exception in case of errors.
     *
     * This is useful to be called in destructors. Errors will be printed to
     * stderr.
     */
    void pqexec_nothrow(const std::string& query) noexcept;
};

}
}
#endif

