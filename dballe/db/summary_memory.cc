#define _DBALLE_LIBRARY_CODE
#include "summary_memory.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "dballe/core/json.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <algorithm>
#include <unordered_set>
#include <cstring>

using namespace std;
using namespace dballe;

namespace dballe {
namespace db {

namespace summary {

namespace {

struct StationFilterBase
{
    const core::Query& q;
    bool has_flt_rep_memo;
    bool has_flt_ident;
    bool has_flt_area;
    bool has_flt_station;

    StationFilterBase(const dballe::Query& query)
        : q(core::Query::downcast(query))
    {
        // Scan the filter building a todo list of things to match

        // If there is any filtering on the station, build a whitelist of matching stations
        has_flt_rep_memo = !q.report.empty();
        has_flt_ident = !q.ident.is_missing();
        has_flt_area = !q.latrange.is_missing() || !q.lonrange.is_missing();
        has_flt_station = has_flt_rep_memo || has_flt_area || has_flt_ident;
    }

    template<typename Station>
    bool matches_station(const Station& station)
    {
        if (has_flt_area)
        {
            if (!q.latrange.contains(station.coords.lat) ||
                !q.lonrange.contains(station.coords.lon))
                return false;
        }

        if (has_flt_rep_memo && q.report != station.report)
            return false;

        if (has_flt_ident && q.ident != station.ident)
            return false;

        return true;
    }
};

template<class Station>
struct StationFilter;

template<>
struct StationFilter<dballe::Station> : public StationFilterBase
{
    using StationFilterBase::StationFilterBase;
    bool matches_station(const Station& station)
    {
        return StationFilterBase::matches_station(station);
    }
};

template<>
struct StationFilter<dballe::DBStation> : public StationFilterBase
{
    StationFilter(const dballe::Query& query)
        : StationFilterBase(query)
    {
        has_flt_station |= (q.ana_id != MISSING_INT);
    }

    bool matches_station(const DBStation& station)
    {
        if (q.ana_id != MISSING_INT and station.id != q.ana_id)
            return false;
        return StationFilterBase::matches_station(station);
    }
};

}

template<typename Station>
CursorMemory<Station>::CursorMemory(const summary::StationEntries<Station>& entries, const Query& query)
{
    const core::Query& q = core::Query::downcast(query);

    summary::StationFilter<Station> filter(query);
    DatetimeRange wanted_dtrange = q.get_datetimerange();

    for (const auto& station_entry: entries)
    {
        if (filter.has_flt_station && !filter.matches_station(station_entry.station))
            continue;

        for (const auto& var_entry: station_entry)
        {
            if (!q.level.is_missing() && q.level != var_entry.var.level)
                continue;

            if (!q.trange.is_missing() && q.trange != var_entry.var.trange)
                continue;

            if (!q.varcodes.empty() && q.varcodes.find(var_entry.var.varcode) == q.varcodes.end())
                continue;

            if (!wanted_dtrange.contains(var_entry.dtrange))
                continue;

            results.emplace_back(station_entry, var_entry);
        }
    }
}

template class CursorMemory<dballe::Station>;
template class CursorMemory<dballe::DBStation>;

}

template<typename Station>
BaseSummaryMemory<Station>::BaseSummaryMemory()
{
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummaryMemory<Station>::query_summary(const Query& query) const
{
    return std::unique_ptr<dballe::CursorSummary>(new summary::CursorMemory<Station>(entries, query));
}

template<typename Station>
bool BaseSummaryMemory<Station>::iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    for (const auto& se: entries)
        for (const auto& ve: se)
            if (!dest(se.station, ve.var, ve.dtrange, ve.count))
                return false;
    return true;
}

template<typename Station>
bool BaseSummaryMemory<Station>::iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    return entries.iter_filtered(query, dest);
}

template<typename Station>
void BaseSummaryMemory<Station>::recompute_summaries() const
{
    bool first = true;
    for (const auto& station_entry: entries)
    {
        m_reports.add(station_entry.station.report);
        for (const auto& var_entry: station_entry)
        {
            m_levels.add(var_entry.var.level);
            m_tranges.add(var_entry.var.trange);
            m_varcodes.add(var_entry.var.varcode);
            if (first)
            {
                first = false;
                dtrange = var_entry.dtrange;
                count = var_entry.count;
            } else {
                dtrange.merge(var_entry.dtrange);
                count += var_entry.count;
            }
        }
    }
    if (first)
    {
        dtrange = DatetimeRange();
        count = 0;
    }
    dirty = false;
}

template<typename Station>
void BaseSummaryMemory<Station>::add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    entries.add(station, vd, dtrange, count);
    dirty = true;
}

template<typename Station>
void BaseSummaryMemory<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    if (const BaseSummaryMemory<Station>* s = dynamic_cast<const BaseSummaryMemory<Station>*>(&summary))
    {
        entries.add_filtered(s->entries, query);
        dirty = true;
    } else {
        BaseSummary<Station>::add_filtered(summary, query);
    }
}

template<typename Station>
void BaseSummaryMemory<Station>::add_summary(const BaseSummary<dballe::Station>& summary)
{
    if (const BaseSummaryMemory<dballe::Station>* s = dynamic_cast<const BaseSummaryMemory<dballe::Station>*>(&summary))
    {
        entries.add(s->_entries());
        dirty = true;
    } else {
        BaseSummary<Station>::add_summary(summary);
    }
}

template<typename Station>
void BaseSummaryMemory<Station>::add_summary(const BaseSummary<dballe::DBStation>& summary)
{
    if (const BaseSummaryMemory<DBStation>* s = dynamic_cast<const BaseSummaryMemory<dballe::DBStation>*>(&summary))
    {
        entries.add(s->_entries());
        dirty = true;
    } else {
        BaseSummary<Station>::add_summary(summary);
    }
}

template<typename Station>
void BaseSummaryMemory<Station>::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("e");
    writer.start_list();
    for (const auto& e: entries)
        e.to_json(writer);
    writer.end_list();
    writer.end_mapping();
}

template<typename Station>
void BaseSummaryMemory<Station>::load_json(core::json::Stream& in)
{
    in.parse_object([&](const std::string& key) {
        if (key == "e")
            in.parse_array([&]{
                entries.add(summary::StationEntry<Station>::from_json(in));
            });
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
    dirty = true;
}

template<typename Station>
void BaseSummaryMemory<Station>::dump(FILE* out) const
{
    fprintf(out, "Summary:\n");
    fprintf(out, "Stations:\n");
    for (const auto& entry: entries)
        entry.dump(out);
    fprintf(out, "Reports:\n");
    for (const auto& val: m_reports)
        fprintf(out, " - %s\n", val.c_str());
    fprintf(out, "Levels:\n");
    for (const auto& val: m_levels)
    {
        fprintf(out, " - ");
        val.print(out);
    }
    fprintf(out, "Tranges:\n");
    for (const auto& val: m_tranges)
    {
        fprintf(out, " - ");
        val.print(out);
    }
    fprintf(out, "Varcodes:\n");
    for (const auto& val: m_varcodes)
    {
        char buf[7];
        format_code(val, buf);
        fprintf(out, " - %s\n", buf);
    }
    fprintf(out, "Datetime range: ");
    dtrange.min.print_iso8601(out, 'T', " to ");
    dtrange.max.print_iso8601(out);
    fprintf(out, "Count: %zd\n", count);
    fprintf(out, "Dirty: %s\n", dirty ? "true" : "false");
}

template class BaseSummaryMemory<dballe::Station>;
template class BaseSummaryMemory<dballe::DBStation>;

}
}
