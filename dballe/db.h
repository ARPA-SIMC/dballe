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
};


struct DB: public std::enable_shared_from_this<DB>
{
    virtual ~DB();

    /**
     * Begin a transaction on this database, and return a Transaction object
     * that can be used to commit it.
     */
    virtual std::shared_ptr<dballe::Transaction> transaction(bool readonly=false) = 0;

    /**
     * Create from an url-like specification, as described in
     * doc/fapi_connect.md
     *
     * @param url
     *   The url-like connection descriptor
     */
    static std::shared_ptr<DB> connect_from_url(const std::string& url);
};

}

#endif
