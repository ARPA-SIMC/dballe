#define _DBALLE_LIBRARY_CODE
#include "summary.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
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

#if 0
std::ostream& operator<<(std::ostream& out, const Entry& e)
{
    return out <<  "s:" << e.station
               << " l:" << e.level
               << " t:" << e.trange
               << " v:" << e.varcode
               << " d:" << e.dtrange
               << " c:" << e.count;
}
#endif

void VarEntry::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("l", var.level);
    writer.add("t", var.trange);
    writer.add("v", var.varcode);
    writer.add("d", dtrange);
    writer.add("c", count);
    writer.end_mapping();
}

VarEntry VarEntry::from_json(core::json::Stream& in)
{
    VarEntry res;
    in.parse_object([&](const std::string& key) {
        if (key == "l")
            res.var.level = in.parse_level();
        else if (key == "t")
            res.var.trange = in.parse_trange();
        else if (key == "v")
            res.var.varcode = in.parse_unsigned<unsigned short>();
        else if (key == "d")
            res.dtrange = in.parse_datetime_range();
        else if (key == "c")
            res.count = in.parse_unsigned<size_t>();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::VarEntry");
    });
    return res;
}

void VarEntry::dump(FILE* out) const
{
    char buf[7];
    format_code(var.varcode, buf);
    fprintf(out, "      Level: "); var.level.print(out);
    fprintf(out, "      Trange: "); var.trange.print(out);
    fprintf(out, "      Varcode: %s\n", buf);
    fprintf(out, "      Datetime range: ");
    dtrange.min.print_iso8601(out, 'T', " to ");
    dtrange.max.print_iso8601(out);
    fprintf(out, "      Count: %zd\n", count);
}


template<typename Station>
void StationEntry<Station>::add(const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    iterator i = find(vd);
    if (i != end())
        i->merge(dtrange, count);
    else
        SmallSet::add(VarEntry(vd, dtrange, count));
}

template<typename Station>
void StationEntry<Station>::add(const StationEntry& entries)
{
    for (const auto& entry: entries)
        add(entry.var, entry.dtrange, entry.count);
}

template<typename Station>
void StationEntry<Station>::add_filtered(const StationEntry& entries, const dballe::Query& query)
{
    const core::Query& q = core::Query::downcast(query);

    DatetimeRange wanted_dtrange = q.get_datetimerange();

    for (const auto& entry: entries)
    {
        if (!q.level.is_missing() && q.level != entry.var.level)
            continue;

        if (!q.trange.is_missing() && q.trange != entry.var.trange)
            continue;

        if (!q.varcodes.empty() && q.varcodes.find(entry.var.varcode) == q.varcodes.end())
            continue;

        if (!wanted_dtrange.contains(entry.dtrange))
            continue;

        add(entry.var, entry.dtrange, entry.count);
    }
}

template<typename Station>
void StationEntry<Station>::to_json(core::JSONWriter& writer) const
{
    writer.start_mapping();
    writer.add("s");
    station.to_json(writer);
    writer.add("v");
    writer.start_list();
    for (const auto entry: *this)
        entry.to_json(writer);
    writer.end_list();
    writer.end_mapping();
}

template<typename Station>
StationEntry<Station> StationEntry<Station>::from_json(core::json::Stream& in)
{
    StationEntry res;
    in.parse_object([&](const std::string& key) {
        if (key == "s")
            res.station = Station::from_json(in);
        else if (key == "v")
            in.parse_array([&]{
                res.add(VarEntry::from_json(in));
            });
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::StationEntry");
    });
    return res;
}

template<typename Station>
void StationEntry<Station>::dump(FILE* out) const
{
    fprintf(out, "   Station: "); station.print(out);
    fprintf(out, "   Vars:\n");
    for (const auto& entry: *this)
        entry.dump(out);
}


template<typename Station>
void StationEntries<Station>::add(const Station& station, const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    iterator cur = this->find(station);

    if (cur != end())
        cur->add(vd, dtrange, count);
    else
        Parent::add(StationEntry<Station>(station, vd, dtrange, count));
}

template<typename Station>
void StationEntries<Station>::add(const StationEntries& entries)
{
    for (auto entry: entries)
    {
        iterator cur = this->find(entry.station);
        if (cur != end())
            cur->add(entry);
        else
            Parent::add(entry);
    }
}

