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
#include "dballe/core/stlutils.h"
#include <algorithm>

namespace dballe {
namespace memdb {

template<typename ITER>
void BaseResults::intersect(const ITER& begin, const ITER& iend, const Match<typename ITER::value_type>* match)
{
    ITER i = begin;

    if (select_all)
    {
        std::copy(i, iend, back_inserter(indices));
        select_all = false;
        return;
    }

    // Vector that holds the common values
    vector<size_t> res;

    // Iterate the two sorted sequence
    vector<size_t>::const_iterator vi = indices.begin();

    // If the set starts after the beginning of the current value vector,
    // jump to a valid starting point
    if (*i > *vi)
        vi = lower_bound(indices.begin(), indices.end(), *i);

    // Step through both sequences looking for the common values
    while (i != iend && vi != indices.end())
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
    indices = res;
}

template<typename T>
size_t Results<T>::size() const
{
    if (select_all)
        return values.element_count();
    else
        return indices.size();
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
    for (typename std::vector<Match<T>*>::const_iterator i = matches.begin(); i != matches.end(); ++i)
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

template<typename T>
void Strategy<T>::add(const Positions& p)
{
    if (!indices)
        indices = new stl::SetIntersection<size_t>;
    indices->add(p);
}

template<typename T>
void Strategy<T>::activate(Results<T>& res) const
{
    if (indices)
    {
        if (filter.get())
            res.intersect(indices->begin(), indices->end(), match::idx2values(res.values, *filter));
        else
            res.intersect(indices->begin(), indices->end());
    } else {
        if (filter.get())
            res.intersect(res.values.index_begin(), res.values.index_end(), match::idx2values(res.values, *filter));
        else
            ; // Nothing to do: we don't filter on any constraint, so we just leave res as it is
    }
}

}

}
}

#endif
