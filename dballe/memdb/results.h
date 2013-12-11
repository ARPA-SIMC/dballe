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

#ifndef DBA_MEMDB_RESULTS_H
#define DBA_MEMDB_RESULTS_H

#include <dballe/core/stlutils.h>
#include <dballe/memdb/valuestorage.h>
#include <dballe/memdb/index.h>
#include <dballe/memdb/match.h>
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
     */
    template<typename OUTITER>
    void copy_valptrs_to(OUTITER res);

    /**
     * Send results to res.
     *
     * It empties all sequences, so it can only be used once.
     */
    template<typename OUTITER>
    void copy_indices_to(OUTITER res);

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
        const std::set<size_t>* s = index.search(val);
        if (!s || s->empty()) return;
        sequences->add(*s);
        found = true;
    }

private:
    SequenceBuilder(const SequenceBuilder&);
    SequenceBuilder& operator=(const SequenceBuilder&);
};

}


}
}

#endif


