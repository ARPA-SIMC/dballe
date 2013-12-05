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

#include <dballe/core/stlutils.h>
#include <dballe/memdb/core.h>
#include <dballe/memdb/index.h>
#include <vector>
#include <memory>
#include <cstddef>

// #define TRACE_QUERY

namespace dballe {

namespace memdb {
template<typename T> struct ValueStorage;

#ifdef TRACE_QUERY
#define IF_TRACE_QUERY if (1)
void trace_query(const char* fmt, ...);
#else
#define IF_TRACE_QUERY if (0)
#define trace_query(...) do {} while(0)
#endif


/// Base class for match functors
template<typename T>
struct Match
{
    virtual ~Match() {}

    virtual bool operator()(const T&) const = 0;
};

namespace match {

template<typename T>
class And : public Match<T>
{
protected:
    std::vector<Match<T>*> matches;

public:
    And() {}
    And(Match<T>* f1, Match<T>* f2) { add(f1); add(f2); }
    ~And();

    /// Add a match to and, taking ownership of its memory management
    void add(Match<T>* m) { matches.push_back(m); }

    bool operator()(const T& val) const;

private:
    And(const And<T>&);
    And<T>& operator=(const And<T>&);
};

/// Build an And of filters step by step
template<typename T>
struct FilterBuilder
{
    Match<T>* filter;
    And<T>* is_and; // When nonzero, it points to the same as 'filter'

    FilterBuilder() : filter(0), is_and(0) {}
    ~FilterBuilder()
    {
        if (filter) delete filter;
    }

    const Match<T>* get() const { return filter; }
    const Match<T>& operator*() const { return *filter; }

    void add(Match<T>* f);
};

template<typename T>
class Idx2Values : public Match<size_t>
{
protected:
    const ValueStorage<T>& index;
    const Match<T>& next;

public:
    Idx2Values(const ValueStorage<T>& index, const Match<T>& next) : index(index), next(next) {}

    bool operator()(const size_t& val) const;
};
template<typename T>
Idx2Values<T> idx2values(const ValueStorage<T>& index, const Match<T>& next) { return Idx2Values<T>(index, next); }

}

template<typename T>
class Results
{
public:
    const ValueStorage<T>& values;

protected:
    /// Sequences of possible results to be intersected
    stl::Sequences<size_t>* others_to_intersect;

    /// Sets of possible results, to be intersected
    stl::SetIntersection<size_t>* indices;

    /// Filters to apply to each candidate result
    match::FilterBuilder<T> filter;

    /// True if all elements are selected
    bool all;

    /// True if it has been determined that there are no results
    bool empty;

public:
    Results(const ValueStorage<T>& values) : values(values), others_to_intersect(0), indices(0), all(true), empty(false) {}
    ~Results()
    {
        if (others_to_intersect) delete others_to_intersect;
        if (indices) delete indices;
    }

    /// Check if we just select all elements
    bool is_select_all() const { return all; }

    /// Check if we just select all elements
    bool is_empty() const { return empty; }

    /// Disregard everything and just return no items
    void set_to_empty() { all = false; empty = true; }

    void add_union(std::auto_ptr< stl::Sequences<size_t> > seq);

    /// Add a set of one single element to intersect with the rest
    void add(size_t singleton);

    void add(const std::set<size_t>& p);

    template<typename K>
    bool add(const Index<K>& index, const K& val);

    template<typename K>
    bool add(const Index<K>& index, const K& min, const K& max);

    template<typename K>
    bool add_since(const Index<K>& index, const typename Index<K>::const_iterator begin)
    {
        bool res = false;
        for (typename Index<K>::const_iterator i = begin; i != index.end(); ++i)
        {
            this->add(i->second);
            res = true;
        }
        all = false;
        return res;
    }

    template<typename K>
    bool add_until(const Index<K>& index, const typename Index<K>::const_iterator end)
    {
        bool res = false;
        for (typename Index<K>::const_iterator i = index.begin(); i != end; ++i)
        {
            this->add(i->second);
            res = true;
        }
        all = false;
        return res;
    }

    void add(Match<T>* f)
    {
        filter.add(f);
        all = false;
    }

    /**
     * Send results to res.
     *
     * It empties all sequences, so it can only be used once.
     *
     * @returns
     *   true if resulting values have been copied to \a res
     *   false if all of values is part of the result set, in which case
     *   nothing is sent to \a res
     */
    template<typename OUTITER>
    bool copy_valptrs_to(OUTITER res);

private:
    Results(const Results<T>&);
    Results<T> operator=(const Results<T>&);
};

namespace match {

template<typename T>
struct SequenceBuilder
{
protected:
    const Index<const T*>& index;
    stl::Sequences<size_t>* sequences;
    bool found;

public:
    SequenceBuilder(const Index<const T*>& index)
       : index(index), sequences(new stl::Sequences<size_t>), found(false) {}
    ~SequenceBuilder()
    {
        if (sequences) delete sequences;
    }

    bool found_items_in_index() const { return found; }

    std::auto_ptr< stl::Sequences<size_t> > release_sequences()
    {
        std::auto_ptr< stl::Sequences<size_t> > res(sequences);
        sequences = 0;
        return res;
    }

    void insert(const T* val)
    {
        found |= index.search(val, *sequences);
    }

private:
    SequenceBuilder(const SequenceBuilder&);
    SequenceBuilder& operator=(const SequenceBuilder&);
};

}


}
}

#endif


