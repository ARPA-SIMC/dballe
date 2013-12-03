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

    // Set to an empty result set
    void set_to_empty()
    {
        select_all = false;
        indices.clear();
    }

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

    typedef std::vector<size_t>::const_iterator index_const_iterator;
    index_const_iterator selected_index_begin() const { return indices.begin(); }
    index_const_iterator selected_index_end() const { return indices.end(); }
};

/// Query results as a sorted vector of indices
template<typename T>
class Results : public BaseResults
{
public:
    /// ValueStorage mapping indices to values
    const ValueStorage<T>& values;

protected:
    struct AbstractIterator
    {
        virtual ~AbstractIterator() {}
        virtual size_t get() const = 0;
        virtual bool next() = 0;
        virtual AbstractIterator* clone() const = 0;
        virtual bool equals(const AbstractIterator* i) const = 0;
    };

    template<typename ITER>
    struct IteratorImpl : public AbstractIterator
    {
        ITER begin;
        ITER end;
        IteratorImpl(const ITER& begin, const ITER& end) : begin(begin), end(end) {}
        virtual size_t get() const { return *begin; }
        virtual bool next()
        {
            ++begin;
            return begin != end;
        }
        virtual AbstractIterator* clone() const
        {
            return new IteratorImpl<ITER>(begin, end);
        }
        virtual bool equals(const AbstractIterator* i) const
        {
            if (const IteratorImpl<ITER>* ii = dynamic_cast<const IteratorImpl<ITER>*>(i))
                return begin == ii->begin;
            return false;
        }
    };

    template<typename ITER>
    static IteratorImpl<ITER>* make_abstract_iter(const ITER& begin, const ITER& end)
    {
        if (begin == end) return 0;
        return new IteratorImpl<ITER>(begin, end);
    }

public:
    Results(const ValueStorage<T>& values) : values(values) {}

    size_t size() const;

    typename ValueStorage<T>::index_iterator all_index_begin() const { return values.index_begin(); }
    typename ValueStorage<T>::index_iterator all_index_end() const { return values.index_end(); }

    struct const_index_iterator : std::iterator<std::forward_iterator_tag, size_t>
    {
        AbstractIterator* iter;

        const_index_iterator(AbstractIterator* iter) : iter(iter) {}
        const_index_iterator(const const_index_iterator& i) : iter(i.iter ? i.iter->clone() : 0) {}
        ~const_index_iterator() { if (iter) delete iter; }
        const_index_iterator& operator=(const const_index_iterator& i)
        {
            if (iter != i.iter)
            {
                if (iter)
                {
                    delete iter;
                    iter = 0;
                }
                if (i.iter)
                    iter = i.iter->clone();
            }
            return *this;
        }

        const size_t operator*() const
        {
            return iter->get();
        }
        const_index_iterator& operator++()
        {
            if (!iter->next())
            {
                delete iter;
                iter = 0;
            }
            return *this;
        }
        bool operator==(const const_index_iterator& i) const
        {
            if (!iter && !i.iter) return true;
            if (!iter || !i.iter) return false;
            return iter->equals(i.iter);
        }
        bool operator!=(const const_index_iterator& i) const
        {
            if (!iter && !i.iter) return false;
            if (!iter || !i.iter) return true;
            return !iter->equals(i.iter);
        }
    };

    struct const_iterator : std::iterator<std::forward_iterator_tag, T>
    {
        const ValueStorage<T>* values;
        AbstractIterator* iter;

        const_iterator(const ValueStorage<T>& values, AbstractIterator* iter)
            : values(&values), iter(iter) {}
        const_iterator(const const_iterator& i)
            : values(i.values), iter(i.iter ? i.iter->clone() : 0) {}
        ~const_iterator() { if (iter) delete iter; }
        const_iterator& operator=(const const_iterator& i)
        {
            values = i.values;
            if (iter != i.iter)
            {
                if (iter)
                {
                    delete iter;
                    iter = 0;
                }
                if (i.iter)
                    iter = i.iter->clone();
            }
            return *this;
        }

        size_t index() const { return iter->get(); }

        const T* operator->() const
        {
            return (*values)[iter->get()];
        }
        const T& operator*() const
        {
            return *(*values)[iter->get()];
        }
        const_iterator& operator++()
        {
            if (!iter->next())
            {
                delete iter;
                iter = 0;
            }
            return *this;
        }
        bool operator==(const const_iterator& i) const
        {
            if (values != i.values) return false;
            if (!iter && !i.iter) return true;
            if (!iter || !i.iter) return false;
            return iter->equals(i.iter);
        }
        bool operator!=(const const_iterator& i) const
        {
            if (values != i.values) return true;
            if (!iter && !i.iter) return false;
            if (!iter || !i.iter) return true;
            return !iter->equals(i.iter);
        }
    };

    const_iterator begin() const
    {
        if (select_all)
        {
            return const_iterator(values, make_abstract_iter(all_index_begin(), all_index_end()));
        } else {
            return const_iterator(values, make_abstract_iter(selected_index_begin(), selected_index_end()));
        }
    }
    const_iterator end() const
    {
        return const_iterator(values, 0);
    }

    const_index_iterator index_begin() const
    {
        if (select_all)
        {
            return const_index_iterator(make_abstract_iter(all_index_begin(), all_index_end()));
        } else {
            return const_index_iterator(make_abstract_iter(selected_index_begin(), selected_index_end()));
        }
    }
    const_index_iterator index_end() const
    {
        return const_index_iterator(0);
    }
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
struct Strategy
{
    stl::Sequences<size_t>* others_to_intersect;
    stl::SetIntersection<size_t>* indices;
    FilterBuilder<T> filter;

    Strategy() : others_to_intersect(0), indices(0) {}
    ~Strategy()
    {
        if (others_to_intersect) delete others_to_intersect;
        if (indices) delete indices;
    }

    void add_union(std::auto_ptr< stl::Sequences<size_t> >& seq);

    void add(const Positions& p);

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
        return res;
    }

    void add(Match<T>* f)
    {
        filter.add(f);
    }

    /**
     * Send results to res.
     *
     * It empties all sequences, so it can only be used once.
     */
    void activate(Results<T>& res);

private:
    Strategy(const Strategy<T>&);
    Strategy<T> operator=(const Strategy<T>&);
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

}
}

#endif


