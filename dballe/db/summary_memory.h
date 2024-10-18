#ifndef DBALLE_DB_SUMMARY_MEMORY_H
#define DBALLE_DB_SUMMARY_MEMORY_H

#include <dballe/core/fwd.h>
#include <dballe/db/summary.h>
#include <dballe/db/summary_utils.h>
#include <filesystem>

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

    std::filesystem::path path;

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
    explicit BaseSummaryMemory(const std::filesystem::path& path);

    const summary::StationEntries<Station>& _entries() const { if (dirty) recompute_summaries(); return entries.sorted(); }

    bool stations(std::function<bool(const Station&)>) const override;
    bool reports(std::function<bool(const std::string&)>) const override;
    bool levels(std::function<bool(const Level&)>) const override;
    bool tranges(std::function<bool(const Trange&)>) const override;
    bool varcodes(std::function<bool(const wreport::Varcode&)>) const override;

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
    std::shared_ptr<dballe::CursorSummary> query_summary(const Query& query) const override;

    bool iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;
    bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;

    void clear() override;

    /// Add an entry to the summary
    void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count) override;

    /// Merge the copy of another summary into this one
    void add_summary(const BaseSummary<dballe::Station>& summary) override;

    /// Merge the copy of another summary into this one
    void add_summary(const BaseSummary<dballe::DBStation>& summary) override;

    /// Merge the copy of another summary into this one
    void add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query) override;

    void commit() override;

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