template<typename Station>
void StationEntries<Station>::add_filtered(const StationEntries& entries, const dballe::Query& query)
{
    // Scan the filter building a todo list of things to match
    const core::Query& q = core::Query::downcast(query);

    // If there is any filtering on the station, build a whitelist of matching stations
    bool has_flt_rep_memo = !q.rep_memo.empty();
    bool has_flt_ident = !q.ident.is_missing();
    bool has_flt_area = !q.latrange.is_missing() || !q.lonrange.is_missing();
    if (has_flt_rep_memo || has_flt_area || has_flt_ident)
    {
        LatRange flt_area_latrange = q.latrange;
        LonRange flt_area_lonrange = q.lonrange;
        for (auto entry: entries)
        {
            const Station& station = entry.station;
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

            iterator cur = this->find(entry.station);
            if (cur != end())
                cur->add_filtered(entry, query);
            else
                Parent::add(StationEntry<Station>(entry, query));
        }
    } else {
        for (auto entry: entries)
            Parent::add(StationEntry<Station>(entry, query));
    }
}

}


template<typename Station>
BaseSummary<Station>::BaseSummary()
{
}

template<typename Station>
void BaseSummary<Station>::recompute_summaries() const
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
void BaseSummary<Station>::add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
{
    entries.add(station, vd, dtrange, count);
    dirty = true;
}

template<typename Station>
void BaseSummary<Station>::add_cursor(const dballe::db::CursorSummary& cur)
{
    add(cur.get_station(), summary::VarDesc(cur.get_level(), cur.get_trange(), cur.get_varcode()), cur.get_datetimerange(), cur.get_count());
}

template<typename Station>
void BaseSummary<Station>::add_message(const dballe::Message& message)
{
    const Msg& msg = Msg::downcast(message);
    const msg::Context* l_ana = msg.find_context(Level(), Trange());

    Station station;

    // Latitude
    if (const wreport::Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        station.coords.lat = var->enqi();
    else
        throw wreport::error_notfound("latitude not found in message to summarise");

    // Longitude
    if (const wreport::Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        station.coords.lon = var->enqi();
    else
        throw wreport::error_notfound("longitude not found in message to summarise");

    // Report code
    if (const wreport::Var* var = msg.get_rep_memo_var())
        station.report = var->enqc();
    else
        station.report = Msg::repmemo_from_type(msg.type);

    // Station identifier
    if (const wreport::Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
        station.ident = var->enqc();

    // Datetime
    Datetime dt = msg.get_datetime();
    DatetimeRange dtrange(dt, dt);

    // TODO: obtain the StationEntry only once, and add the rest to it, to
    // avoid looking it up for each variable

    // Station variables
    summary::VarDesc vd_ana;
    vd_ana.level = Level();
    vd_ana.trange = Trange();
    for (size_t i = 0; i < l_ana->data.size(); ++i)
    {
        vd_ana.varcode = l_ana->data[i]->code();
        add(station, vd_ana, dtrange, 1);
    }

    // Variables
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        if (msg.data[i] == l_ana) continue;
        const msg::Context& ctx = *msg.data[i];

        summary::VarDesc vd(ctx.level, ctx.trange, 0);

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const wreport::Var* var = ctx.data[j];
            if (not var->isset()) continue;
            vd.varcode = var->code();
            add(station, vd, dtrange, 1);
        }
    }
}

template<typename Station>
void BaseSummary<Station>::add_messages(const dballe::Messages& messages)
{
    for (const auto& message: messages)
        add_message(message);
}

template<typename Station>
void BaseSummary<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    entries.add_filtered(summary.entries, query);
    dirty = true;
}

template<typename Station>
void BaseSummary<Station>::add_summary(const BaseSummary<Station>& summary)
{
    entries.add(summary.entries);
    dirty = true;
}

#if 0
void Summary::merge_entries()
{
    if (entries.size() < 2) return;

    std::sort(entries.begin(), entries.end());

    auto first = entries.begin();
    auto last = entries.end();
    auto tail = first;
    while (++first != last) {
        if (tail->same_metadata(*first))
            tail->merge(*first);
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
#endif

template<typename Station>
void BaseSummary<Station>::to_json(core::JSONWriter& writer) const
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
BaseSummary<Station> BaseSummary<Station>::from_json(core::json::Stream& in)
{
    BaseSummary<Station> summary;
    in.parse_object([&](const std::string& key) {
        if (key == "e")
            in.parse_array([&]{
                summary.entries.add(summary::StationEntry<Station>::from_json(in));
            });
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for summary::Entry");
    });
    summary.dirty = true;
    return summary;
}

template<typename Station>
void BaseSummary<Station>::dump(FILE* out) const
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

template class BaseSummary<dballe::Station>;
template class BaseSummary<dballe::DBStation>;

}
}
