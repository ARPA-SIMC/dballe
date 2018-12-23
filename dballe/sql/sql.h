/** @file
 * Common infrastructure for talking with SQL databases
 */
#ifndef DBALLE_SQL_H
#define DBALLE_SQL_H

#include <dballe/core/error.h>
#include <dballe/fwd.h>
#include <dballe/sql/fwd.h>
#include <string>
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
class Datetime;

namespace sql {

/** @enum ServerType
 * Supported SQL servers.
 *
 * @cond WORKAROUND
 */
enum class ServerType
{
    MYSQL,
    SQLITE,
    ORACLE,
    POSTGRES,
};
/// @endcond

/**
 * Return a string description for a ServerType value
 */
const char* format_server_type(ServerType type);


class Connection
{
protected:
    std::string url;

public:
    /**
     * Type of SQL server we are connected to.
     *
     * Use this to tell which SQL dialect to use, in case standard SQL
     * behaviour is not enough
     */
    ServerType server_type;

    Connection();
    virtual ~Connection();

    const std::string& get_url() const { return url; }

    /**
     * Begin a transaction.
     *
     * The transaction will be controller by the returned Transaction object,
     * and will end when its destuctor is called.
     */
    virtual std::unique_ptr<Transaction> transaction(bool readonly=false) = 0;

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

    /// Format a datetime and add it to the querybuf
    virtual void add_datetime(Querybuf& qb, const Datetime& dt) const;

    /// Execute a query without reading its results
    virtual void execute(const std::string& query) = 0;

    /// Format and print the EXPLAIN output for the query to the given file
    virtual void explain(const std::string& query, FILE* out) = 0;

    /// Create a new connection from a URL
    static std::unique_ptr<Connection> create(const DBConnectOptions& options);
};

/**
 * A RAII transaction interface for SQL transactions.
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
    Transaction() {}
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    virtual ~Transaction();

    /// Commit this transaction
    virtual void commit() = 0;

    /// Roll back this transaction
    virtual void rollback() = 0;

    /// Roll back this transaction
    virtual void rollback_nothrow() noexcept = 0;

    /// Get an exclusive lock on the given table until the end of the
    /// transaction
    virtual void lock_table(const char* name) = 0;
};

}
}

#endif
