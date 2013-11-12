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
#include <memory>

namespace dballe {
namespace stl {

namespace stlutils {

template<typename T>
struct Sequence
{
    virtual ~Sequence() {}
    virtual bool valid() const = 0;
    virtual const T& get() const = 0;
    virtual void next() = 0;
};

} // back to dballe::stl

/// List of ranges
template<typename T>
struct Sequences : public std::vector<stlutils::Sequence<T>*>
{
    typedef typename std::vector<stlutils::Sequence<T>*>::iterator iterator;
    typedef typename std::vector<stlutils::Sequence<T>*>::const_iterator const_iterator;

    Sequences() {}
    ~Sequences()
    {
        for (iterator i = this->begin(); i != this->end(); ++i)
            delete *i;
    }

    // Add a begin-end range
    template<typename ITER>
    void add(const ITER& begin, const ITER& end);

    // Add a container's begin()-end() range
    template<typename C>
    void add(const C& container);

    void add(std::auto_ptr< stlutils::Sequence<T> >& sequence);

    // Add the union of the given sequences
    void add_union(std::auto_ptr< Sequences<T> >& sequences);

    // Add the union of the given sequences
    void add_intersection(std::auto_ptr< Sequences<T> >& sequences);

private:
    Sequences(const Sequences&);
    Sequences& operator=(const Sequences&);
};

namespace stlutils {

template<typename T>
struct SequenceGenerator
{
    Sequences<T>* sequences;

    SequenceGenerator() : sequences(0) {}
    SequenceGenerator(const SequenceGenerator<T>& sg) : sequences(sg.sequences)
    {
        // auto_ptr semantics
        const_cast<SequenceGenerator<T>*>(&sg)->sequences = 0;
    }
    SequenceGenerator(std::auto_ptr< Sequences<T> >& sequences) : sequences(sequences.release()) {}
    ~SequenceGenerator() { if (sequences) delete sequences; }

    SequenceGenerator<T>& operator=(const SequenceGenerator<T>& sg)
    {
        // auto_ptr semantics
        if (&sg == this) return this;
        if (sequences) delete sequences;
        sequences = sg.sequences;
        const_cast<SequenceGenerator<T>*>(&sg)->sequences = 0;
        return *this;
    }

    void clear()
    {
        if (sequences) delete sequences;
        sequences = 0;
    }

    bool has_items() const { return sequences != 0; }

    bool operator==(const SequenceGenerator<T>& i) const
    {
        if (sequences == i.sequences) return true;
        if (!sequences || !i.sequences) return false;
        return *sequences == *i.sequences;
    }
    bool operator!=(const SequenceGenerator<T>& i) const
    {
        if (sequences == i.sequences) return false;
        if (!sequences || !i.sequences) return true;
        return *sequences != *i.sequences;
    }
};

template<typename T>
struct Itersection : public SequenceGenerator<T>, public std::iterator<std::forward_iterator_tag, T>
{
    Itersection() {}
    Itersection(const Itersection<T>& sg) : SequenceGenerator<T>(sg) {}
    Itersection(std::auto_ptr< Sequences<T> >& sequences)
        : SequenceGenerator<T>(sequences)
    {
        sync_iters();
    }

    const T& get() const { return (*this->sequences)[0]->get(); }

    void advance()
    {
        (*this->sequences)[0]->next();
        sync_iters();
    }

    // Advance iterators so that they all point to items of the same value,
    // or so that we become the end iterator
    void sync_iters();

    const T& operator*() const { return this->get(); }
    Itersection<T>& operator++()
    {
        this->advance();
        return *this;
    }
};

template<typename T>
struct Iterunion : public SequenceGenerator<T>, public std::iterator<std::forward_iterator_tag, T>
{
    const T* minval;

    Iterunion() {}
    Iterunion(const Iterunion<T>& sg)
        : SequenceGenerator<T>(sg), minval(sg.minval)
    {
        // auto_ptr semantics
        const_cast<Iterunion<T>*>(&sg)->minval = 0;
    }
    Iterunion(std::auto_ptr< Sequences<T> >& sequences)
        : SequenceGenerator<T>(sequences), minval(0)
    {
        find_min();
    }
    Iterunion<T>& operator=(const Iterunion<T>& sg)
    {
        // auto_ptr semantics
        SequenceGenerator<T>::operator=(sg);
        minval = sg.minval;
        const_cast<Iterunion<T>*>(&sg)->minval = 0;
        return *this;
    }

    const T& get() const { return *minval; }

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
    void find_min();

    const T& operator*() const { return this->get(); }
    Iterunion<T>& operator++()
    {
        this->find_min();
        return *this;
    }
};

}

/**
 * Virtual container containing the intersection of an arbitrary number of
 * sorted (begin, end) sequences.
 */
template<class T>
class Intersection
{
public:
    typedef stlutils::Itersection<T> const_iterator;

    const_iterator begin(std::auto_ptr< Sequences<T> >& sequences) const
    {
        return const_iterator(sequences);
    }

    const_iterator end() const
    {
        return const_iterator();
    }
};

template<typename T>
class SetIntersection
{
protected:
    std::vector<const std::set<T>*> sets;
    Intersection<T> intersection;

public:
    void add(const std::set<T>& set)
    {
        sets.push_back(&set);
    }

    typedef typename Intersection<T>::const_iterator const_iterator;

    const_iterator begin()
    {
        std::auto_ptr< Sequences<T> > sequences(new Sequences<T>);

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
            sequences->add(s.lower_bound(max_of_first), s.end());
        }

        return intersection.begin(sequences);
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
template<class T>
class Union
{
public:
    typedef stlutils::Iterunion<T> const_iterator;

    const_iterator begin(std::auto_ptr< Sequences<T> >& sequences) const
    {
        return const_iterator(sequences);
    }

    const_iterator end() const
    {
        return const_iterator();
    }
};


}
}

#endif
