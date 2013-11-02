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

#ifndef DBA_MEMDB_CORE_H
#define DBA_MEMDB_CORE_H

#include <set>
#include <map>
#include <vector>
#include <cstddef>

namespace dballe {
namespace memdb {

/// Indices of elements inside a vector
struct Positions : public std::set<size_t>
{
    bool contains(size_t val) const
    {
        return find(val) != end();
    }

    void inplace_intersect(const Positions& pos)
    {
        // Inplace intersection
        typename Positions::iterator ia = begin();
        typename Positions::const_iterator ib = pos.begin();
        while (ia != end() && ib != pos.end())
        {
            if (*ia < *ib)
                erase(ia++);
            else if (*ib < *ia)
                ++ib;
            else
            {
                ++ia;
                ++ib;
            }
        }
        erase(ia, end());
    }
};

/// Query results as a sorted vector of indices
class Results
{
protected:
    /// True if the result is 'any ID is good'
    bool select_all;
    /// If select_all is false, this is the list of good IDs
    std::vector<size_t> values;

public:
    Results();

    /// Intersect with a singleton set
    void intersect(size_t pos);

    /// Intersect with Positions
    template<typename ITER>
    void intersect(ITER begin, const ITER& end);
};

/// Index a vector's elements based by one value
template<typename T>
struct Index : public std::map<T, Positions>
{
    /// Lookup all positions for a value
    Positions search(const T& el) const;

    /// Refine a Positions set with the results of this lookup
    void refine(const T& el, Positions& res);
};

template<typename T>
class ValueStorage
{
protected:
    std::vector<T*> values;
    std::vector<size_t> empty_slots;

public:
    ValueStorage() {}
    virtual ~ValueStorage();

    void clear();

    T* at(size_t idx) { return values.at(idx); }
    const T* at(size_t idx) const { return values.at(idx); }

    typename std::vector<T*>::reference operator[](size_t idx) { return values[idx]; }
    typename std::vector<T*>::const_reference operator[](size_t idx) const { return values[idx]; }

protected:
    /// Add the value to the storage and return its index
    /// take ownership of the pointer memory management
    size_t value_add(T* value);

    /// Remove a value given its position
    void value_remove(size_t pos);

private:
    ValueStorage(const ValueStorage<T>&);
    ValueStorage& operator=(const ValueStorage<T>&);
};

}
}

#endif

