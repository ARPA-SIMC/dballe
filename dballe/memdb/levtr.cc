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

void LevTrs::query(const Record& rec, Results<LevTr>& res) const
{
    match::Strategy<LevTr> strategy;

    Level level = rec.get_level();
    if (level != Level())
    {
        Level levmin(
                level.ltype1 != MISSING_INT ? level.ltype1 : 0,
                level.l1 != MISSING_INT ? level.l1 : 0,
                level.ltype2 != MISSING_INT ? level.ltype2 : 0,
                level.l2 != MISSING_INT ? level.l2 : 0);
        strategy.add(by_level, levmin, level);
    }

    Trange trange = rec.get_trange();
    if (trange != Trange())
    {
        Trange trmin(
                trange.pind != MISSING_INT ? trange.pind : 0,
                trange.p1 != MISSING_INT ? trange.p1 : 0,
                trange.p2 != MISSING_INT ? trange.p2 : 0);
        strategy.add(by_trange, trmin, trange);
    }

    strategy.activate(res);
}


}
}

#include "core.tcc"
#include "query.tcc"

namespace dballe {
namespace memdb {
template class Results<LevTr>;
}
}
