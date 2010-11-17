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
     * Database variable ID, or MISSING_INT if not applicable
     *
     * This corresponds to B33195
     */
    virtual int get_var_id() const;

    /**
     * Get the station ID, or MISSING_INT if not applicable
     *
     * This corresponds to DBA_KEY_ANA_ID
     */
    virtual int get_station_id() const;

    /**
     * Get station WMO code as a 5 character string.
     *
     * buf must be long enough to store 6 characters (the 5 digits of the WMO
     * code and the trailing \0)
     *
     * Set buf[0] = 0 if not applicable
     */
    virtual void get_station_wmo(char* buf) const;

    /**
     * Get the date as an array of 6 values
     *
     * If not applicable, set values[0] to -1
     */
    virtual void get_date(int* values) const;

    /**
     * Get coordinates in 1/10000 of degree
     *
     * Either value can be set to MISSING_INT if not applicable.
     */
    virtual void get_coords(int* lat, int* lon) const;

    /**
     * Get rep_memo.
     *
     * Return NULL if not applicable
     */
    virtual const char* get_rep_memo() const;
};

struct Matcher
{
    enum Result {
        MATCH_YES,  // Item matches
        MATCH_NO,   // Item does not match
        MATCH_NA    // Match not applicable to this item
    };

    virtual ~Matcher() {}

    virtual Result match(const Matched& item) const = 0;
    virtual void to_record(dballe::Record& query) const = 0;

    static std::auto_ptr<Matcher> create(const dballe::Record& query);
};

}

/* vim:set ts=4 sw=4: */
#endif
