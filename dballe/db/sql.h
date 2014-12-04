/*
 * db/sql - Generic infrastructure for talking with SQL databases
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_INTERNALS_H
#define DBALLE_DB_INTERNALS_H

#include <wreport/error.h>
#include <memory>

/// Define this to enable referential integrity
#undef USE_REF_INT

/** Trace macros internally used for debugging
 * @{
 */

// #define TRACE_DB

#ifdef TRACE_DB
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
/** Ouput a trace message */
#define TRACE(...) do { } while (0)
/** Prefix a block of code to compile only if trace is enabled */
#define IFTRACE if (0)
#endif

/** @} */

namespace dballe {
class Querybuf;

namespace db {
class Transaction;
class Statement;

/**
 * Supported SQL servers.
 */
enum ServerType
{
    MYSQL,
    SQLITE,
    ORACLE,
    POSTGRES,
};

struct Connection
{
    /**
     * Type of SQL server we are connected to.
     *
     * Use this to tell which SQL dialect to use, in case standard SQL
     * behaviour is not enough
     */
    enum ServerType server_type;

    virtual ~Connection();

    /**
     * Begin a transaction.
     *
     * The transaction will be controller by the returned Transaction object,
     * and will end when its destuctor is called.
     */
    virtual std::unique_ptr<Transaction> transaction() = 0;

    /**
     * Create a statement.
     *
     * A statement holds a precompiled query, and input/output bind arguments.
     */
    virtual std::unique_ptr<Statement> statement() = 0;

    /// Execute a one-shot query
    virtual void exec(const std::string& query) = 0;

    /// Check if the database contains a table
    virtual bool has_table(const std::string& name) = 0;

    /**
     * Get a value from the settings table.
     *
     * Returns the empty string if the table does not exist.
     */
    virtual std::string get_setting(const std::string& key) = 0;

    /**
     * Set a value in the settings table.
     *
     * The table is created if it does not exist.
     */
    virtual void set_setting(const std::string& key, const std::string& value) = 0;

    /// Drop the settings table
    virtual void drop_settings() = 0;

    /**
     * Delete a table in the database if it exists, otherwise do nothing.
     */
    virtual void drop_table_if_exists(const char* name) = 0;

    /**
     * Delete a sequence in the database if it exists, otherwise do nothing.
     */
    virtual void drop_sequence_if_exists(const char* name) = 0;

    /**
     * Return LAST_INSERT_ID or LAST_INSER_ROWID or whatever is appropriate for
     * the current database, if supported.
     *
     * If not supported, an exception is thrown.
     */
    virtual int get_last_insert_id() = 0;

    /// Format a datetime and add it to the querybuf
    virtual void add_datetime(Querybuf& qb, const int* dt) const = 0;
};

/**
 * A RAII transaction interface.
 *
 * The transaction will be valid during the lifetime of this object.
 *
 * You can commit or rollback the transaction using its methods. If at
 * destruction time the transaction has not been committed or rolled back, a
 * rollback is automatically performed.
 */
class Transaction
{
public:
    virtual ~Transaction() {}

    /// Commit this transaction
    virtual void commit() = 0;

    /// Roll back this transaction
    virtual void rollback() = 0;
};

/**
 * A prepared SQL statement
 */
class Statement
{
public:
    virtual ~Statement() {}

    /// Compile the SQL statement
    virtual void prepare(const std::string& query) = 0;

    /// Run the statement ignoring its results
    virtual void execute_ignoring_results() = 0;

    virtual void bind_in(int idx, const int& val) = 0;
    virtual void bind_in(int idx, const unsigned& val) = 0;
    virtual void bind_in(int idx, const unsigned short& val) = 0;
    virtual void bind_in(int idx, const char* val) = 0;
};

}
}

#endif
