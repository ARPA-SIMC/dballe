/*
 * db/summary - High level code for working with DB-All.e DB summaries
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "summary.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

namespace summary {

Entry::Entry(dballe::db::Cursor &cur, bool want_details)
{
    ana_id = cur.get_station_id();
    rep_memo = cur.get_rep_memo();
    level = cur.get_level();
    trange = cur.get_trange();
    varcode = cur.get_varcode();
    if (want_details)
    {
        Record rec;
        cur.to_record(rec);
        count = rec[DBA_KEY_CONTEXT_ID].enqi();
        datemin = rec.get_datetimemin();
        datemax = rec.get_datetimemax();
    }
}

}

Summary::Summary(const Query& query)
    : query(query)
{
}

summary::Support Summary::supports(const Query& query) const
{
    using namespace summary;

    // If query is not just a restricted version of our query, then it can
    // select more data than this summary knows about.
    if (!query.is_subquery(this->query))
        return Support::UNSUPPORTED;

    // Now we know that query has either more fields than this->query or changes
    // in datetime or data-related filters
    Support res = Support::EXACT;

    // Check if the query has more restrictive datetime extremes
    Datetime new_min = query.datetime_min.lower_bound();
    Datetime new_max = query.datetime_max.upper_bound();
    Datetime our_min = this->query.datetime_min.lower_bound();
    Datetime our_max = this->query.datetime_max.upper_bound();
    if (new_min != our_min || new_max != our_max)
    {
        if (count == MISSING_INT)
        {
            // We do not contain precise datetime information, so we cannot at
            // this point say anything better than "this summary may
            // overestimate the query"
            res = Support::OVERESTIMATED;
        } else {
            // The query introduced further restrictions, check with the actual entries what we can do
            for (const auto& e: summary)
            {
                if (Datetime::range_contains(new_min, new_max, e.datemin, e.datemax))
                    ; // If the query entirely contains this summary entry, we can still match it exactly
                else if (Datetime::range_disjoint(new_min, new_max, e.datemin, e.datemax))
                    // If the query is completely outside of this entry, we can still match exactly
                    ;
                else
                {
                    // If the query instead only partially overlaps this entry,
                    // we may overestimate the results
                    res = Support::OVERESTIMATED;
                    break;
                }
            }
        }
    }

    return res;
}

void Summary::aggregate(const summary::Entry &val)
{
    all_stations.insert(val.ana_id);
    all_reports.insert(val.rep_memo);
    all_levels.insert(val.level);
    all_tranges.insert(val.trange);
    all_varcodes.insert(val.varcode);

    if (val.count != MISSING_INT)
    {
        if (count == MISSING_INT)
        {
            dtmin = val.datemin;
            dtmax = val.datemax;
            count = val.count;
        } else {
            if (val.datemin < dtmin) dtmin = val.datemin;
            if (val.datemax > dtmax) dtmax = val.datemax;
            count += val.count;
        }
    }

    valid = true;
}

void Summary::add_summary(dballe::db::Cursor &cur, bool with_details)
{
    summary.emplace_back(cur, with_details);
    aggregate(summary.back());
}

void Summary::add_entry(const summary::Entry &entry)
{
    summary.push_back(entry);
    aggregate(summary.back());
}

bool Summary::iterate(std::function<bool(const summary::Entry&)> f) const
{
    for (auto i: summary)
        if (!f(i))
            return false;
    return true;
}

namespace summary {

Summary& Stack::push(const Query& query)
{
    summaries.emplace_back(query);
    return summaries.back();
}

Support Stack::query(const Query& query, bool exact, std::function<bool(const Entry&)> match)
{
    if (summaries.empty())
    {
        // If we do not contain any summary, every query is unsupported
        return Support::UNSUPPORTED;
    }

    // Starting from the most specific, drop all summaries that do not support the current query
    while (summaries.size() > 1)
        summaries.pop_back();

    Support supported = summaries.back().supports(query);
    if (supported == UNSUPPORTED || (supported == OVERESTIMATED && exact))
        return supported;
    else
    {
        int last = summaries.size() - 1;
        summaries.emplace_back(query);
        const Summary& source = summaries[last];

        source.iterate([&](const Entry& entry) {
            if (match(entry))
                summaries.back().add_entry(entry);
            return true;
        });
        return supported;
    }
}

}

}
}
