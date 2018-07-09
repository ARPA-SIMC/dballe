#include "summary.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/json.h"
#include <algorithm>
#include <unordered_set>
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

void Entry::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("s");
    writer.start_mapping();
    writer.add("r", station.report);
    writer.add("c", station.coords);
    writer.add("i", station.ident);
    writer.end_mapping();
    writer.add("l", level);
    writer.add("t", trange);
    writer.add("v", varcode);
    writer.add("d", dtrange);
    writer.add("c", count);
    writer.end_mapping();
}

static Station station_from_json(core::json::Stream& in)
{
    Station res;
    in.parse_object([&](const std::string& key) {
        if (key == "r")
            res.report = in.parse_string();
        else if (key == "c")
            res.coords = in.parse_coords();
        else if (key == "i")
            res.ident = in.parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for Station");
    });
    return res;
}

Entry Entry::from_json(core::json::Stream& in)
{
    Entry res;
    in.parse_object([&](const std::string& key) {
        if (key == "s")
            res.station = station_from_json(in);
        else if (key == "l")
            res.level = in.parse_level();
        else if (key == "t")
            res.trange = in.parse_trange();
        else if (key == "v")
            res.varcode = in.parse_unsigned<unsigned short>();
        else if (key == "d")
            res.dtrange = in.parse_datetime_range();
        else if (key == "c")
            res.count = in.parse_unsigned<size_t>();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
    return res;
}

std::ostream& operator<<(std::ostream& out, const Entry& e)
{
    return out <<  "s:" << e.station
               << " l:" << e.level
               << " t:" << e.trange
               << " v:" << e.varcode
               << " d:" << e.dtrange
               << " c:" << e.count;
}

}


Summary::Summary()
{
}

Summary::Summary(std::vector<summary::Entry>&& entries)
    : entries(std::move(entries))
{
    for (const auto& e: this->entries)
        aggregate(e);
}

void Summary::aggregate(const summary::Entry &val)
{
    all_stations.insert(make_pair(val.station.id, val.station));
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

void Summary::add_cursor(dballe::db::CursorSummary& cur)
{
    entries.emplace_back(cur);
    aggregate(entries.back());
}

void Summary::add_entry(const summary::Entry &entry)
{
    entries.push_back(entry);
    aggregate(entries.back());
}

void Summary::add_summary(const Summary& summary)
{
    bool was_empty = entries.empty();
    for (const auto& entry: summary.entries)
        add_entry(entry);
    if (!was_empty)
        merge_entries();
}

void Summary::merge_entries()
{
    if (entries.size() < 2) return;

    std::sort(entries.begin(), entries.end());

    auto first = entries.begin();
    auto last = entries.end();
    auto tail = first;
    while (++first != last) {
        if (tail->same_metadata(*first))
            tail->count += first->count;
        else if (++tail != first)
            *tail = std::move(*first);
    }

    ++tail;

    if (tail != last)
        entries.erase(tail, last);
}

bool Summary::iterate(std::function<bool(const summary::Entry&)> f) const
{
    for (auto entry: entries)
        if (!f(entry))
            return false;
    return true;
}

bool Summary::iterate_filtered(const Query& query, std::function<bool(const summary::Entry&)> f) const
{
    // Scan the filter building a todo list of things to match

    const core::Query& q = core::Query::downcast(query);

    // If there is any filtering on the station, build a whitelist of matching stations
    bool has_flt_rep_memo = !q.rep_memo.empty();
    std::unordered_set<int> wanted_stations;
    bool has_flt_ident = !q.ident.is_missing();
    bool has_flt_area = !q.get_latrange().is_missing() || !q.get_lonrange().is_missing();
    bool has_flt_station = has_flt_ident || has_flt_area || q.ana_id != MISSING_INT;
    if (has_flt_station)
    {
        LatRange flt_area_latrange = q.get_latrange();
        LonRange flt_area_lonrange = q.get_lonrange();
        for (auto s: all_stations)
        {
            const Station& station = s.second;
            if (q.ana_id != MISSING_INT && station.id != q.ana_id)
                continue;

            if (has_flt_area)
            {
                if (!flt_area_latrange.contains(station.coords.lat) ||
                    !flt_area_lonrange.contains(station.coords.lon))
                    continue;
            }

            if (has_flt_rep_memo && q.rep_memo != station.report)
                continue;

            if (has_flt_ident && q.ident != station.ident)
                continue;

            wanted_stations.insert(station.id);
        }
    }

    bool has_flt_level = !q.level.is_missing();
    bool has_flt_trange = !q.trange.is_missing();
    bool has_flt_varcode = !q.varcodes.empty();
    wreport::Varcode wanted_varcode = has_flt_varcode ? *q.varcodes.begin() : 0;
    DatetimeRange wanted_dtrange = q.get_datetimerange();

    for (const auto& entry: entries)
    {
        if (has_flt_station)
        {
            if (wanted_stations.find(entry.station.id) == wanted_stations.end())
                continue;
        } else if (has_flt_rep_memo && q.rep_memo != entry.station.report)
            continue;

        if (has_flt_level && q.level != entry.level)
            continue;

        if (has_flt_trange && q.trange != entry.trange)
            continue;

        if (has_flt_varcode && wanted_varcode != entry.varcode)
            continue;

        if (!wanted_dtrange.contains(entry.dtrange))
            continue;

        if (!f(entry))
            return false;
    }

    return true;
}

void Summary::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("e");
    writer.start_list();
    for (const auto& e: entries)
        e.to_json(writer);
    writer.end_list();
    writer.end_mapping();
}

Summary Summary::from_json(core::json::Stream& in)
{
    std::vector<summary::Entry> entries;
    in.parse_object([&](const std::string& key) {
        if (key == "e")
            in.parse_array([&]{
                entries.emplace_back(summary::Entry::from_json(in));
            });
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
    return Summary(std::move(entries));
}

}
}
