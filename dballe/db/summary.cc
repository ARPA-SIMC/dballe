#include "summary.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {
namespace summary {

Entry::Entry(dballe::db::CursorSummary& cur)
{
    station = cur.get_station();
    level = cur.get_level();
    trange = cur.get_trange();
    varcode = cur.get_varcode();
    count = cur.get_count();
    dtrange = cur.get_datetimerange();
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
            for (const auto& e: entries)
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
    all_stations.insert(make_pair(val.station.ana_id, val.station));
    all_reports.insert(val.station.report);
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

void Summary::add_filtered(const Summary& summary)
{
    switch (summary.supports(query))
    {
        case summary::Support::UNSUPPORTED:
            throw std::runtime_error("Source summary does not support the query for this summary");
        case summary::Support::OVERESTIMATED:
        case summary::Support::EXACT:
            break;
    }

    const core::Query& q = core::Query::downcast(query);

    // Scan the filter building a todo list of things to match

    // If there is any filtering on the station, build a whitelist of matching stations
    bool has_flt_station;
    std::set<int> wanted_stations;
    bool has_flt_ident = !q.ident.is_missing();
    bool has_flt_area = !q.get_latrange().is_missing() || !q.get_lonrange().is_missing();
    if (has_flt_ident || has_flt_area || q.ana_id != MISSING_INT)
    {
        LatRange flt_area_latrange = q.get_latrange();
        LonRange flt_area_lonrange = q.get_lonrange();
        has_flt_station = true;
        for (auto s: summary.all_stations)
        {
            const Station& station = s.second;
            if (q.ana_id != MISSING_INT && station.ana_id != q.ana_id)
                continue;

            if (has_flt_area)
            {
                if (!flt_area_latrange.contains(station.coords.lat) ||
                    !flt_area_lonrange.contains(station.coords.lon))
                    continue;
            }
            if (has_flt_ident && query.ident != station.ident)
                continue;

            wanted_stations.insert(station.ana_id);
        }
    }

    bool has_flt_rep_memo = !query.rep_memo.empty();
    bool has_flt_level = !query.level.is_missing();
    bool has_flt_trange = !query.trange.is_missing();
    bool has_flt_varcode = !query.varcodes.empty();
    wreport::Varcode wanted_varcode = has_flt_varcode ? *query.varcodes.begin() : 0;
    DatetimeRange wanted_dtrange = query.get_datetimerange();

    for (const auto& entry: summary.entries)
    {
        if (has_flt_station && wanted_stations.find(entry.station.ana_id) == wanted_stations.end())
            continue;

        if (has_flt_rep_memo && query.rep_memo != entry.station.report)
            continue;

        if (has_flt_level && query.level != entry.level)
            continue;

        if (has_flt_trange && query.trange != entry.trange)
            continue;

        if (has_flt_varcode && wanted_varcode != entry.varcode)
            continue;

        if (!wanted_dtrange.contains(entry.dtrange))
            continue;

        add_entry(entry);
    }
}

void Summary::add_summary(dballe::db::CursorSummary& cur)
{
    entries.emplace_back(cur);
    aggregate(entries.back());
}

void Summary::add_entry(const summary::Entry &entry)
{
    entries.push_back(entry);
    aggregate(entries.back());
}

bool Summary::iterate(std::function<bool(const summary::Entry&)> f) const
{
    for (auto i: entries)
        if (!f(i))
            return false;
    return true;
}

}
}
