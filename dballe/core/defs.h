/*
 * msg/defs - Common definitions
 *
 * Copyright (C) 2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_MSG_DEFS_H
#define DBA_MSG_DEFS_H

/** @file
 * @ingroup msg
 *
 * Common definitions
 */

#include <limits.h>
#include <iosfwd>

namespace dballe {

/**
 * Supported encodings
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2,
} Encoding;

const char* encoding_name(Encoding enc);

/**
 * Value to use for missing levels and time range components
 */
static const int MISSING_INT = INT_MAX;

struct Level
{
    /** Type of the first level.  See @ref level_table. */
    int ltype1;
    /** L1 value of the level.  See @ref level_table. */
    int l1;
    /** Type of the second level.  See @ref level_table. */
    int ltype2;
    /** L2 value of the level.  See @ref level_table. */
    int l2;

    Level(int ltype1=MISSING_INT, int l1=MISSING_INT, int ltype2=MISSING_INT, int l2=MISSING_INT)
        : ltype1(ltype1), l1(l1), ltype2(ltype2), l2(l2) {}

    bool operator==(const Level& l) const
    {
        return ltype1 == l.ltype1 && l1 == l.l1
            && ltype2 == l.ltype2 && l2 == l.l2;
    }

    bool operator!=(const Level& l) const
    {
        return ltype1 != l.ltype1 || l1 != l.l1
            || ltype2 != l.ltype2 || l2 != l.l2;
    }

    /**
     * Compare two Level strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < l, 0 if *this == l, 1 if *this > l
     */
    int compare(const Level& l) const
    {
        int res;
        if ((res = ltype1 - l.ltype1)) return res;
        if ((res = l1 - l.l1)) return res;
        if ((res = ltype2 - l.ltype2)) return res;
        return l2 - l.l2;
    }

    /**
     * Return a string description of this level
     */
    std::string describe() const;

    static inline Level cloud(int ltype2, int l2=MISSING_INT) { return Level(256, MISSING_INT, ltype2, l2); }
    static inline Level ana() { return Level(257); }
};

std::ostream& operator<<(std::ostream& out, const Level& l);

struct Trange
{
    /** Time range type indicator.  See @ref trange_table. */
    int pind;
    /** Time range P1 indicator.  See @ref trange_table. */
    int p1;
    /** Time range P2 indicator.  See @ref trange_table. */
    int p2;

    Trange(int pind=MISSING_INT, int p1=MISSING_INT, int p2=MISSING_INT)
        : pind(pind), p1(p1), p2(p2) {}

    bool operator==(const Trange& tr) const
    {
        return pind == tr.pind && p1 == tr.p1 && p2 == tr.p2;
    }

    bool operator!=(const Trange& tr) const
    {
        return pind != tr.pind || p1 != tr.p1 || p2 != tr.p2;
    }

    /**
     * Compare two Trange strutures, for use in sorting.
     *
     * @return
     *   -1 if *this < t, 0 if *this == t, 1 if *this > t
     */
    int compare(const Trange& t) const
    {
        int res;
        if ((res = pind - t.pind)) return res;
        if ((res = p1 - t.p1)) return res;
        return p2 - t.p2;
    }

    /**
     * Return a string description of this time range
     */
    std::string describe() const;

    static inline Trange instant() { return Trange(254); }
    static inline Trange ana() { return Trange(); }
};

std::ostream& operator<<(std::ostream& out, const Trange& l);

}

// vim:set ts=4 sw=4:
#endif
