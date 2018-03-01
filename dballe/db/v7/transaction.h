#ifndef DBALLE_DB_V7_TRANSACTION_H
#define DBALLE_DB_V7_TRANSACTION_H

#include <dballe/transaction.h>
#include <dballe/db/db.h>
#include <dballe/db/v7/state.h>
#include <memory>

namespace dballe {
namespace db {
namespace v7 {
struct DB;

struct Transaction : public dballe::db::Transaction
{
    v7::DB& db;
    dballe::Transaction* sql_transaction = nullptr;
    State state;

    Transaction(v7::DB& db, std::unique_ptr<dballe::Transaction> sql_transaction)
        : db(db), sql_transaction(sql_transaction.release()) {}
    Transaction(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction& operator=(Transaction&&) = delete;
    ~Transaction()
    {
        delete sql_transaction;
    }

    void commit() override { sql_transaction->commit(); }
    void rollback() override { sql_transaction->rollback(); }
    void clear_cached_state() override { state.clear(); }

    void remove_all() override;

    static Transaction& downcast(dballe::Transaction& transaction);
};

}
}
}
#endif
