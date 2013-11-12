/*
 * memdb/core - Core functions for memdb implementation
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

#ifndef DBA_MEMDB_CORE_TCC
#define DBA_MEMDB_CORE_TCC

#include "core.h"
#include "query.h"
#include "dballe/core/stlutils.h"
#include <algorithm>

namespace dballe {
namespace memdb {

template<typename T>
void Index<T>::query(const const_iterator& begin, const const_iterator& end, const Match<size_t>* filter, BaseResults& res) const
{
    std::auto_ptr< stl::Sequences<size_t> > sequences(new stl::Sequences<size_t>);
    for (const_iterator i = begin; begin != end; ++i)
        sequences->add(i->second.begin(), i->second.end());
    stl::Intersection<size_t> merger;
    res.intersect(merger.begin(sequences), merger.end(), filter);
}

template<typename T>
void Index<T>::query(const const_iterator& begin, const const_iterator& end, const Match<size_t>& filter, BaseResults& res) const
{
    query(begin, end, &filter, res);
}

template<typename T>
void Index<T>::query(const const_iterator& begin, const const_iterator& end, BaseResults& res) const
{
    query(begin, end, 0, res);
}

template<typename T>
ValueStorage<T>::~ValueStorage()
{
    for (typename std::vector<T*>::iterator i = values.begin(); i != values.end(); ++i)
        delete *i;
}

template<typename T>
void ValueStorage<T>::clear()
{
    for (typename std::vector<T*>::iterator i = values.begin(); i != values.end(); ++i)
        delete *i;
    values.clear();
}

template<typename T>
size_t ValueStorage<T>::value_add(T* value)
{
    if (empty_slots.empty())
    {
        // No slots to reuse: append
        values.push_back(value);
        return values.size() - 1;
    }

    // Reuse an old slot
    size_t res = empty_slots.back();
    empty_slots.pop_back();
    (*this)[res] = value;
    return res;
}

template<typename T>
void ValueStorage<T>::value_remove(size_t pos)
{
    delete (*this)[pos];
    (*this)[pos] = 0;
    empty_slots.push_back(pos);
}

}
}

#endif


