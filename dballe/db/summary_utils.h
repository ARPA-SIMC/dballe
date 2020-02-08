#ifndef DBALLE_DB_SUMMARY_UTILS_H
#define DBALLE_DB_SUMMARY_UTILS_H

#include <dballe/db/summary.h>
#include <dballe/types.h>

namespace dballe {
namespace db {
namespace summary {

/**
 * Statistics about a variable
 */
struct VarEntry
{
    VarDesc var;

    dballe::DatetimeRange dtrange;
    size_t count = 0;

    VarEntry() = default;

    VarEntry(const VarDesc& var, const dballe::DatetimeRange& dtrange, size_t count)
        : var(var), dtrange(dtrange), count(count)
    {
    }

    VarEntry(const VarEntry&) = default;

    bool operator==(const VarEntry& o) const { return std::tie(var, dtrange, count) == std::tie(o.var, o.dtrange, o.count); }
    bool operator!=(const VarEntry& o) const { return std::tie(var, dtrange, count) != std::tie(o.var, o.dtrange, o.count); }

    void merge(const dballe::DatetimeRange& dtrange, size_t count)
    {
        this->dtrange.merge(dtrange);
        this->count += count;
    }

    void to_json(core::JSONWriter& writer) const;
    static VarEntry from_json(core::json::Stream& in);

    DBALLE_TEST_ONLY void dump(FILE* out) const;
};


inline const VarDesc& station_entry_get_value(const VarEntry& item) { return item.var; }

/**
 * Information about a station, and statistics about its variables.
 *
 * It behaves similarly to a std::vector<VarEntry>
 */
template<typename Station>
struct StationEntry : protected core::SmallSet<VarEntry, VarDesc, station_entry_get_value>
{
    using SmallSet::iterator;
    using SmallSet::const_iterator;
    using SmallSet::reverse_iterator;
    using SmallSet::const_reverse_iterator;
    using SmallSet::begin;
    using SmallSet::end;
    using SmallSet::rbegin;
    using SmallSet::rend;
    using SmallSet::size;
    using SmallSet::empty;
    using SmallSet::add;
    bool operator==(const StationEntry& o) const { return SmallSet::operator==(o); }
    bool operator!=(const StationEntry& o) const { return SmallSet::operator!=(o); }

    Station station;

    StationEntry() = default;

    template<typename OStation>
    StationEntry(const Station& station, const StationEntry<OStation>& entry)
        : station(station)
    {
        for (const auto& item: entry)
            this->add(item);
    }

    StationEntry(const Station& station, const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count)
        : station(station)
    {
        add(vd, dtrange, count);
    }

    StationEntry(const StationEntry& entries, const dballe::Query& query)
        : station(entries.station)
    {
        add_filtered(entries, query);
    }

    StationEntry(const StationEntry&) = default;

    void add(const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count);
    template<typename OStation>
    void add(const StationEntry<OStation>& entries);
    void add_filtered(const StationEntry& entries, const dballe::Query& query);
    bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const;

    void to_json(core::JSONWriter& writer) const;
    static StationEntry from_json(core::json::Stream& in);

    DBALLE_TEST_ONLY void dump(FILE* out) const;
};


template<typename Station>
inline const Station& station_entries_get_value(const StationEntry<Station>& item) { return item.station; }

/**
 * Index of all stations known to a summary
 *
 * It behaves similarly to a std::vector<StationEntry<Station>>
 */
template<typename Station>
struct StationEntries : protected core::SmallSet<StationEntry<Station>, Station, station_entries_get_value<Station>>
{
    typedef core::SmallSet<StationEntry<Station>, Station, station_entries_get_value<Station>> Parent;
    typedef typename Parent::iterator iterator;
    typedef typename Parent::const_iterator const_iterator;
    typedef typename Parent::reverse_iterator reverse_iterator;
    typedef typename Parent::const_reverse_iterator const_reverse_iterator;
    using Parent::begin;
    using Parent::end;
    using Parent::rbegin;
    using Parent::rend;
    using Parent::size;
    using Parent::empty;
    bool operator==(const StationEntries<Station>& o) const { return Parent::operator==(o); }
    bool operator!=(const StationEntries<Station>& o) const { return Parent::operator!=(o); }

