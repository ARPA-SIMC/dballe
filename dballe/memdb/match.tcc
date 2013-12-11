/*
 * memdb/match - Record-by-record match
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

#ifndef DBA_MEMDB_MATCH_TCC
#define DBA_MEMDB_MATCH_TCC

#include "dballe/core/stlutils.h"
#include <algorithm>
#ifdef TRACE_QUERY
#include <cstdio>
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace memdb {
namespace match {

template<typename T>
And<T>::~And()
{
    for (typename std::vector<Match<T>*>::iterator i = matches.begin(); i != matches.end(); ++i)
        delete *i;
}

template<typename T>
bool And<T>::operator()(const T& val) const
{
    for (typename std::vector<Match<T>*>::const_iterator i = matches.begin(); i != matches.end(); ++i)
        if (!(**i)(val))
            return false;
    return true;
}

template<typename T>
void FilterBuilder<T>::add(Match<T>* f)
{
    if (!filter)
        filter = f;
    else if (!is_and)
        filter = is_and = new And<T>(filter, f);
    else
        is_and->add(f);
}

}
}
}

#include "dballe/memdb/core.tcc"
#endif

