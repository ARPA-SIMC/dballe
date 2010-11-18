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

#include <dballe/core/match-wreport.h>
#include <dballe/core/defs.h>
#include <wreport/subset.h>
#include <wreport/bulletin.h>
#include <cmath>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {

MatchedSubset::MatchedSubset(const Subset& r)
    : r(r), lat(MISSING_INT), lon(MISSING_INT),
      var_ana_id(0), var_block(0), var_station(0), var_rep_memo(0)
{
    for (int i = 0; i < 6; ++i)
        date[i] = -1;

    // Scan message taking note of significant values
    for (Subset::const_iterator i = r.begin(); i != r.end(); ++i)
    {
        switch (i->code())
        {
            case WR_VAR(0,  1,   1): var_block = &*i; break;
            case WR_VAR(0,  1,   2): var_station = &*i; break;
            case WR_VAR(0,  1, 192): var_ana_id = &*i; break;
            case WR_VAR(0,  1, 194): var_rep_memo = &*i; break;
            case WR_VAR(0,  4,   1): date[0] = i->enq(-1); break;
            case WR_VAR(0,  4,   2): date[1] = i->enq(-1); break;
            case WR_VAR(0,  4,   3): date[2] = i->enq(-1); break;
            case WR_VAR(0,  4,   4): date[3] = i->enq(-1); break;
            case WR_VAR(0,  4,   5): date[4] = i->enq(-1); break;
            case WR_VAR(0,  4,   6): date[5] = i->enq(-1); break;
            case WR_VAR(0,  4,   7): date[5] = (int)rint(i->enq(-1.0)); break;
            case WR_VAR(0,  5,   1):
            case WR_VAR(0,  5,   2): if (i->isset()) lat = i->enqd() * 100000; break;
            case WR_VAR(0,  6,   1):
            case WR_VAR(0,  6,   2): if (i->isset()) lon = i->enqd() * 100000; break;
        }
    }

    // Fill in missing date bits
    if (date[0] != -1)
    {
        date[1] = date[1] != -1 ? date[1] : 1;
        date[2] = date[2] != -1 ? date[2] : 1;
        date[3] = date[3] != -1 ? date[3] : 0;
        date[4] = date[4] != -1 ? date[4] : 0;
        date[5] = date[5] != -1 ? date[5] : 0;
    }
}

MatchedSubset::~MatchedSubset()
{
}

matcher::Result MatchedSubset::match_var_id(int val) const
{
    for (Subset::const_iterator i = r.begin(); i != r.end(); ++i)
    {
        if (const Var* a = i->enqa(WR_VAR(0, 33, 195)))
            if (a->enqi() == val)
                return matcher::MATCH_YES;
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedSubset::match_station_id(int val) const
{
    if (!var_ana_id) return matcher::MATCH_NA;
    if (!var_ana_id->isset()) return matcher::MATCH_NA;
    return var_ana_id->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedSubset::match_station_wmo(int block, int station) const
{
    if (!var_block) return matcher::MATCH_NA;
    if (!var_block->isset()) return matcher::MATCH_NA;
    if (var_block->enqi() != block) return matcher::MATCH_NO;
    if (station == -1) return matcher::MATCH_YES;
    if (!var_station) return matcher::MATCH_NA;
    if (!var_station->isset()) return matcher::MATCH_NA;
    if (var_station->enqi() != station) return matcher::MATCH_NO;
    return matcher::MATCH_YES;
}

matcher::Result MatchedSubset::match_date(const int* min, const int* max) const
{
    if (date[0] == -1) return matcher::MATCH_NA;
    return Matched::date_in_range(date, min, max);
}

matcher::Result MatchedSubset::match_coords(int latmin, int latmax, int lonmin, int lonmax) const
{
    matcher::Result r1 = matcher::MATCH_NA;
    if (lat != MISSING_INT)
        r1 = Matched::int_in_range(lat, latmin, latmax);
    else if (latmin == MISSING_INT && latmax == MISSING_INT)
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (lon != MISSING_INT)
        r2 = Matched::int_in_range(lon, lonmin, lonmax);
    else if (lonmin == MISSING_INT && lonmax == MISSING_INT)
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedSubset::match_rep_memo(const char* memo) const
{
    if (const char* var = var_rep_memo ? var_rep_memo->value() : 0)
    {
        return strcmp(memo, var) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

MatchedBulletin::MatchedBulletin(const wreport::Bulletin& r)
    : r(r)
{
    subsets = new const MatchedSubset*[r.subsets.size()];
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        subsets[i] = new MatchedSubset(r.subsets[i]);
}
MatchedBulletin::~MatchedBulletin()
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        delete subsets[i];
    delete[] subsets;
}

matcher::Result MatchedBulletin::match_var_id(int val) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_var_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_station_id(int val) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_station_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_station_wmo(int block, int station) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_station_wmo(block, station) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_date(const int* min, const int* max) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_date(min, max) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_coords(int latmin, int latmax, int lonmin, int lonmax) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_coords(latmin, latmax, lonmin, lonmax) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedBulletin::match_rep_memo(const char* memo) const
{
    for (unsigned i = 0; i < r.subsets.size(); ++i)
        if (subsets[i]->match_rep_memo(memo) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

}

/* vim:set ts=4 sw=4: */