    /// Merge the given entry
    void add(const Station& station, const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count);

    /// Merge the given entries
    template<typename OStation>
    void add(const StationEntries<OStation>& entry);

    /// Merge the given entries
    void add(const StationEntries<Station>& entry);

    /// Merge the given entry
    void add(const StationEntry<Station>& entry);

    void add_filtered(const StationEntries& entry, const dballe::Query& query);

    bool has(const Station& station) const { return this->find(station) != this->end(); }

    const StationEntries& sorted() const { if (this->dirty) this->rearrange_dirty(); return *this; }

    bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange& dtrange, size_t count)> dest) const;
};


extern template class StationEntry<dballe::Station>;
extern template class StationEntry<dballe::DBStation>;

extern template class StationEntries<dballe::Station>;
extern template void StationEntries<dballe::Station>::add(const StationEntries<dballe::DBStation>&);

extern template class StationEntries<dballe::DBStation>;
extern template void StationEntries<dballe::DBStation>::add(const StationEntries<dballe::Station>&);


template<typename S1, typename S2>
inline S1 convert_station(const S2& s)
{
    throw wreport::error_unimplemented("unsupported station conversion");
}

template<> inline Station convert_station<Station, Station>(const Station& s) { return s; }
template<> inline DBStation convert_station<DBStation, DBStation>(const DBStation& s) { return s; }

template<>
inline Station convert_station<Station, DBStation>(const DBStation& station)
{
    Station res(station);
    return res;
}

template<>
inline DBStation convert_station<DBStation, Station>(const Station& station)
{
    DBStation res;
    res.report = station.report;
    res.coords = station.coords;
    res.ident = station.ident;
    return res;
}


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

template<typename Station>
struct Cursor : public impl::CursorSummary
{
    summary::StationEntries<Station> results;
    typename summary::StationEntries<Station>::const_iterator station_entry;
    typename summary::StationEntry<Station>::const_iterator var_entry;
    bool at_start = true;
    int _remaining = 0;

    Cursor(const BaseSummary<Station>& summary, const Query& query);
    Cursor(const summary::StationEntries<Station>& entries, const Query& query);

    bool has_value() const { return !at_start && station_entry != results.end(); }

    int remaining() const override
    {
        return _remaining;
    }

    bool next() override
    {
        if (at_start)
        {
            station_entry = results.begin();
            if (station_entry != results.end())
                var_entry = station_entry->begin();
            at_start = false;
        } else if (station_entry == results.end())
            return false;
        else {
            if (var_entry != station_entry->end())
                ++var_entry;
            if (var_entry == station_entry->end())
            {
                ++station_entry;
                if (station_entry != results.end())
                {
                    var_entry = station_entry->begin();
                    --_remaining;
                }
            }
        }
        return station_entry != results.end();
    }

    void discard() override
    {
        station_entry = results.end();
    }

    static DBStation _get_dbstation(const DBStation& s) { return s; }
    static DBStation _get_dbstation(const dballe::Station& station)
    {
        DBStation res;
        res.report = station.report;
        res.coords = station.coords;
        res.ident = station.ident;
        return res;
    }
    static int _get_station_id(const DBStation& s) { return s.id; }
    static int _get_station_id(const dballe::Station& s) { return MISSING_INT; }

    DBStation get_station() const override
    {
        return _get_dbstation(station_entry->station);
    }

    Level get_level() const override { return var_entry->var.level; }
    Trange get_trange() const override { return var_entry->var.trange; }
    wreport::Varcode get_varcode() const override { return var_entry->var.varcode; }
    DatetimeRange get_datetimerange() const override { return var_entry->dtrange; }
    size_t get_count() const override { return var_entry->count; }

    void enq(impl::Enq& enq) const;
};

extern template class Cursor<dballe::Station>;
extern template class Cursor<dballe::DBStation>;

}
}
}

#endif
