#include "summary.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
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
        core::Record rec;
        cur.to_record(rec);
        count = rec["context_id"].enqi();
        dtrange = DatetimeRange(rec.get_datetimemin(), rec.get_datetimemax());
    }
}

}

Summary::Summary(const Query& query)
    : query(core::Query::downcast(query))
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

    const DatetimeRange& new_range = core::Query::downcast(query).datetime;
    const DatetimeRange& old_range = core::Query::downcast(this->query).datetime;

    // Check if the query has more restrictive datetime extremes
    if (old_range != new_range)
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
                if (new_range.contains(e.dtrange))
                    ; // If the query entirely contains this summary entry, we can still match it exactly
                else if (new_range.is_disjoint(e.dtrange))
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
            dtrange = val.dtrange;
            count = val.count;
        } else {
            dtrange.merge(val.dtrange);
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
