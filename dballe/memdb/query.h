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

#ifndef DBA_MEMDB_QUERY_H
#define DBA_MEMDB_QUERY_H

#include <vector>
#include <cstddef>

namespace dballe {
namespace memdb {
template<typename T> struct ValueStorage;

/// Base class for match functors
template<typename T>
struct Match
{
    virtual ~Match() {}

    virtual bool operator()(const T&) const = 0;
};

class BaseResults
{
protected:
    /// True if the result is 'any ID is good'
    bool select_all;
    /// If select_all is false, this is the list of good IDs
    std::vector<size_t> indices;

public:
    BaseResults();

    bool is_select_all() const { return select_all; }

    /// Intersect with a singleton set
    void intersect(size_t pos);

    /// Intersect with Positions
    template<typename ITER>
    void intersect(const ITER& begin, const ITER& end, const Match<typename ITER::value_type>* match=0);

    /// Intersect with Positions
    template<typename ITER>
    void intersect(const ITER& begin, const ITER& end, const Match<typename ITER::value_type>& match)
    {
        intersect(begin, end, &match);
    }
};

/// Query results as a sorted vector of indices
template<typename T>
class Results : public BaseResults
{
protected:
    /// ValueStorage mapping indices to values
    const ValueStorage<T>& values;

public:
    typedef std::vector<size_t>::const_iterator index_const_iterator;

    Results(const ValueStorage<T>& values) : values(values) {}

    size_t size() const;

    template<typename ITER>
    struct values_const_iterator : std::iterator<std::forward_iterator_tag, T>
    {
        const ValueStorage<T>* values;
        ITER iter;

        values_const_iterator(const ValueStorage<T>& values, const ITER& iter)
            : values(&values), iter(iter) {}

        size_t index() const { return *iter; }

        const T& operator*() const
        {
            return *(*values)[*iter];
        }
        values_const_iterator& operator++()
        {
            ++iter;
            return *this;
        }
        bool operator==(const values_const_iterator& i) const
        {
            return values == i.values && iter == i.iter;
        }
        bool operator!=(const values_const_iterator& i) const
        {
            return values != i.values || iter != i.iter;
        }
    };

    typedef values_const_iterator<std::vector<size_t>::const_iterator> selected_const_iterator;
    typedef values_const_iterator<typename ValueStorage<T>::index_iterator> all_const_iterator;

    selected_const_iterator selected_begin() const
    {
        return selected_const_iterator(values, indices.begin());
    }
    selected_const_iterator selected_end() const
    {
        return selected_const_iterator(values, indices.end());
    }

    index_const_iterator selected_index_begin() const { return indices.begin(); }
    index_const_iterator selected_index_end() const { return indices.end(); }

    all_const_iterator all_begin() const
    {
        return all_const_iterator(values, values.index_begin());
    }
    all_const_iterator all_end() const
    {
        return all_const_iterator(values, values.index_end());
    }

    typename ValueStorage<T>::index_iterator all_index_begin() const { return values.index_begin(); }
    typename ValueStorage<T>::index_iterator all_index_end() const { return values.index_end(); }
};

namespace match {

template<typename T>
class And : public Match<T>
{
protected:
    std::vector<Match<T>*> matches;

public:
    ~And();

    /// Add a match to and, taking ownership of its memory management
    void add(Match<T>* m) { matches.push_back(m); }

    bool operator()(const T& val) const;

private:
    And(const And<T>&);
    And<T>& operator=(const And<T>&);
};

template<typename T>
class Idx2Values : public Match<size_t>
{
protected:
    const ValueStorage<T>& index;
    const Match<const T&>& next;

public:
    Idx2Values(const ValueStorage<T>& index, const Match<const T&>& next) : index(index), next(next) {}

    bool operator()(const size_t& val) const;
};
template<typename T>
Idx2Values<T> idx2values(const ValueStorage<T>& index, const Match<const T&>& next) { return Idx2Values<T>(index, next); }

}

}
}

#endif


