/*
 * memdb/levtr - In memory representation of level-timerange metadata
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "levtr.h"
#include "query.h"
#include "dballe/core/record.h"
#include "dballe/core/stlutils.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace dballe {
namespace memdb {

void LevTrs::clear()
{
    by_level.clear();
    by_trange.clear();
    ValueStorage<LevTr>::clear();
}

LevTrs::LevTrs() : ValueStorage<LevTr>() {}

size_t LevTrs::obtain(const Level& level, const Trange& trange)
{
    // Search
    stl::SetIntersection<size_t> res;
    if (by_level.search(level, res) && by_trange.search(trange, res))
        for (stl::SetIntersection<size_t>::const_iterator i = res.begin(); i != res.end(); ++i)
            if ((*this)[*i])
                return *i;

    // Station not found, create it
    size_t pos = value_add(new LevTr(level, trange));
    // Index it
    by_level[level].insert(pos);
    by_trange[trange].insert(pos);
    // And return it
    return pos;
}

size_t LevTrs::obtain(const Record& rec)
{
    return obtain(rec.get_level(), rec.get_trange());
}

namespace {
struct MatchLevel : public Match<LevTr>
{
    Level level;

    MatchLevel(const Level& level) : level(level) {}
    virtual bool operator()(const LevTr& val) const
    {
        if (level.ltype1 != MISSING_INT && level.ltype1 != val.level.ltype1) return false;
        if (level.l1 != MISSING_INT && level.l1 != val.level.l1) return false;
        if (level.ltype2 != MISSING_INT && level.ltype2 != val.level.ltype2) return false;
        if (level.l2 != MISSING_INT && level.l2 != val.level.l2) return false;
        return true;
    }
};
struct MatchTrange : public Match<LevTr>
{
    Trange trange;

    MatchTrange(const Trange& trange) : trange(trange) {}
    virtual bool operator()(const LevTr& val) const
    {
        if (trange.pind != MISSING_INT && trange.pind != val.trange.pind) return false;
        if (trange.p1 != MISSING_INT && trange.p1 != val.trange.p1) return false;
        if (trange.p2 != MISSING_INT && trange.p2 != val.trange.p2) return false;
        return true;
    }
};
}

void LevTrs::query(const Record& rec, Results<LevTr>& res) const
{
    Level level = rec.get_level();
    if (level != Level())
    {
        if (level.ltype1 != MISSING_INT)
        {
            Level levmin(
                    level.ltype1,
                    level.l1 != MISSING_INT ? level.l1 : 0,
                    level.ltype2 != MISSING_INT ? level.ltype2 : 0,
                    level.l2 != MISSING_INT ? level.l2 : 0);
            res.add(by_level, levmin, level);
            trace_query("Adding level range to strategy, from %d,%d,%d,%d to %d,%d,%d,%d\n",
                    levmin.ltype1, levmin.l1, levmin.ltype2, levmin.l2,
                    level.ltype1, level.l1, level.ltype2, level.l2);
        }
        trace_query("Adding level matcher for %d,%d,%d,%d\n",
                level.ltype1, level.l1, level.ltype2, level.l2);
        res.add(new MatchLevel(level));
    }

    Trange trange = rec.get_trange();
    if (trange != Trange())
    {
        if (trange.pind != MISSING_INT)
        {
            Trange trmin(
                    trange.pind,
                    trange.p1 != MISSING_INT ? trange.p1 : 0,
                    trange.p2 != MISSING_INT ? trange.p2 : 0);
            res.add(by_trange, trmin, trange);
            trace_query("Adding trange range to strategy, from %d,%d,%d to %d,%d,%d\n",
                    trmin.pind, trmin.p1, trmin.p2,
                    trange.pind, trange.p1, trange.p2);
        }
        trace_query("Adding trange matcher for %d,%d,%d\n",
                trange.pind, trange.p1, trange.p2);
        res.add(new MatchTrange(trange));
    }
}

void LevTrs::dump(FILE* out) const
{
    fprintf(out, "Levtrs:\n");
    for (size_t pos = 0; pos < values.size(); ++pos)
    {
        if (values[pos])
        {
            stringstream buf;
            buf << values[pos]->level
                << "\t" << values[pos]->trange;

            fprintf(out, " %4zu: %s\n", pos, buf.str().c_str());
        } else
            fprintf(out, " %4zu: (empty)\n", pos);
    }
};


}
}

#include "core.tcc"
#include "query.tcc"

namespace dballe {
namespace memdb {
template class Results<LevTr>;
}
}
