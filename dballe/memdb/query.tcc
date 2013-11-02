/*
 * memdb/query - Infrastructure used to query memdb data
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

#ifndef DBA_MEMDB_QUERY_TCC
#define DBA_MEMDB_QUERY_TCC

#include "query.h"
#include <algorithm>

namespace dballe {
namespace memdb {

template<typename ITER>
void Results::intersect(const ITER& begin, const ITER& iend, const Match<typename ITER::value_type>* match)
{
    ITER i = begin;

    if (select_all)
    {
        std::copy(i, iend, back_inserter(values));
        select_all = false;
        return;
    }

    // Vector that holds the common values
    vector<size_t> res;

    // Iterate the two sorted sequence
    vector<size_t>::const_iterator vi = values.begin();

    // If the set starts after the beginning of the current value vector,
    // jump to a valid starting point
    if (*i > *vi)
        vi = lower_bound(values.begin(), values.end(), *i);

    // Step through both sequences looking for the common values
    while (i != iend && vi != values.end())
    {
        if (*vi < *i)
            ++vi;
        else if (*vi > *i)
            ++i;
        else
        {
            if (!match || (*match)(*vi))
                res.push_back(*vi);
            ++vi;
            ++i;
        }
    }

    // Commit the results
    values = res;
}

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
    for (typename std::vector<Match<T>*>::iterator i = matches.begin(); i != matches.end(); ++i)
        if (!(**i)(val))
            return false;
    return true;
}

template<typename T>
bool Idx2Values<T>::operator()(const size_t& val) const
{
    const T* v = index[val];
    if (!v) return false;
    return next(*v);
}

}

}
}

#endif
