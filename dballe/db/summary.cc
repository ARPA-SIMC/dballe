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

    // If query removed or changed a filter, then it can select more data than this summary knows about
    bool has_removed_or_changed = !this->query.iter_keys([&](dba_keyword key, const wreport::Var& val) {
        // Ignore changes in datetime and data-related filters
        if ((key >= DBA_KEY_YEAR && key <= DBA_KEY_SECMIN)
            || (key >= DBA_KEY_ANA_FILTER || key <= DBA_KEY_ATTR_FILTER)
            || key == DBA_KEY_QUERY)
            return true;
        // Check for deletions
        const char* new_val = query.key_peek_value(key);
        if (!new_val) return false;
        // If anything else has changed, report it
        if (strcmp(val.value(), new_val) != 0)
            return false;
        return true;
    });
    if (has_removed_or_changed)
        return Support::UNSUPPORTED;
/*
    // If query added a filter, then we can only give an approximate match
    bool has_added = !query.iter_keys([&](dba_keyword key, const wreport::Var& val) {
        qDebug() << "supports() HA" << key;
        return this->query.contains(key);
    });
    qDebug() << "supports() has_added" << has_added;
    */

    // Now we know that query has either more fields than this->query or changes
    // in datetime or data-related filters
    Support res = Support::EXACT;

    // Check differences in the value/attr based query filters
    for (const auto& k: { DBA_KEY_ANA_FILTER, DBA_KEY_DATA_FILTER, DBA_KEY_ATTR_FILTER })
    {
        // If query has introduced a filter, then this filter can only give approximate results
        const char* our_filter = this->query.key_peek_value(k);
        const char* new_filter = query.key_peek_value(k);

        if (our_filter)
        {
            if (new_filter)
            {
                // If they are different, query can return more values than we know of
                if (strcmp(our_filter, new_filter) != 0)
                    return Support::UNSUPPORTED;
                // If they are the same, then nothing has changed
            } else {
                // Filter has been removed
                // (this is caught in the diff above, but I repeat the branch here
                // to avoid leaving loose ends in the code)
                return Support::UNSUPPORTED;
            }
        } else {
            if (new_filter)
            {
                // If the query introduced a new filter, then we can only return approximate results
                res = Support::OVERESTIMATED;
            } else {
                ; // Nothing to do: both queries do not contain this filter
            }
        }
    }

    Datetime new_min;
    Datetime new_max;
    query.parse_date_extremes(new_min, new_max);

    Datetime our_min;
    Datetime our_max;
    this->query.parse_date_extremes(our_min, our_max);

    // datetime extremes should be the same, or query should have more restrictive extremes
    if (Datetime::range_equals(our_min, our_max, new_min, new_max))
    {
        ; // No change in datetime, good
    }
    else if (Datetime::range_contains(our_min, our_max, new_min, new_max))
    {
        if (count == MISSING_INT)
        {
            // We do not contain precise datetime information, so we cannot at
            // this point say anything better than "this summary may
            // overestimate the query"
            res = Support::OVERESTIMATED;
        } else if (res == Support::EXACT) {
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
    else
    {
        return Support::UNSUPPORTED;
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
