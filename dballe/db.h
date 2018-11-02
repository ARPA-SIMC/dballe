#ifndef DBALLE_DB_H
#define DBALLE_DB_H

#include <dballe/fwd.h>
#include <memory>

namespace dballe {

class Transaction : public std::enable_shared_from_this<Transaction>
{
public:
    virtual ~Transaction();

    /// Commit this transaction
    virtual void commit() = 0;

    /// Roll back this transaction
    virtual void rollback() = 0;

    /// Roll back this transaction
    virtual void rollback_nothrow() noexcept = 0;

    /**
     * Remove all data from the database.
     *
     * This is faster than remove() with an empty record, and unlike reset() it
     * preserves existing report information.
     *
     * @param transaction
     *   The current active transaction.
     */
    virtual void remove_all() = 0;

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    virtual void remove_station_data(const Query& query) = 0;

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    virtual void remove_data(const Query& query) = 0;
};


struct DB: public std::enable_shared_from_this<DB>
{
    virtual ~DB();

    /**
     * Create from an url-like specification, as described in
     * doc/fapi_connect.md
     *
     * @param url
     *   The url-like connection descriptor
     */
    static std::shared_ptr<DB> connect_from_url(const std::string& url);

    /**
     * Begin a transaction on this database, and return a Transaction object
     * that can be used to commit it.
     */
    virtual std::shared_ptr<dballe::Transaction> transaction(bool readonly=false) = 0;

    /**
     * Remove all data from the database.
     *
     * This is faster than remove() with an empty record, and unlike reset() it
     * preserves existing report information.
     */
    void remove_all();

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    void remove_station_data(const Query& query);

    /**
     * Remove data from the database
     *
     * @param rec
     *   The query selecting the data to remove
     */
    void remove_data(const Query& query);
};

}

#endif
