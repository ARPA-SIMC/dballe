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
#ifndef DBA_CORE_STLUTILS_TCC
#define DBA_CORE_STLUTILS_TCC

namespace dballe {
namespace stl {
namespace stlutils {

template<typename ITER>
bool Itersection<ITER>::sync_iters()
{
    using namespace std;

    while (true)
    {
        typename std::vector< Sequence<ITER> >::iterator i = iters.begin();
        const typename ITER::value_type& candidate = **i;

        for ( ; i != iters.end(); ++i)
        {
            while (i->valid() && **i < candidate)
                i->next();
            // When we reach the end of a sequence, we are done
            if (!i->valid())
                return false;
            if (**i > candidate)
            {
                iters[0].next();
                if (!iters[0].valid())
                    return false;
                // Continue at the while level
                goto next_round;
            }
        }

        // All sequences have the same item: stop.
        return true;
        next_round: ;
    }
}

template<typename ITER>
bool Iterunion<ITER>::find_min()
{
    /**
     * If minval is 0, set it to the minimum value.
     * If minval points to an element, advance all sequences that have that
     * element as minimum value, and set minval to the next minimum value.
     *
     * Returns false when all sequences are exausted; true if a new minimum
     * value was found.
     */
    if (minval)
    {
        // Advance all sequences that have that element as minimum value, and
        // set minval to the next minimum value.

        // Advance all iterators that point to the minimum value
        for (typename std::vector< Sequence<ITER> >::iterator i = iters.begin();
                i != iters.end(); ++i)
        {
            if (!i->valid()) continue;
            if (**i == *minval)
                i->next();
        }
    }

    // Set it to the minimum value.
    bool found = false;
    minval = 0;
    for (typename std::vector< Sequence<ITER> >::const_iterator i = iters.begin();
            i != iters.end(); ++i)
    {
        if (!i->valid()) continue;
        if (!minval || **i < *minval)
        {
            minval = &**i;
            found = true;
        }
    }

    return found;
#if 0
        for (typename std::vector< Sequence<ITER> >::const_iterator i = iters.begin();
                i != iters.end(); )
        {
            if (**i != *minval)
            {
                ++i;
            } else {
                // Same as minval: advance it
                i->next();
                if (i->valid) {
                    ++i;
                    continue;
                } else if (i == iters.rbegin()) {
                    TODO_reduce_by_one();
                    break;
                } else {
                    TODO_swap_last_to_here();
                    // do not advance i
                }
            }
        }
#endif
}

}
}
}

#endif
