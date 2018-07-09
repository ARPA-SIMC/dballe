#ifndef DBALLE_DB_SUMMARY_H
#define DBALLE_DB_SUMMARY_H

#include <dballe/core/fwd.h>
#include <dballe/core/defs.h>
#include <dballe/core/query.h>
#include <dballe/core/values.h>
#include <dballe/db/db.h>
#include <vector>
#include <set>
#include <functional>

namespace dballe {
namespace db {

class Matcher;
class Summary;

namespace summary {

/// Represent whether a summary can satisfy a given query
enum Support
{
    /// It cannot.
    UNSUPPORTED = 0,
    /// The query may select less data than this summary can estimate.
    OVERESTIMATED = 1,
    /// The query selects data that this summary can estimate exactly.
    EXACT = 2,
};

struct Entry
{
    Station station;
    dballe::Level level;
    dballe::Trange trange;
    wreport::Varcode varcode;

    dballe::DatetimeRange dtrange;
    size_t count = 0;

    Entry() = default;
    Entry(db::CursorSummary& cur);

    bool operator==(const Entry& o) const
    {
        return std::tie(station, level, trange, varcode, dtrange, count) ==
               std::tie(o.station, o.level, o.trange, o.varcode, o.dtrange, o.count);
    }
    bool operator!=(const Entry& o) const
    {
        return std::tie(station, level, trange, varcode, dtrange, count) !=
               std::tie(o.station, o.level, o.trange, o.varcode, o.dtrange, o.count);
    }
    bool operator<(const Entry& o) const
    {
        return std::tie(station, level, trange, varcode, dtrange, count) <
               std::tie(o.station, o.level, o.trange, o.varcode, o.dtrange, o.count);
    }

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

    void to_json(core::JSONWriter& writer) const;

    static Entry from_json(core::json::Stream& in);
};

std::ostream& operator<<(std::ostream& out, const Entry& e);

}


/**
 * High level objects for working with DB-All.e DB summaries
 */
class Summary
{
protected:
    // Summary of items for the currently active filter
    std::vector<summary::Entry> entries;

    void aggregate(const summary::Entry& entry);

public:
    Summary();
    Summary(std::vector<summary::Entry>&& entries);

    bool operator==(const Summary& o) const
    {
        return entries == o.entries;
    }

    // True if the summary has been filled with data
    bool valid = false;

    std::map<int, Station> all_stations;
    std::set<std::string> all_reports;
    std::set<dballe::Level> all_levels;
    std::set<dballe::Trange> all_tranges;
    std::set<wreport::Varcode> all_varcodes;

    // Last known datetime range for the data that we have
    dballe::DatetimeRange dtrange;
    // Last known count for the data that we have
    unsigned count = MISSING_INT;

    /// Return true if the summary has been filled with data
    bool is_valid() const { return valid; }

    const Datetime& datetime_min() const { return dtrange.min; }
    const Datetime& datetime_max() const { return dtrange.max; }
    unsigned data_count() const { return count; }

    /// Add an entry to the summary taken from the current status of \a cur
    void add_cursor(db::CursorSummary& cur);

    /// Add a copy of an existing entry
    void add_entry(const summary::Entry& entry);

    /// Merge the copy of another summary into this one
    void add_summary(const Summary& summary);

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

    void to_json(core::JSONWriter& writer) const;

    static Summary from_json(core::json::Stream& in);

    DBALLE_TEST_ONLY std::vector<summary::Entry>& test_entries() { return entries; }
    DBALLE_TEST_ONLY const std::vector<summary::Entry>& test_entries() const { return entries; }
};

}
}

#endif
