#ifndef DBALLE_DB_SUMMARY_H
#define DBALLE_DB_SUMMARY_H

#include <dballe/core/defs.h>
#include <dballe/core/fwd.h>
#include <dballe/types.h>
#include <functional>
#include <vector>

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

    VarDesc(const dballe::Level& level, const dballe::Trange& trange,
            wreport::Varcode varcode)
        : level(level), trange(trange), varcode(varcode)
    {
    }

    VarDesc(const VarDesc&) = default;

    bool operator==(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) ==
               std::tie(o.level, o.trange, o.varcode);
    }
    bool operator!=(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) !=
               std::tie(o.level, o.trange, o.varcode);
    }
    bool operator<(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) <
               std::tie(o.level, o.trange, o.varcode);
    }
    bool operator<=(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) <=
               std::tie(o.level, o.trange, o.varcode);
    }
    bool operator>(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) >
               std::tie(o.level, o.trange, o.varcode);
    }
    bool operator>=(const VarDesc& o) const
    {
        return std::tie(level, trange, varcode) >=
               std::tie(o.level, o.trange, o.varcode);
    }
};

} // namespace summary

/**
 * High level objects for working with DB-All.e DB summaries
 */
template <typename Station> class BaseSummary
{
public:
    typedef Station station_type;

    BaseSummary();
    BaseSummary(const BaseSummary&)            = delete;
    BaseSummary(BaseSummary&&)                 = delete;
    BaseSummary& operator=(const BaseSummary&) = delete;
    BaseSummary& operator=(BaseSummary&&)      = delete;
    virtual ~BaseSummary();

    // virtual bool operator==(const BaseSummary<Station>& o) const = 0;

    virtual bool stations(std::function<bool(const Station&)>) const    = 0;
    virtual bool reports(std::function<bool(const std::string&)>) const = 0;
    virtual bool levels(std::function<bool(const Level&)>) const        = 0;
    virtual bool tranges(std::function<bool(const Trange&)>) const      = 0;
    virtual bool
        varcodes(std::function<bool(const wreport::Varcode&)>) const = 0;

    /**
     * Recompute reports, levels, tranges, and varcodes.
     *
     * Call this after performing changes to the summary, to make those sets
     * valid before reading them.
     */
    virtual Datetime datetime_min() const = 0;
    virtual Datetime datetime_max() const = 0;
    virtual unsigned data_count() const   = 0;

    /**
     * Query the contents of the summary
     *
     * @param query
     *   The record with the query data (see technical specifications,
     * par. 1.6.4 "parameter output/input")
     * @return
     *   The cursor to use to iterate over the results. The results are the
     *   same as DB::query_summary.
     */
    virtual std::shared_ptr<dballe::CursorSummary>
    query_summary(const Query& query) const;

    /// Completely empty the summary
    virtual void clear() = 0;

    /// Add an entry to the summary
    virtual void add(const Station& station, const summary::VarDesc& vd,
                     const dballe::DatetimeRange& dtrange, size_t count) = 0;

    /// Add an entry to the summary taken from the current status of \a cur
    virtual void add_cursor(const dballe::CursorSummary& cur);

    /// Add the contents of a Message
    virtual void add_message(const dballe::Message& message,
                             bool station_data = true, bool data = true);

    /// Add the contents of a Messages
    virtual void
    add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages,
                 bool station_data = true, bool data = true);

    /// Merge the copy of another summary into this one
    virtual void add_summary(const BaseSummary<dballe::Station>& summary);

    /// Merge the copy of another summary into this one
    virtual void add_summary(const BaseSummary<dballe::DBStation>& summary);

    /// Merge the copy of another summary into this one
    virtual void add_filtered(const BaseSummary<Station>& summary,
                              const dballe::Query& query);

    /// Write changes to disk
    virtual void commit() = 0;

    /// Iterate the contents of this summary. There is no guarantee on sorting
    /// order.
    virtual bool
        iter(std::function<bool(const Station&, const summary::VarDesc&,
                                const DatetimeRange&, size_t)>) const = 0;

    /// Iterate the contents of this summary. There is no guarantee on sorting
    /// order.
    virtual bool
    iter_filtered(const dballe::Query& query,
                  std::function<bool(const Station&, const summary::VarDesc&,
                                     const DatetimeRange&, size_t)>) const = 0;

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

} // namespace db
} // namespace dballe

#endif
