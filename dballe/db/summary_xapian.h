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
    std::string pathname;
    Xapian::WritableDatabase db;

public:
    BaseSummaryXapian();
    BaseSummaryXapian(const std::string& pathname);
    ~BaseSummaryXapian();

    bool stations(std::function<bool(const Station&)>) const override;
    bool reports(std::function<bool(const std::string&)>) const override;
    bool levels(std::function<bool(const Level&)>) const override;
    bool tranges(std::function<bool(const Trange&)>) const override;
    bool varcodes(std::function<bool(const wreport::Varcode&)>) const override;

    Datetime datetime_min() const override;
    Datetime datetime_max() const override;
    unsigned data_count() const override;

    void clear() override;
    void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count) override;
    void commit() override;

    bool iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;
    bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const override;

    /// Serialize to JSON
    void to_json(core::JSONWriter& writer) const override;

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
