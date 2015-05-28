#ifndef DBALLE_QUERY_H
#define DBALLE_QUERY_H

#include <wreport/var.h>
#include <dballe/types.h>
#include <string>
#include <functional>
#include <memory>

namespace dballe {

/// Query used to filter DB-All.e data
struct Query
{
    virtual ~Query() {}

    /**
     * Get the Datetime bounds set in this query.
     *
     * dtmin and dtmax could be set to missing if the query has no minimum or
     * maximum bound.
     * dtmin and dtmax could both be set to the same value if the query matches
     * an exact datetime.
     */
    virtual void get_datetime_bounds(Datetime& dtmin, Datetime& dtmax) const = 0;

    /// Clear the contents of the query, making it match all data
    virtual void clear() = 0;

    virtual void seti(const char* key, int val) = 0;
    virtual void setd(const char* key, double val) = 0;
    virtual void setc(const char* key, const char* val) = 0;
    virtual void sets(const char* key, const std::string& val) = 0;

    void set(const char* key, int val) { seti(key, val); }
    void set(const char* key, double val) { setd(key, val); }
    void set(const char* key, const char* val) { setc(key, val); }
    void set(const char* key, const std::string& val) { sets(key, val); }

    virtual void unset(const char* key) = 0;

    /**
     * Return true if this query matches a subset of the given query.
     *
     * In other words, it returns true if this query is the same as \a other,
     * plus zero or more extra fields set, or zero or more ranges narrowed.
     */
    virtual bool is_subquery(const Query& other) const = 0;

    /**
     * Generate a sequence of dba_keyword and unique_ptr<Var> for all contents
     * of the query that can be represented in a record.
     */
    virtual void to_vars(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const = 0;

    /// Print the query contents to stderr
    virtual void print(FILE* out) const = 0;

    /// Create a new Query
    static std::unique_ptr<Query> create();
};

}
#endif
