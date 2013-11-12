/*
 * core/stlutils - Useful functions to work with the STL
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

#ifndef DBA_CORE_STLUTILS_H
#define DBA_CORE_STLUTILS_H

#include <vector>
#include <set>

namespace dballe {
namespace stl {

namespace stlutils {

template<typename ITER>
struct Sequence
{
    ITER begin;
    ITER end;

    Sequence(const ITER& begin, const ITER& end)
        : begin(begin), end(end) {}

    bool valid() const { return begin != end; }
    const typename ITER::value_type& operator*() const { return *begin; }
    void next() { ++begin; }
    bool operator==(const Sequence& s) const
    {
        return begin == s.begin && end == s.end;
    }
    bool operator!=(const Sequence& s) const
    {
        return begin != s.begin || end != s.end;
    }
};

template<typename ITER>
struct Itersection
{
    std::vector< Sequence<ITER> > iters;

    Itersection(const std::vector< Sequence<ITER> >& iters) : iters(iters) {}
    bool operator==(const Itersection& i) { return iters == i.iters; }
    bool operator!=(const Itersection& i) { return iters != i.iters; }

    const typename ITER::value_type& get() const { return *iters[0].begin; }

    bool advance()
    {
        iters[0].next();
        return sync_iters();
    }

    // Advance iterators so that they all point to items of the same value,
    // or so that we become the end iterator
    bool sync_iters();
};

template<typename ITER>
struct Iterunion
{
    std::vector< Sequence<ITER> > iters;
    const typename ITER::value_type* minval;

    Iterunion(const std::vector< Sequence<ITER> >& iters) : iters(iters), minval(0) { }
    bool operator==(const Iterunion& i) { return iters == i.iters; }
    bool operator!=(const Iterunion& i) { return iters != i.iters; }

    const typename ITER::value_type& get() const { return *minval; }

    /**
     * Find the next minimum value.
     *
     * If minval is 0, set it to the minimum value.
     * If minval points to an element, advance all sequences that have that
     * element as minimum value, and set minval to the next minimum value.
     *
     * Returns false when all sequences are exausted; true if a new minimum
     * value was found.
     */
    bool find_min();
};

}

/**
 * Virtual container containing the intersection of an arbitrary number of
 * sorted (begin, end) sequences.
 */
template<class ITER>
class Intersection
{
protected:
    std::vector< stlutils::Sequence<ITER> > sequences;

public:
    struct const_iterator : public std::iterator<std::forward_iterator_tag, typename ITER::value_type>
    {
        stlutils::Itersection<ITER>* iters;

        // Begin iterator
        const_iterator(const std::vector< stlutils::Sequence<ITER> >& seqs)
            : iters(new stlutils::Itersection<ITER>(seqs))
        {
            if (!iters->sync_iters())
            {
                delete iters;
                iters = 0;
            }
        }
        // End iterator
        const_iterator() : iters(0) {}
        const_iterator(const const_iterator& i)
            : iters(i.iters)
        {
            // auto_ptr semantics
            const_cast<const_iterator*>(&i)->iters = 0;
        }
        const_iterator& operator=(const const_iterator& i)
        {
            // auto_ptr semantics
            if (&i == this) return this;
            if (iters) delete iters;
            iters = i.iters;
            const_cast<const_iterator*>(&i)->iters = 0;
            return *this;
        }
        ~const_iterator() { if (iters) delete iters; }

        const typename ITER::value_type& operator*() const { return iters->get(); }
        const_iterator& operator++()
        {
            if (!iters->advance())
            {
                delete iters;
                iters = 0;
            }
            return *this;
        }
        bool operator==(const const_iterator& i) const
        {
            if (iters == i.iters) return true;
            if (!iters || !i.iters) return false;
            return *iters == *i.iters;
        }
        bool operator!=(const const_iterator& i) const
        {
            if (iters == i.iters) return false;
            if (!iters || !i.iters) return true;
            return *iters != *i.iters;
        }
    };

