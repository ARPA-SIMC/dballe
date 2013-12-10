/*
 * memdb/match - Record-by-record match
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

#ifndef DBA_MEMDB_MATCH_H
#define DBA_MEMDB_MATCH_H

#include <dballe/core/stlutils.h>
#include <vector>

namespace dballe {
namespace memdb {

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

}
}
}

#endif



