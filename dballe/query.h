#ifndef DBALLE_QUERY_H
#define DBALLE_QUERY_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/var.h>
#include <string>
#include <functional>
#include <memory>

namespace dballe {

/// Query used to filter DB-All.e data
class Query
{
public:
    Query() {}
    Query(const Query&) = default;
    Query(Query&&) = default;
    virtual ~Query() {}

    Query& operator=(const Query& o) = default;
    Query& operator=(Query&& o) = default;

    /// Get the Datetime bounds set in this query
    virtual DatetimeRange get_datetimerange() const = 0;

    /// Set the Datetime range for this query
    virtual void set_datetimerange(const DatetimeRange& dt) = 0;

    /// Get the range of latitudes to be matched
    virtual LatRange get_latrange() const = 0;

    /// Set the range of latitudes to be matched
    virtual void set_latrange(const LatRange& latrange) = 0;

    /// Get the range of longitudes to be matched
    virtual LonRange get_lonrange() const = 0;

    /// Set the range of longitudes to be matched
    virtual void set_lonrange(const LonRange& lonrange) = 0;

    /// Get the level to be matched
    virtual Level get_level() const = 0;

    /// Set the level to be matched
    virtual void set_level(const Level& level) = 0;

    /// Get the time range to be matched
    virtual Trange get_trange() const = 0;

    /// Set the level to be matched
    virtual void set_trange(const Trange& trange) = 0;

    /// Clear the contents of the query, making it match all data
    virtual void clear() = 0;

    /**
     * Return true if this query matches a subset of the given query.
     *
     * In other words, it returns true if this query is the same as \a other,
     * plus zero or more extra fields set, or zero or more ranges narrowed.
     */
    virtual bool is_subquery(const Query& other) const = 0;

#if 0
    /**
     * Generate a sequence of key names and unique_ptr<Var> for all the
     * contents of the query
     */
    virtual void foreach_key(std::function<void(const char*, wreport::Var&&)> dest) const = 0;
#endif

    /// Print the query contents to stderr
    virtual void print(FILE* out) const = 0;

    /// Return a copy of this query
    virtual std::unique_ptr<Query> clone() const = 0;

    /// Create a new Query
    static std::unique_ptr<Query> create();

    /// Check if the query is empty, that is, it queries everything
    virtual bool empty() const = 0;
};

}
#endif
