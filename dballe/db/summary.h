#ifndef DBALLE_DB_SUMMARY_H
#define DBALLE_DB_SUMMARY_H

#include <dballe/core/fwd.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include <dballe/core/values.h>
#include <dballe/core/smallset.h>
#include <dballe/db/db.h>
#include <vector>
#include <set>
#include <functional>

namespace dballe {
namespace db {
namespace summary {

struct VarDesc
{
    dballe::Level level;
    dballe::Trange trange;
    wreport::Varcode varcode;

    VarDesc() = default;

    VarDesc(const dballe::Level& level, const dballe::Trange& trange, wreport::Varcode varcode)
        : level(level), trange(trange), varcode(varcode) {}

    VarDesc(const VarDesc&) = default;

    bool operator==(const VarDesc& o) const { return std::tie(level, trange, varcode) == std::tie(o.level, o.trange, o.varcode); }
    bool operator!=(const VarDesc& o) const { return std::tie(level, trange, varcode) != std::tie(o.level, o.trange, o.varcode); }
    bool operator< (const VarDesc& o) const { return std::tie(level, trange, varcode) <  std::tie(o.level, o.trange, o.varcode); }
    bool operator<=(const VarDesc& o) const { return std::tie(level, trange, varcode) <= std::tie(o.level, o.trange, o.varcode); }
    bool operator> (const VarDesc& o) const { return std::tie(level, trange, varcode) >  std::tie(o.level, o.trange, o.varcode); }
    bool operator>=(const VarDesc& o) const { return std::tie(level, trange, varcode) >= std::tie(o.level, o.trange, o.varcode); }
};

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
    void add(const StationEntry& entries);
    void add_filtered(const StationEntry& entries, const dballe::Query& query);

    void to_json(core::JSONWriter& writer) const;
    static StationEntry from_json(core::json::Stream& in);

    DBALLE_TEST_ONLY void dump(FILE* out) const;
};


#if 0
    bool same_metadata(const Entry& o) const
    {
        return std::tie(station, level, trange, varcode) ==
               std::tie(o.station, o.level, o.trange, o.varcode);
    }

    void merge(const Entry& o)
    {
        dtrange.merge(o.dtrange);
        count += o.count;
    }

#endif

#if 0
std::ostream& operator<<(std::ostream& out, const Entry& e);
#endif

template<typename Station>
inline const Station& station_entries_get_value(const StationEntry<Station>& item) { return item.station; }

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
    using Parent::add;
    bool operator==(const StationEntries<Station>& o) const { return Parent::operator==(o); }
    bool operator!=(const StationEntries<Station>& o) const { return Parent::operator!=(o); }

    void add(const Station& station, const VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count);
    void add(const StationEntries& entry);
    void add_filtered(const StationEntries& entry, const dballe::Query& query);

    bool has(const Station& station) const { return this->find(station) != this->end(); }

    const StationEntries& sorted() const { if (this->dirty) this->rearrange_dirty(); return *this; }
};

}


/**
 * High level objects for working with DB-All.e DB summaries
 */
template<typename Station>
class BaseSummary
{
protected:
    // Summary of items for the currently active filter
    summary::StationEntries<Station> entries;

    mutable core::SortedSmallUniqueValueSet<std::string> m_reports;
    mutable core::SortedSmallUniqueValueSet<dballe::Level> m_levels;
    mutable core::SortedSmallUniqueValueSet<dballe::Trange> m_tranges;
    mutable core::SortedSmallUniqueValueSet<wreport::Varcode> m_varcodes;
    mutable dballe::DatetimeRange dtrange;
    mutable size_t count = 0;

    mutable bool dirty = false;

    void recompute_summaries() const;

public:
    BaseSummary();

    bool operator==(const BaseSummary& o) const
    {
        return entries == o.entries;
    }

    const summary::StationEntries<Station>& stations() const { if (dirty) recompute_summaries(); return entries.sorted(); }
    core::SortedSmallUniqueValueSet<std::string> reports() const { if (dirty) recompute_summaries(); return m_reports; }
    core::SortedSmallUniqueValueSet<dballe::Level> levels() const { if (dirty) recompute_summaries(); return m_levels; }
    core::SortedSmallUniqueValueSet<dballe::Trange> tranges() const { if (dirty) recompute_summaries(); return m_tranges; }
    core::SortedSmallUniqueValueSet<wreport::Varcode> varcodes() const { if (dirty) recompute_summaries(); return m_varcodes; }

    /**
     * Recompute reports, levels, tranges, and varcodes.
     *
     * Call this after performing changes to the summary, to make those sets
     * valid before reading them.
     */
    const Datetime& datetime_min() const { if (dirty) recompute_summaries(); return dtrange.min; }
    const Datetime& datetime_max() const { if (dirty) recompute_summaries(); return dtrange.max; }
    unsigned data_count() const { if (dirty) recompute_summaries(); return count; }

    /// Add an entry to the summary
    void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count);

    /// Add an entry to the summary taken from the current status of \a cur
    void add_cursor(const db::CursorSummary& cur);

    /// Add the contents of a Message
    void add_message(const dballe::Message& message);

    /// Add the contents of a Messages
    void add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages);

    /// Merge the copy of another summary into this one
    void add_summary(const BaseSummary& summary);

    /// Merge the copy of another summary into this one
    void add_filtered(const BaseSummary& summary, const dballe::Query& query);

#if 0
    /**
     * Merge entries with duplicate metadata
     *
     * Call this function if you call add_entry with entries that may already
     * exist in the summary
     */
    void merge_entries();

    /// Iterate all values in the summary
    bool iterate(std::function<bool(const summary::Entry&)> f) const;

    /// Iterate all values in the summary that match the given query
    bool iterate_filtered(const Query& query, std::function<bool(const summary::Entry&)> f) const;
#endif

    void to_json(core::JSONWriter& writer) const;

    static BaseSummary from_json(core::json::Stream& in);

#if 0
    DBALLE_TEST_ONLY std::vector<summary::Entry>& test_entries() { return entries; }
    DBALLE_TEST_ONLY const std::vector<summary::Entry>& test_entries() const { return entries; }
#endif
    DBALLE_TEST_ONLY void dump(FILE* out) const;
};

typedef BaseSummary<dballe::Station> Summary;
typedef BaseSummary<dballe::DBStation> DBSummary;

extern template class BaseSummary<dballe::Station>;
extern template class BaseSummary<dballe::DBStation>;

}
}

#endif
