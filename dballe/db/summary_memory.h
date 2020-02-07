#ifndef DBALLE_DB_SUMMARY_MEMORY_H
#define DBALLE_DB_SUMMARY_MEMORY_H

#include <dballe/core/fwd.h>
#include <dballe/db/summary.h>

namespace dballe {
namespace db {

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

    // bool operator==(const BaseSummary<Station>& o) const
    // {
    //     return entries == o.entries;
    // }

    const summary::StationEntries<Station>& stations() const override { if (dirty) recompute_summaries(); return entries.sorted(); }
    const core::SortedSmallUniqueValueSet<std::string>& reports() const override { if (dirty) recompute_summaries(); return m_reports; }
    const core::SortedSmallUniqueValueSet<dballe::Level>& levels() const override { if (dirty) recompute_summaries(); return m_levels; }
    const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges() const override { if (dirty) recompute_summaries(); return m_tranges; }
    const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes() const override { if (dirty) recompute_summaries(); return m_varcodes; }

    /**
     * Recompute reports, levels, tranges, and varcodes.
     *
     * Call this after performing changes to the summary, to make those sets
     * valid before reading them.
     */
    const Datetime& datetime_min() const override { if (dirty) recompute_summaries(); return dtrange.min; }
    const Datetime& datetime_max() const override { if (dirty) recompute_summaries(); return dtrange.max; }
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

    /// Add an entry to the summary
    void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count) override;

    /// Add an entry to the summary taken from the current status of \a cur
    void add_cursor(const dballe::CursorSummary& cur) override;

    /// Add the contents of a Message
    void add_message(const dballe::Message& message) override;

    /// Add the contents of a Messages
    void add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages) override;

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
// extern template void BaseSummary<dballe::Station>::add_summary(const BaseSummary<dballe::Station>&);
// extern template void BaseSummary<dballe::Station>::add_summary(const BaseSummary<dballe::DBStation>&);
extern template class BaseSummaryMemory<dballe::DBStation>;
// extern template void BaseSummary<dballe::DBStation>::add_summary(const BaseSummary<dballe::Station>&);
// extern template void BaseSummary<dballe::DBStation>::add_summary(const BaseSummary<dballe::DBStation>&);

}
}

#endif
