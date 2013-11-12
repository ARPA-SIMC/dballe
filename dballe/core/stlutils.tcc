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

template<typename T>
void Itersection<T>::sync_iters()
{
    using namespace std;

    while (true)
    {
        typename Sequences<T>::iterator i = this->sequences->begin();
        const T& candidate = (*i)->get();

        for ( ; i != this->sequences->end(); ++i)
        {
            while ((*i)->valid() && (*i)->get() < candidate)
                (*i)->next();
            // When we reach the end of a sequence, we are done
            if (!(*i)->valid())
            {
                this->clear();
                return;
            }
            if ((*i)->get() > candidate)
            {
                (*this->sequences)[0]->next();
                if (!(*this->sequences)[0]->valid())
                {
                    this->clear();
                    return;
                }
                // Continue, but at the while level
                goto next_round;
            }
        }

        // All sequences have the same item: stop.
        return;
        next_round: ;
    }
}

template<typename T>
void Iterunion<T>::find_min()
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
        for (typename Sequences<T>::iterator i = this->sequences->begin();
                i != this->sequences->end(); ++i)
        {
            if (!(*i)->valid()) continue;
            if ((*i)->get() == *minval)
                (*i)->next();
        }
    }

    // Set it to the minimum value.
    bool found = false;
    minval = 0;
    for (typename Sequences<T>::const_iterator i = this->sequences->begin();
            i != this->sequences->end(); ++i)
    {
        if (!(*i)->valid()) continue;
        if (!minval || (*i)->get() < *minval)
        {
            minval = &((*i)->get());
            found = true;
        }
    }

    if (!found)
    {
        this->clear();
        minval = 0;
    }
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
