#ifndef DBALLE_DB_EXPLORER_H
#define DBALLE_DB_EXPLORER_H

#include <dballe/core/query.h>
#include <dballe/core/values.h>
#include <dballe/db/db.h>
#include <dballe/db/summary.h>
#include <vector>
#include <map>
#include <set>

namespace dballe {
namespace db {

template<typename Station>
class BaseExplorer
{
protected:
    /// Summary of the whole database
    dballe::db::BaseSummary<Station>* _global_summary = nullptr;

    /// Currently active filter
    dballe::core::Query filter;

    /// Summary of active_filter
    dballe::db::BaseSummary<Station>* _active_summary = nullptr;

    /// Regenerate _active_summary based on filter
    void update_active_summary();

public:
    BaseExplorer();
    BaseExplorer(const BaseExplorer&) = delete;
    BaseExplorer(BaseExplorer&&) = delete;
    ~BaseExplorer();
    BaseExplorer& operator=(const BaseExplorer&) = delete;
    BaseExplorer& operator=(BaseExplorer&&) = delete;

    /// Get the current filter
    const dballe::Query& get_filter() const;

    /**
     * Set a new filter, updating all browsing data
     */
    void set_filter(const dballe::Query& query);

    /**
     * Throw away all cached data and reload everything from the database.
     *
     * Use this when you suspect that the database has been externally modified
     */
    void revalidate(dballe::db::Transaction& tr);

    /// Get a reference to the global summary
    const dballe::db::BaseSummary<Station>& global_summary() const;

    /// Get a reference to the summary for the current filter
    const dballe::db::BaseSummary<Station>& active_summary() const;

    /// Export the explorer to JSON
    void to_json(core::JSONWriter& writer) const;

    /// Load the explorer contents from JSON
    void from_json(core::json::Stream& in);
};


/**
 * Explorer without database station IDs
 */
typedef BaseExplorer<dballe::Station> Explorer;

/**
 * Explorer with database station IDs
 */
typedef BaseExplorer<dballe::DBStation> DBExplorer;

extern template class BaseExplorer<dballe::Station>;
extern template class BaseExplorer<dballe::DBStation>;

}
}
#endif
