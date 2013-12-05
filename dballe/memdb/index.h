/*
 * memdb/index - map-based index for memdb
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

#ifndef DBA_MEMDB_INDEX_H
#define DBA_MEMDB_INDEX_H

#include <set>
#include <map>
#include <cstddef>

namespace dballe {

namespace stl {
template<typename T> class SetIntersection;
template<typename T> class Sequences;
}

namespace memdb {

/// Index element positions based by one value
template<typename T>
struct Index : public std::map< T, std::set<size_t> >
{
    typedef typename std::map< T, std::set<size_t> >::const_iterator const_iterator;

    /// Lookup all positions for a value
    std::set<size_t> search(const T& el) const;

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

    /**
     * Lookup all positions for a value and all values after it, appending the
     * results to a Sequences
     *
     * @returns false if el was not found; it leaves out untouched in that case
     */
    bool search_from(const T& first, stl::Sequences<size_t>& out) const;

    /**
     * Lookup all positions all values before the given one, appending the
     * results to a Sequences
     *
     * @returns false if el was not found; it leaves out untouched in that case
     */
    bool search_to(const T& end, stl::Sequences<size_t>& out) const;

    /**
     * Lookup all positions all values between two extremes (first included,
     * second excluded), appending the results to a Sequences
     *
     * @returns false if el was not found; it leaves out untouched in that case
     */
    bool search_between(const T& first, const T& end, stl::Sequences<size_t>& out) const;

#if 0
    void query(const const_iterator& begin, const const_iterator& end, const Match<size_t>& filter, BaseResults& res) const;
    void query(const const_iterator& begin, const const_iterator& end, BaseResults& res) const;
    void query(const const_iterator& begin, const const_iterator& end, const Match<size_t>* filter, BaseResults& res) const;
#endif
};

}
}

#endif

