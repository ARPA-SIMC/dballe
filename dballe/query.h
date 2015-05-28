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

    /**
     * Set this query to match the given datetime.
     *
     * If part of dt are unset, the query will match a range from the minimum
     * and maximum possible values of dt.
     */
    virtual void set_datetime_exact(const Datetime& dt) = 0;

    /**
     * Set this query to match the given datetime interval.
     *
     * The exact interval will go from dtmin.lower_bound() to
     * dtmax.upper_bound() inclusive.
     *
     * If dtmin, dtmax, or both, are unset, then the interval is considered
     * open on the corresponding side.
     */
    virtual void set_datetime_bounds(const Datetime& dtmin, const Datetime& dtmax) = 0;

    /// Clear the contents of the query, making it match all data
    virtual void clear() = 0;

    /**
     * Set a key to an integer value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the integer value represents the units of maximum precision of the
     * field. For example, using seti to set lat to 4500000 is the same as
     * setting it to 45.0.
     */
    virtual void seti(const char* key, int val) = 0;

    /**
     * Set a key to a double value.
     */
    virtual void setd(const char* key, double val) = 0;

    /**
     * Set a key to a string value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the string is converted to an integer value representing the units of
     * maximum precision of the field. For example, using seti to set lat to
     * "4500000" is the same as setting it to 45.0.
     */
    virtual void setc(const char* key, const char* val) = 0;

    /**
     * Set a key to a string value.
     *
     * If the key that is being set has a decimal component (like lat and lon),
     * the string is converted to an integer value representing the units of
     * maximum precision of the field. For example, using seti to set lat to
     * "4500000" is the same as setting it to 45.0.
     */
    virtual void sets(const char* key, const std::string& val) = 0;

    /**
     * Set a key to a string value.
     *
     * Contrarily to setc, the string is parsed according to the natural
     * representation for the given key. For example, if lat is set to "45",
     * then it gets the value 45.0.
     */
    virtual void setf(const char* key, const char* val) = 0;

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
     * Generate a sequence of key names and unique_ptr<Var> for all the
     * contents of the query
     */
    virtual void to_vars(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const = 0;

    /// Print the query contents to stderr
    virtual void print(FILE* out) const = 0;

    /// Return a copy of this query
    virtual std::unique_ptr<Query> clone() const = 0;

    /// Create a new Query
    static std::unique_ptr<Query> create();
};

}
#endif
