/*
 * memdb/station - In memory representation of stations
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

#include "station.h"
#include <dballe/core/record.h>
#include <iostream>

using namespace std;

namespace dballe {
namespace memdb {

Stations::Stations() : ValueStorage<Station>() {}


const Station& Stations::obtain(const Coord& coords, const std::string& report)
{
    // Search
    Positions res = by_coord.search(coords);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && !get(*i)->mobile && get(*i)->report == report)
            return *get(*i);

    // Station not found, create it
    size_t pos = value_add(new Station(coords, report));
    // Index it
    by_coord[coords].insert(pos);
    // And return it
    return *get(pos);
}

const Station& Stations::obtain(const Coord& coords, const std::string& ident, const std::string& report)
{
    // Search
    Positions res = by_coord.search(coords);
    by_ident.refine(ident, res);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && get(*i)->mobile && get(*i)->report == report)
            return *get(*i);

    // Station not found, create it
    size_t pos = value_add(new Station(coords, ident, report));
    // Index it
    by_coord[coords].insert(pos);
    by_ident[ident].insert(pos);
    // And return it
    return *get(pos);
}

}
}

