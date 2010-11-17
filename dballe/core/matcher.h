/*
 * dballe/matcher - Local query match infrastructure
 *
 * Copyright (C) 2009--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBALLE_CORE_MATCHER_H
#define DBALLE_CORE_MATCHER_H

#include <memory>

namespace dballe {
struct Record;

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
    virtual matcher::Result match_date(const int* min, const int* max) const;

    /**
     * Match coordinates, with bounds in 1/10000 of degree
     *
     * Any value can be set to MISSING_INT if not applicable or to represent an
     * open bound
     */
    virtual matcher::Result match_coords(int latmin, int latmax, int lonmin, int lonmax) const;

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
    static matcher::Result date_in_range(const int* date, const int* min, const int* max);

    /**
     * Match if min <= val <= max
     *
     * It correctly deals with min and max being set to MISSING_INT to signify an open
     * bound.
     */
    static matcher::Result int_in_range(int val, int min, int max);
};

struct Matcher
{
    virtual ~Matcher() {}

    virtual matcher::Result match(const Matched& item) const = 0;
    virtual void to_record(dballe::Record& query) const = 0;

    static std::auto_ptr<Matcher> create(const dballe::Record& query);
};

}

/* vim:set ts=4 sw=4: */
#endif
