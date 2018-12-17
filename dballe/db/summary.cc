#define _DBALLE_LIBRARY_CODE
#include "summary.h"
#include "dballe/core/var.h"
#include "dballe/core/query.h"
#include "dballe/core/json.h"
#include "dballe/core/fortran.h"
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
            res.dtrange = in.parse_datetimerange();
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

template<typename Station> template<typename OStation>
void StationEntry<Station>::add(const StationEntry<OStation>& entries)
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
    writer.add(station);
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
            res.station = in.parse<Station>();
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
void StationEntries<Station>::add(const StationEntries<Station>& entries)
{
    for (const auto& entry: entries)
        add(entry);
}

template<typename Station>
void StationEntries<Station>::add(const StationEntry<Station>& entry)
{
    iterator cur = this->find(entry.station);
    if (cur != end())
        cur->add(entry);
    else
        Parent::add(entry);
}


namespace {
Station convert_station(const DBStation& station)
{
    Station res(station);
    return res;
}
DBStation convert_station(const Station& station)
{
    DBStation res;
    res.report = station.report;
    res.coords = station.coords;
    res.ident = station.ident;
    return res;
}
}

template<typename Station> template<typename OStation>
void StationEntries<Station>::add(const StationEntries<OStation>& entries)
{
    for (const auto& entry: entries)
    {
        Station station = convert_station(entry.station);
        iterator cur = this->find(station);
        if (cur != end())
            cur->add(entry);
        else
            Parent::add(StationEntry<Station>(station, entry));
    }
}

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
void StationEntries<Station>::add_filtered(const StationEntries& entries, const dballe::Query& query)
{
    StationFilter<Station> filter(query);

    if (filter.has_flt_station)
    {
        for (auto entry: entries)
        {
            if (!filter.matches_station(entry.station))
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


template<typename Station>
Cursor<Station>::Cursor(const summary::StationEntries<Station>& entries, const Query& query)
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

template<typename Station>
bool Cursor<Station>::enqi(const char* key, unsigned len, int& res) const
{
    impl::Enqi enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

template<typename Station>
bool Cursor<Station>::enqd(const char* key, unsigned len, double& res) const
{
    impl::Enqd enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

template<typename Station>
bool Cursor<Station>::enqs(const char* key, unsigned len, std::string& res) const
{
    impl::Enqs enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

template<typename Station>
bool Cursor<Station>::enqf(const char* key, unsigned len, std::string& res) const
{
    impl::Enqf enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}


template class Cursor<dballe::Station>;
template class Cursor<dballe::DBStation>;

}


template<typename Station>
BaseSummary<Station>::BaseSummary()
{
}

template<typename Station>
std::unique_ptr<dballe::CursorSummary> BaseSummary<Station>::query_summary(const Query& query) const
{
    return std::unique_ptr<dballe::CursorSummary>(new summary::Cursor<Station>(entries, query));
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
void BaseSummary<Station>::add_cursor(const dballe::CursorSummary& cur)
{
    add(cur.get_station(), summary::VarDesc(cur.get_level(), cur.get_trange(), cur.get_varcode()), cur.get_datetimerange(), cur.get_count());
}

template<typename Station>
void BaseSummary<Station>::add_message(const dballe::Message& message)
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
void BaseSummary<Station>::add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages)
{
    for (const auto& message: messages)
        add_message(*message);
}

template<typename Station>
void BaseSummary<Station>::add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query)
{
    entries.add_filtered(summary.entries, query);
    dirty = true;
}

template<typename Station> template<typename OStation>
void BaseSummary<Station>::add_summary(const BaseSummary<OStation>& summary)
{
    entries.add(summary.stations());
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
void BaseSummary<Station>::load_json(core::json::Stream& in)
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
template void BaseSummary<dballe::Station>::add_summary(const BaseSummary<dballe::Station>&);
template void BaseSummary<dballe::Station>::add_summary(const BaseSummary<dballe::DBStation>&);
template class BaseSummary<dballe::DBStation>;
template void BaseSummary<dballe::DBStation>::add_summary(const BaseSummary<dballe::Station>&);
template void BaseSummary<dballe::DBStation>::add_summary(const BaseSummary<dballe::DBStation>&);

}
}

#include "summary-access.tcc"
