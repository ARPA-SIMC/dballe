/*
 * memdb/ltr - In memory representation of level-timerange metadata
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

#ifndef DBA_MEMDB_LTR_H
#define DBA_MEMDB_LTR_H

#include <dballe/memdb/core.h>
#include <dballe/core/defs.h>
#include <string>
#include <vector>

namespace dballe {
namespace memdb {

/// Station information
struct LevTr
{
    Level level;
    Trange trange;

    LevTr(const Level& level, const Trange& trange)
        : level(level), trange(trange) {}
};

/// Storage and index for station information
class LevTrs : public ValueStorage<LevTr>
{
protected:
    Index<Level> by_level;
    Index<Trange> by_trange;

public:
    LevTrs();

    /// Get a LevTr record
    const LevTr& obtain(const Level& level, const Trange& trange);
};

}
}

#endif

