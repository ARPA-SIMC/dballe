#ifndef DBALLE_DB_SUMMARY_XAPIAN_H
#define DBALLE_DB_SUMMARY_XAPIAN_H

#include <dballe/core/fwd.h>
#include <dballe/db/summary.h>
#include <xapian.h>

namespace dballe {
namespace db {

/**
 * High level objects for working with DB-All.e DB summaries
 */
template<typename Station>
class BaseSummaryXapian: public BaseSummary<Station>
{
    Xapian::WritableDatabase db;

public:
    BaseSummaryXapian();

    const summary::StationEntries<Station>& stations() const override;
    const core::SortedSmallUniqueValueSet<std::string>& reports() const override;
    const core::SortedSmallUniqueValueSet<dballe::Level>& levels() const override;
    const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges() const override;
    const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes() const override;

    const Datetime& datetime_min() const override;
    const Datetime& datetime_max() const override;
    unsigned data_count() const override;

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
typedef BaseSummaryXapian<dballe::Station> SummaryXapian;

/**
 * Summary with database station IDs
 */
typedef BaseSummaryXapian<dballe::DBStation> DBSummaryXapian;

extern template class BaseSummaryXapian<dballe::Station>;
extern template class BaseSummaryXapian<dballe::DBStation>;

}
}

#endif