    void clear() { sequences.clear(); }

    size_t sources_size() const { return sequences.size(); }

    const_iterator begin() const
    {
        return const_iterator(sequences);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

    void add(const ITER& begin, const ITER& end)
    {
        sequences.push_back(stlutils::Sequence<ITER>(begin, end));
    }
};

template<typename T>
class SetIntersection
{
protected:
    std::vector<const std::set<T>*> sets;
    Intersection<typename std::set<T>::const_iterator> intersection;

public:
    void add(const std::set<T>& set)
    {
        sets.push_back(&set);
    }

    typedef typename Intersection<typename std::set<T>::const_iterator>::const_iterator const_iterator;

    const_iterator begin()
    {
        if (sets.size() != intersection.sources_size())
        {
            intersection.clear();

            // Look for the highest first element in all sets
            bool first = true;
            T max_of_first;
            for (typename std::vector<const std::set<T>*>::const_iterator i = sets.begin();
                    i != sets.end(); ++i)
            {
                const std::set<T>& s = **i;
                // If one of the sets is empty, then the intersection is empty
                if (s.begin() == s.end()) return end();
                if (first)
                {
                    max_of_first = *s.begin();
                    first = false;
                } else {
                    if (max_of_first < *s.begin())
                        max_of_first = *s.begin();
                }
            }

            // Populate intersection with all the ranges we intersect
            for (typename std::vector<const std::set<T>*>::const_iterator i = sets.begin();
                    i != sets.end(); ++i)
            {
                const std::set<T>& s = **i;
                intersection.add(s.lower_bound(max_of_first), s.end());
            }
        }
        return intersection.begin();
    }

    const_iterator end()
    {
        return intersection.end();
    }
};

/**
 * Virtual container containing the union of an arbitrary number of
 * sorted (begin, end) sequences.
 */
template<class ITER>
class Union
{
protected:
    std::vector< stlutils::Sequence<ITER> > sequences;

public:
    struct const_iterator : public std::iterator<std::forward_iterator_tag, typename ITER::value_type>
    {
        typename stlutils::Iterunion<ITER>* iters;

        // Begin iterator
        const_iterator(const std::vector< stlutils::Sequence<ITER> >& seqs)
            : iters(new stlutils::Iterunion<ITER>(seqs))
        {
            if (!iters->find_min())
            {
                delete iters;
                iters = 0;
            }
        }
        // End iterator
        const_iterator() : iters(0) {}
        const_iterator(const const_iterator& i)
            : iters(i.iters)
        {
            // auto_ptr semantics
            const_cast<const_iterator*>(&i)->iters = 0;
        }
        const_iterator& operator=(const const_iterator& i)
        {
            // auto_ptr semantics
            if (&i == this) return this;
            if (iters) delete iters;
            iters = i.iters;
            const_cast<const_iterator*>(&i)->iters = 0;
            return *this;
        }
        ~const_iterator() { if (iters) delete iters; }

        const typename ITER::value_type& operator*() const { return iters->get(); }
        const_iterator& operator++()
        {
            if (!iters->find_min())
            {
                delete iters;
                iters = 0;
            }
            return *this;
        }
        bool operator==(const const_iterator& i) const
        {
            if (iters == i.iters) return true;
            if (!iters || !i.iters) return false;
            return *iters == *i.iters;
        }
        bool operator!=(const const_iterator& i) const
        {
            if (iters == i.iters) return false;
            if (!iters || !i.iters) return true;
            return *iters != *i.iters;
        }
    };

    void clear() { sequences.clear(); }

    size_t sources_size() const { return sequences.size(); }

    const_iterator begin() const
    {
        return const_iterator(sequences);
    }

    const_iterator end() const
    {
        return const_iterator();
    }

    void add(const ITER& begin, const ITER& end)
    {
        sequences.push_back(stlutils::Sequence<ITER>(begin, end));
    }
};

}
}

#endif
