#ifndef DBALLE_DB_SUMMARY_MEMORY_H
#define DBALLE_DB_SUMMARY_MEMORY_H

#include <dballe/core/fwd.h>
#include <dballe/db/summary.h>
#include <dballe/db/summary_utils.h>

namespace dballe {
namespace db {

namespace summary {

template<typename Station>
struct CursorMemory : public impl::CursorSummary
{
    summary::StationEntries<Station> results;
    typename summary::StationEntries<Station>::const_iterator station_entry;
    typename summary::StationEntry<Station>::const_iterator var_entry;
    bool at_start = true;
    int _remaining = 0;

    CursorMemory(const summary::StationEntries<Station>& entries, const Query& query);

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

extern template class CursorMemory<dballe::Station>;
extern template class CursorMemory<dballe::DBStation>;

}


/**
 * High level objects for working with DB-All.e DB summaries
 */
template<typename Station>
class BaseSummaryMemory : public BaseSummary<Station>
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
    BaseSummaryMemory();

    const summary::StationEntries<Station>& _entries() const { if (dirty) recompute_summaries(); return entries.sorted(); }

    // bool operator==(const BaseSummary<Station>& o) const
    // {
    //     return entries == o.entries;
    // }

    bool stations(std::function<bool(const Station&)>) const override;
    const core::SortedSmallUniqueValueSet<std::string>& reports() const override { if (dirty) recompute_summaries(); return m_reports; }
    const core::SortedSmallUniqueValueSet<dballe::Level>& levels() const override { if (dirty) recompute_summaries(); return m_levels; }
    const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges() const override { if (dirty) recompute_summaries(); return m_tranges; }
    const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes() const override { if (dirty) recompute_summaries(); return m_varcodes; }

    Datetime datetime_min() const override { if (dirty) recompute_summaries(); return dtrange.min; }
    Datetime datetime_max() const override { if (dirty) recompute_summaries(); return dtrange.max; }
    unsigned data_count() const override { if (dirty) recompute_summaries(); return count; }

    /**
     * Query the contents of the summary
     *
     * @param query
     *   The record with the query data (see technical specifications, par. 1.6.4
     *   "parameter output/input")
     * @return
     *   The cursor to use to iterate over the results. The results are the
     *   same as DB::query_summary.
     */
    std::unique_ptr<dballe::CursorSummary> query_summary(const Query& query) const override;

    bool iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;
    bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;

    /// Add an entry to the summary
    void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count) override;

    /// Merge the copy of another summary into this one
    void add_summary(const BaseSummary<dballe::Station>& summary) override;

    /// Merge the copy of another summary into this one
    void add_summary(const BaseSummary<dballe::DBStation>& summary) override;

    /// Merge the copy of another summary into this one
    void add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query) override;

    /// Serialize to JSON
    void to_json(core::JSONWriter& writer) const override;

    /// Load contents from JSON, merging with the current contents
    void load_json(core::json::Stream& in) override;

    DBALLE_TEST_ONLY void dump(FILE* out) const override;
};

/**
 * Summary without database station IDs
 */
typedef BaseSummaryMemory<dballe::Station> SummaryMemory;

/**
 * Summary with database station IDs
 */
typedef BaseSummaryMemory<dballe::DBStation> DBSummaryMemory;

extern template class BaseSummaryMemory<dballe::Station>;
extern template class BaseSummaryMemory<dballe::DBStation>;

}
}

#endif
