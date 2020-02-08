#define _DBALLE_LIBRARY_CODE
#include "summary_utils.h"
#include "dballe/core/json.h"

namespace dballe {
namespace db {
namespace summary {

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
bool StationEntry<Station>::iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    const core::Query& q = core::Query::downcast(query);

    DatetimeRange wanted_dtrange = q.get_datetimerange();

    for (const auto& entry: *this)
    {
        if (!q.level.is_missing() && q.level != entry.var.level)
            continue;

        if (!q.trange.is_missing() && q.trange != entry.var.trange)
            continue;

        if (!q.varcodes.empty() && q.varcodes.find(entry.var.varcode) == q.varcodes.end())
            continue;

        if (!wanted_dtrange.contains(entry.dtrange))
            continue;

        if (!dest(station, entry.var, entry.dtrange, entry.count))
            return false;
    }
    return true;
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

template<typename Station> template<typename OStation>
void StationEntries<Station>::add(const StationEntries<OStation>& entries)
{
    for (const auto& entry: entries)
    {
        Station station = convert_station<Station, OStation>(entry.station);
        iterator cur = this->find(station);
        if (cur != end())
            cur->add(entry);
        else
            Parent::add(StationEntry<Station>(station, entry));
    }
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
bool StationEntries<Station>::iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const
{
    StationFilter<Station> filter(query);

    if (filter.has_flt_station)
    {
        for (auto entry: *this)
        {
            if (!filter.matches_station(entry.station))
                continue;

            if (!entry.iter_filtered(query, dest))
                return false;
        }
    } else {
        for (auto entry: *this)
            if (!entry.iter_filtered(query, dest))
                return false;
    }
    return true;
}


template class StationEntry<dballe::Station>;
template class StationEntry<dballe::DBStation>;
template class StationEntries<dballe::Station>;
template void StationEntries<dballe::Station>::add(const StationEntries<dballe::DBStation>&);
template class StationEntries<dballe::DBStation>;
template void StationEntries<dballe::DBStation>::add(const StationEntries<dballe::Station>&);


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
}
}
