/*
 * dballe/match-wreport - Matched implementation for wreport bulletins
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

#ifndef DBALLE_CORE_MATCH_WREPORT_H
#define DBALLE_CORE_MATCH_WREPORT_H

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/core/matcher.h>

namespace wreport {
struct Var;
struct Subset;
}

namespace dballe {

struct MatchedSubset : public Matched
{
    const wreport::Subset& r;

    MatchedSubset(const wreport::Subset& r);
    ~MatchedSubset();

    /**
     * Return YES if the subset contains at least one var with the given B33195
     * attribute; else return NA.
     */
    virtual matcher::Result match_var_id(int val) const;

    virtual matcher::Result match_station_id(int val) const;
    virtual matcher::Result match_station_wmo(int block, int station=-1) const;
    virtual matcher::Result match_date(const int* min, const int* max) const;
    virtual matcher::Result match_coords(int latmin, int latmax, int lonmin, int lonmax) const;
    virtual matcher::Result match_rep_memo(const char* memo) const;

protected:
    int date[6];
    int lat, lon;
    const wreport::Var* var_ana_id;
    const wreport::Var* var_block;
    const wreport::Var* var_station;
    const wreport::Var* var_rep_memo;
};

}

/* vim:set ts=4 sw=4: */
#endif
