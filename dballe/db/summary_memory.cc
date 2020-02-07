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

template<typename Station>
BaseSummaryMemory<Station>::BaseSummaryMemory()
{
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummaryMemory<Station>::query_summary(const Query& query) const
{
    return std::unique_ptr<dballe::CursorSummary>(new summary::Cursor<Station>(entries, query));
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
void BaseSummaryMemory<Station>::add_cursor(const dballe::CursorSummary& cur)
{
    add(cur.get_station(), summary::VarDesc(cur.get_level(), cur.get_trange(), cur.get_varcode()), cur.get_datetimerange(), cur.get_count());
}

template<typename Station>
void BaseSummaryMemory<Station>::add_message(const dballe::Message& message)
{
    const impl::Message& msg = impl::Message::downcast(message);

    Station station;

    // Coordinates
    station.coords = msg.get_coords();
    if (station.coords.is_missing())
        throw wreport::error_notfound("coordinates not found in message to summarise");

    // Report code
    station.report = msg.get_report();

    // Station identifier
    station.ident = msg.get_ident();

    // Datetime
    Datetime dt = msg.get_datetime();
    DatetimeRange dtrange(dt, dt);

    // TODO: obtain the StationEntry only once, and add the rest to it, to
    // avoid looking it up for each variable

    // Station variables
    summary::VarDesc vd_ana;
    vd_ana.level = Level();
    vd_ana.trange = Trange();
    for (const auto& val: msg.station_data)
    {
        vd_ana.varcode = val->code();
        add(station, vd_ana, dtrange, 1);
    }

    // Variables
    for (const auto& ctx: msg.data)
    {
        summary::VarDesc vd(ctx.level, ctx.trange, 0);

        for (const auto& val: ctx.values)
        {
            if (not val->isset()) continue;
            vd.varcode = val->code();
            add(station, vd, dtrange, 1);
        }
    }
}

template<typename Station>
void BaseSummaryMemory<Station>::add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages)
{
    for (const auto& message: messages)
        add_message(*message);
}

template<typename Station>
void BaseSummaryMemory<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    if (const BaseSummaryMemory<Station>* s = dynamic_cast<const BaseSummaryMemory<Station>*>(&summary))
    {
        entries.add_filtered(s->entries, query);
        dirty = true;
    } else {
        throw wreport::error_unimplemented("add_filtered from a different summary type");
    }
}

template<typename Station>
void BaseSummaryMemory<Station>::add_summary(const BaseSummary<dballe::Station>& summary)
{
    entries.add(summary.stations());
    dirty = true;
}

template<typename Station>
void BaseSummaryMemory<Station>::add_summary(const BaseSummary<dballe::DBStation>& summary)
{
    entries.add(summary.stations());
    dirty = true;
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
// template void BaseSummaryMemory<dballe::Station>::add_summary(const BaseSummary<dballe::Station>&);
// template void BaseSummaryMemory<dballe::Station>::add_summary(const BaseSummary<dballe::DBStation>&);
template class BaseSummaryMemory<dballe::DBStation>;
// template void BaseSummaryMemory<dballe::DBStation>::add_summary(const BaseSummary<dballe::Station>&);
// template void BaseSummaryMemory<dballe::DBStation>::add_summary(const BaseSummary<dballe::DBStation>&);

}
}
