#ifndef DBALLE_CORE_MATCHER_H
#define DBALLE_CORE_MATCHER_H

#include <dballe/core/defs.h>
#include <memory>

namespace dballe {
struct Record;
struct Query;

namespace matcher {

enum Result {
    MATCH_YES,  // Item matches
    MATCH_NO,   // Item does not match
    MATCH_NA    // Match not applicable to this item
};

}

/**
 * Common interface for things that are matched.
 *
 * This allows the Record-derived matcher to operate on several different
 * elements. Examples are Record and Msg, but can also be unknown elements
 * provided by code that uses DB-All.e.
 */
struct Matched
{
    virtual ~Matched() {}

    /**
     * Match variable ID
     *
     * This corresponds to B33195
     */
    virtual matcher::Result match_var_id(int val) const;

    /**
     * Match station ID
     *
     * This corresponds to DBA_KEY_ANA_ID
     */
    virtual matcher::Result match_station_id(int val) const;

    /**
     * Match station WMO code
     *
     * If station is -1, only match the block.
     */
    virtual matcher::Result match_station_wmo(int block, int station=-1) const;

    /**
     * Match date
     *
     * min and max are arrays of 6 ints (from year to second), and either of
     * them can have -1 as the first element to indicate an open bound.
     */
    virtual matcher::Result match_date(const Datetime& min, const Datetime& max) const;

    /**
     * Match coordinates, with bounds in 1/100000 of degree
     *
     * Any value can be set to MISSING_INT if not applicable or to represent an
     * open bound
     */
    virtual matcher::Result match_coords(const Coords& cmin, const Coords& cmax) const;

    /**
     * Match rep_memo
     *
     * the memo value that is passed is always lowercase
     */
    virtual matcher::Result match_rep_memo(const char* memo) const;

    /**
     * Match if min <= date <= max
     *
     * It correctly deals with min and max having the first element set to -1
     * to signify an open bound.
     */
    static matcher::Result date_in_range(const Datetime& date, const Datetime& min, const Datetime& max);

    /**
     * Match if min <= val <= max
     *
     * It correctly deals with min and max being set to MISSING_INT to signify an open
     * bound.
     */
    static matcher::Result int_in_range(int val, int min, int max);

    /**
     * Match if val is contained inside the given longitude range
     */
    static matcher::Result lon_in_range(int val, int min, int max);
};

/**
 * Match DB-All.e objects using the same queries that can be made on DB-All.e
 * databases.
 */
struct Matcher
{
    virtual ~Matcher() {}

    virtual matcher::Result match(const Matched& item) const = 0;
    virtual void to_record(dballe::Record& query) const = 0;

    static std::unique_ptr<Matcher> create(const dballe::Query& query);
};

}

/* vim:set ts=4 sw=4: */
#endif
