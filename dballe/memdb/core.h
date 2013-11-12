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
#include <cstdio>

namespace dballe {

namespace stl {
template<typename T> class SetIntersection;
template<typename T> class Sequences;
}

namespace memdb {

struct BaseResults;
template<typename T> struct Match;

/// Indices of elements inside a vector
struct Positions : public std::set<size_t>
{
    bool contains(size_t val) const
    {
        return find(val) != end();
    }

#if 0
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
#endif

    void dump(FILE* out) const;
};

/// Index a vector's elements based by one value
template<typename T>
struct Index : public std::map<T, Positions>
{
    typedef typename std::map<T, Positions>::const_iterator const_iterator;

    /// Lookup all positions for a value
    Positions search(const T& el) const;

    /**
     * Lookup all positions for a value, appending the results to a SetIntersection
     *
     * @returns false if el was not found; it leaves out untouched in that case
     */
    bool search(const T& el, stl::SetIntersection<size_t>& out) const;

    /**
     * Lookup all positions for a value, appending the results to a Sequences
     *
     * @returns false if el was not found; it leaves out untouched in that case
     */
    bool search(const T& el, stl::Sequences<size_t>& out) const;

    void query(const const_iterator& begin, const const_iterator& end, const Match<size_t>& filter, BaseResults& res) const;
    void query(const const_iterator& begin, const const_iterator& end, BaseResults& res) const;
    void query(const const_iterator& begin, const const_iterator& end, const Match<size_t>* filter, BaseResults& res) const;
};

template<typename T>
class ValueStorage
{
protected:
    std::vector<T*> values;
    std::vector<size_t> empty_slots;

public:
    struct index_iterator : public std::iterator<std::forward_iterator_tag, size_t>
    {
        const std::vector<T*>* values;
        size_t pos;

        index_iterator(const std::vector<T*>& values, size_t pos) : values(&values), pos(pos) { skip_gaps(); }
        // End iterator
        index_iterator(const std::vector<T*>& values) : values(&values), pos(values.size()) {}
        index_iterator& operator++() {
            ++pos;
            skip_gaps();
            return *this;
        }
        void skip_gaps()
        {
            while (pos != values->size() && !(*values)[pos])
                ++pos;
        }
        size_t operator*() const { return pos; }

        bool operator==(const index_iterator& i) const
        {
            return values == i.values && pos == i.pos;
        }

        bool operator!=(const index_iterator& i) const
        {
            return values != i.values || pos != i.pos;
        }
    };

    ValueStorage() {}
    virtual ~ValueStorage();

    void clear();

    /**
     * Number of valid elements
     *
     * This is not called size, because it is not the same as the maximum index
     * that can be passed to operator[]
     */
    size_t element_count() const { return values.size() - empty_slots.size(); }

    T* at(size_t idx) { return values.at(idx); }
    const T* at(size_t idx) const { return values.at(idx); }

    typename std::vector<T*>::reference operator[](size_t idx) { return values[idx]; }
    typename std::vector<T*>::const_reference operator[](size_t idx) const { return values[idx]; }

    index_iterator index_begin() const { return index_iterator(values, 0); }
    index_iterator index_end() const { return index_iterator(values); }

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

