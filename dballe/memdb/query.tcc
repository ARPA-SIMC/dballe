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
#ifdef TRACE_QUERY
#include <cstdio>
#endif

namespace dballe {
namespace memdb {

template<typename ITER>
void BaseResults::intersect(const ITER& begin, const ITER& iend, const Match<typename ITER::value_type>* match)
{
    ITER i = begin;

    if (select_all)
    {
        for ( ; i != iend; ++i)
            if (!match || (*match)(*i))
                indices.push_back(*i);
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
void FilterBuilder<T>::add(Match<T>* f)
{
    if (!filter)
        filter = f;
    else if (!is_and)
        filter = is_and = new And<T>(filter, f);
    else
        is_and->add(f);
}

template<typename T>
void Strategy<T>::add_union(std::auto_ptr< stl::Sequences<size_t> >& seq)
{
    if (!others_to_intersect)
        others_to_intersect = new stl::Sequences<size_t>;
    others_to_intersect->add_union(seq);
}

template<typename T>
void Strategy<T>::add(const Positions& p)
{
    if (!indices)
        indices = new stl::SetIntersection<size_t>;
    indices->add(p);
}

template<typename T> template<typename K>
bool Strategy<T>::add(const Index<K>& index, const K& val)
{
    typename Index<K>::const_iterator i = index.find(val);
    if (i == index.end())
    {
        trace_query("Adding positions from index lookup: element not found\n");
        return false;
    }
    trace_query("Adding positions from index lookup: found %zu elements\n", i->second.size());
    this->add(i->second);
    return true;
}

template<typename T> template<typename K>
bool Strategy<T>::add(const Index<K>& index, const K& min, const K& max)
{
    bool res = false;
    for (typename Index<K>::const_iterator i = index.lower_bound(min);
            i != index.upper_bound(max); ++i)
    {
        trace_query("Adding positions from index lookup: found %zu elements\n", i->second.size());
        this->add(i->second);
        res = true;
    }
    if (!res) trace_query("Adding positions from index lookup: no element not found\n");
    return res;
}

template<typename T>
void Strategy<T>::activate(Results<T>& res)
{
    if (!others_to_intersect)
    {
        if (indices)
        {
            if (filter.get())
            {
                trace_query("Activating strategy with intersection and filter\n");
                res.intersect(indices->begin(), indices->end(), match::idx2values(res.values, *filter));
            } else {
                trace_query("Activating strategy with intersection only\n");
                res.intersect(indices->begin(), indices->end());
            }
        } else {
            if (filter.get())
            {
                trace_query("Activating strategy with filter only\n");
                res.intersect(res.values.index_begin(), res.values.index_end(), match::idx2values(res.values, *filter));
            } else {
                // Nothing to do: we don't filter on any constraint, so we just leave res as it is
                trace_query("Activating strategy leaving results as it is\n");
            }
        }
    } else {
        if (indices)
            others_to_intersect->add(indices->begin(), indices->end());
        auto_ptr< stl::Sequences<size_t> > sequences(others_to_intersect);
        others_to_intersect = 0;
        stl::Intersection<size_t> intersection;
        if (filter.get())
        {
            trace_query("Activating strategy with filter only\n");
            res.intersect(intersection.begin(sequences), intersection.end(), match::idx2values(res.values, *filter));
        } else {
            res.intersect(intersection.begin(sequences), intersection.end());
        }
    }
}

}

}
}

#include "dballe/core/stlutils.tcc"
#endif
