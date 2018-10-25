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
    class Update
    {
    protected:
        BaseExplorer<Station>* explorer = nullptr;

        Update(BaseExplorer<Station>* explorer);

    public:
        Update();
        Update(const Update&) = delete;
        Update(Update&&);
        ~Update();

        Update& operator=(const Update&) = delete;
        Update& operator=(Update&&);

        /// Merge summary data from a database
        void add_db(dballe::db::Transaction& tr);

        /// Merge summary data from a database
        void add_cursor(dballe::db::CursorSummary& cur);

        /// Load the explorer contents from JSON
        void add_json(core::json::Stream& in);

        /// Merge the contents of another explorer into this one
        template<typename OStation>
        void add_explorer(const BaseExplorer<OStation>& explorer);

        /// Merge the contents of a message
        void add_message(const dballe::Message& message);

        /// Merge the contents of a vector of messages
        void add_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages);

        void commit();

        friend class BaseExplorer<Station>;
    };

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
     * Throw away all cached data and rebuild the explorer from scratch.
     *
     * Use this when you suspect that the database has been externally modified
     */
    Update rebuild();

    /**
     * Merge new data into the explorer
     */
    Update update();

    /// Get a reference to the global summary
    const dballe::db::BaseSummary<Station>& global_summary() const;

    /// Get a reference to the summary for the current filter
    const dballe::db::BaseSummary<Station>& active_summary() const;

    /// Export the explorer to JSON
    void to_json(core::JSONWriter& writer) const;
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
extern template void BaseExplorer<dballe::Station>::Update::add_explorer(const BaseExplorer<DBStation>&);
extern template class BaseExplorer<dballe::DBStation>;
extern template void BaseExplorer<dballe::DBStation>::Update::add_explorer(const BaseExplorer<Station>&);

}
}
#endif
