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

class Explorer
{
protected:
    /// Summary of the whole database
    dballe::db::Summary* _global_summary = nullptr;

    /// Currently active filter
    dballe::core::Query filter;

    /// Summary of active_filter
    dballe::db::Summary* _active_summary = nullptr;

    /// Regenerate _active_summary based on filter
    void update_active_summary();

public:
    Explorer();
    ~Explorer();

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
    const dballe::db::Summary& global_summary() const;

    /// Get a reference to the summary for the current filter
    const dballe::db::Summary& active_summary() const;

    /**
     * Update \a val in the database to have the value \a new_val
     *
     * Updates the 'val' member of 'val' if it succeeded, otherwise
     * exceptions are raised
     */
    void update_station(values::Value& val, const wreport::Var& new_val);

    /**
     * Update \a val in the database to have the value \a new_val
     *
     * Updates the 'val' member of 'val' if it succeeded, otherwise
     * exceptions are raised
     */
    void update_data(values::Value& val, const wreport::Var& new_val);

    /**
     * Update an attribute
     */
    void update_attr(int var_id, wreport::Varcode var_related, const wreport::Var& new_val);

    /// Remove the value from the database
    void remove(const values::Value& val);

};

}
}
#endif
