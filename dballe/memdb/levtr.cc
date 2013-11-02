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
#include <dballe/core/record.h>
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

const LevTr& LevTrs::obtain(const Level& level, const Trange& trange)
{
    // Search
    Positions res = by_level.search(level);
    by_trange.refine(trange, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i))
            return *get(*i);

    // Station not found, create it
    size_t pos = value_add(new LevTr(level, trange));
    // Index it
    by_level[level].insert(pos);
    by_trange[trange].insert(pos);
    // And return it
    return *get(pos);
}

const LevTr& LevTrs::obtain(const Record& rec)
{
    return obtain(rec.get_level(), rec.get_trange());
}

}
}

