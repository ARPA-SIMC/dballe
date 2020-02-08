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

template<typename Station>
CursorMemory<Station>::CursorMemory(const summary::StationEntries<Station>& entries, const Query& query)
{
    results.add_filtered(entries, query);
    for (const auto& s: results)
        _remaining += s.size();
}

template class CursorMemory<dballe::Station>;
template class CursorMemory<dballe::DBStation>;

}

template<typename Station>
BaseSummaryMemory<Station>::BaseSummaryMemory()
{
}

template<typename Station>
bool BaseSummaryMemory<Station>::stations(std::function<bool(const Station&)> dest) const
{
    if (dirty) recompute_summaries();
    entries.sorted();
    for (const auto& e: entries)
        if (!dest(e.station))
            return false;
    return true;
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
