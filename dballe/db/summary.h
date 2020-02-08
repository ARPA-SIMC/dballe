#ifndef DBALLE_DB_SUMMARY_H
#define DBALLE_DB_SUMMARY_H

#include <dballe/core/fwd.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include <dballe/core/smallset.h>
#include <dballe/db/db.h>
#include <vector>
#include <set>
#include <functional>

namespace dballe {
namespace db {
namespace summary {

/**
 * Description of a variable, independent of where and when it was measured
 */
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

}

/**
 * High level objects for working with DB-All.e DB summaries
 */
template<typename Station>
class BaseSummary
{
public:
    typedef Station station_type;

    BaseSummary();
    BaseSummary(const BaseSummary&) = delete;
    BaseSummary(BaseSummary&&) = delete;
    BaseSummary& operator=(const BaseSummary&) = delete;
    BaseSummary& operator=(BaseSummary&&) = delete;
    virtual ~BaseSummary();

    // virtual bool operator==(const BaseSummary<Station>& o) const = 0;

    virtual bool stations(std::function<bool(const Station&)>) const = 0;
    virtual const core::SortedSmallUniqueValueSet<std::string>& reports() const = 0;
    virtual const core::SortedSmallUniqueValueSet<dballe::Level>& levels() const = 0;
    virtual const core::SortedSmallUniqueValueSet<dballe::Trange>& tranges() const = 0;
    virtual const core::SortedSmallUniqueValueSet<wreport::Varcode>& varcodes() const = 0;

    /**
     * Recompute reports, levels, tranges, and varcodes.
     *
     * Call this after performing changes to the summary, to make those sets
     * valid before reading them.
     */
    virtual Datetime datetime_min() const = 0;
    virtual Datetime datetime_max() const = 0;
    virtual unsigned data_count() const = 0;

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
    virtual std::unique_ptr<dballe::CursorSummary> query_summary(const Query& query) const = 0;

    /// Add an entry to the summary
    virtual void add(const Station& station, const summary::VarDesc& vd, const dballe::DatetimeRange& dtrange, size_t count) = 0;

    /// Add an entry to the summary taken from the current status of \a cur
    virtual void add_cursor(const dballe::CursorSummary& cur);

    /// Add the contents of a Message
    virtual void add_message(const dballe::Message& message);

    /// Add the contents of a Messages
    virtual void add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages);

    /// Merge the copy of another summary into this one
    virtual void add_summary(const BaseSummary<dballe::Station>& summary);

    /// Merge the copy of another summary into this one
    virtual void add_summary(const BaseSummary<dballe::DBStation>& summary);

    /// Merge the copy of another summary into this one
    virtual void add_filtered(const BaseSummary<Station>& summary, const dballe::Query& query);

    /// Iterate the contents of this summary. There is no guarantee on sorting order.
    virtual bool iter(std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const = 0;

    /// Iterate the contents of this summary. There is no guarantee on sorting order.
    virtual bool iter_filtered(const dballe::Query& query, std::function<bool(const Station&, const summary::VarDesc&, const DatetimeRange&, size_t)>) const = 0;

    /// Serialize to JSON
    virtual void to_json(core::JSONWriter& writer) const = 0;

    /// Load contents from JSON, merging with the current contents
    virtual void load_json(core::json::Stream& in);

    DBALLE_TEST_ONLY virtual void dump(FILE* out) const = 0;
};


/**
 * Summary without database station IDs
 */
typedef BaseSummary<dballe::Station> Summary;

/**
 * Summary with database station IDs
 */
typedef BaseSummary<dballe::DBStation> DBSummary;

extern template class BaseSummary<dballe::Station>;
extern template class BaseSummary<dballe::DBStation>;

}
}

#endif
