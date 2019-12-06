#ifndef DBALLE_CORE_QUERY_H
#define DBALLE_CORE_QUERY_H

#include <dballe/query.h>
#include <dballe/core/defs.h>
#include <dballe/core/fwd.h>
#include <wreport/varinfo.h>
#include <set>

/**
 * Values for query modifier flags
 */
/** When values from different reports exist on the same point, only report the
 * one from the report with the highest priority */
#define DBA_DB_MODIFIER_BEST        (1 << 0)
/** Do not bother sorting the results */
#define DBA_DB_MODIFIER_UNSORTED    (1 << 5)
/** Sort by report after ana_id, to ease reconstructing messages on export */
#define DBA_DB_MODIFIER_SORT_FOR_EXPORT (1 << 7)
/// Add minimum date, maximum date and data count details to summary query results
#define DBA_DB_MODIFIER_SUMMARY_DETAILS (1 << 8)
/// Also get attributes alongside data
#define DBA_DB_MODIFIER_WITH_ATTRIBUTES (1 << 9)

namespace dballe {
namespace core {

struct JSONWriter;

/// Standard dballe::Query implementation
struct Query : public dballe::Query
{
    static const uint32_t WANT_MISSING_IDENT =  (1 << 0);
    static const uint32_t WANT_MISSING_LTYPE1 = (1 << 1);
    static const uint32_t WANT_MISSING_L1 =     (1 << 2);
    static const uint32_t WANT_MISSING_LTYPE2 = (1 << 3);
    static const uint32_t WANT_MISSING_L2 =     (1 << 4);
    static const uint32_t WANT_MISSING_PIND =   (1 << 5);
    static const uint32_t WANT_MISSING_P1 =     (1 << 6);
    static const uint32_t WANT_MISSING_P2 =     (1 << 7);

    /**
     * Set a bit a 1 with WANT_MISSING_* constants to specify that the query
     * wants results in which the corresponding field is set to a missing
     * value.
     */
    uint32_t want_missing = 0;
    int ana_id = MISSING_INT;
    int priomin = MISSING_INT;
    int priomax = MISSING_INT;
    std::string report;
    int mobile = MISSING_INT;
    Ident ident;
    LatRange latrange;
    LonRange lonrange;
    DatetimeRange dtrange;
    Level level;
    Trange trange;
    std::set<wreport::Varcode> varcodes;
    std::string query;
    std::string ana_filter;
    std::string data_filter;
    std::string attr_filter;
    int limit = MISSING_INT;
    int block = MISSING_INT;
    int station = MISSING_INT;

    bool operator==(const Query& o) const
    {
        return std::tie(want_missing, ana_id, priomin, priomax, report, mobile, ident, latrange, lonrange, dtrange, level, trange, varcodes, query, ana_filter, data_filter, attr_filter, limit, block, station)
            == std::tie(o.want_missing, o.ana_id, o.priomin, o.priomax, o.report, o.mobile, o.ident, o.latrange, o.lonrange, o.dtrange, o.level, o.trange, o.varcodes, o.query, o.ana_filter, o.data_filter, o.attr_filter, o.limit, o.block, o.station);
    }

    /**
     * Check the query fields for consistency, and fill in missing values:
     *
     *  - month without year, day without month, and so on, cause errors
     *  - only one longitude extreme without the other causes error
     *  - min and max datetimes are filled with the actual minimum and maximum
     *    values acceptable for that range (year=2017, for example, becomes
     *    min=2017-01-01 00:00:00, max=2017-12-31 23:59:59
     */
    void validate();

    std::unique_ptr<dballe::Query> clone() const override;

    unsigned get_modifiers() const;

    DatetimeRange get_datetimerange() const override { return dtrange; }
    void set_datetimerange(const DatetimeRange& dt) override { dtrange = dt; }
    LatRange get_latrange() const override { return latrange; }
    void set_latrange(const LatRange& lr) override { latrange = lr; }
    LonRange get_lonrange() const override { return lonrange; }
    void set_lonrange(const LonRange& lr) override { lonrange = lr; }
    Level get_level() const override { return level; }
    void set_level(const Level& level) override { this->level = level; }
    Trange get_trange() const override { return trange; }
    void set_trange(const Trange& trange) override { this->trange = trange; }

    void clear() override;

    /**
     * Set a value in the record according to an assignment encoded in a string.
     *
     * String can use keywords, aliases and varcodes.  Examples: ana_id=3,
     * name=Bologna, B12012=32.4
     *
     * In case of numeric parameter, a hyphen ("-") means MISSING_INT (e.g.,
     * `leveltype2=-`).
     *
     * @param str
     *   The string containing the assignment.
     */
    void set_from_string(const char* str);

    /**
     * Set a record from a ", "-separated string of assignments.
     *
     * The implementation is not efficient and the method is not safe for any
     * input, since ", " could appear in a station identifier. It is however
     * useful to quickly create test queries for unit testing.
     */
    void set_from_test_string(const std::string& s);

    /**
     * Return true if this query matches a subset of the given query.
     *
     * In other words, it returns true if this query is the same as \a other,
     * plus zero or more extra fields set, or zero or more ranges narrowed.
     */
    bool is_subquery(const dballe::Query& other) const override;

    /// Print the query contents to stderr
    void print(FILE* out) const override;

    /// Send the contents to a JSONWriter
    void serialize(JSONWriter& out) const;

    /**
     * Parse the modifiers specification given a query=* string, returning the ORed
     * flags.
     */
    static unsigned parse_modifiers(const char* str);

    /**
     * Return a reference to query downcasted as core::Query.
     *
     * Throws an exception if query is not a core::Query.
     */
    static const Query& downcast(const dballe::Query& query);

    /**
     * Return a reference to query downcasted as core::Query.
     *
     * Throws an exception if query is not a core::Query.
     */
    static Query& downcast(dballe::Query& query);

    static Query from_json(core::json::Stream& in);

protected:
    void setf(const char* key, unsigned len, const char* val);
    void unset(const char* key, unsigned len);
};

}
}
#endif
