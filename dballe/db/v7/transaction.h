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
    std::shared_ptr<v7::DB> db;
    dballe::Transaction* sql_transaction = nullptr;
    State state;

    Transaction(std::shared_ptr<v7::DB> db, std::unique_ptr<dballe::Transaction> sql_transaction)
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

    std::unique_ptr<db::CursorStation> query_stations(const Query& query);
    std::unique_ptr<db::CursorStationData> query_station_data(const Query& query) override;
    std::unique_ptr<db::CursorData> query_data(const Query& query);
    std::unique_ptr<db::CursorSummary> query_summary(const Query& query);
    void attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;
    void attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;

    void insert_station_data(StationValues& vals, bool can_replace, bool station_can_add) override;
    void insert_data(DataValues& vals, bool can_replace, bool station_can_add) override;
    void remove_station_data(const Query& query) override;
    void remove(const Query& query);
    void remove_all() override;

    void attr_insert_station(int data_id, const Values& attrs) override;
    void attr_insert_data(int data_id, const Values& attrs) override;
    void attr_remove_station(int data_id, const db::AttrList& attrs) override;
    void attr_remove_data(int data_id, const db::AttrList& attrs) override;
    void import_msg(const Message& msg, const char* repmemo, int flags) override;
    bool export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest) override;

    static Transaction& downcast(dballe::Transaction& transaction);
};

}
}
}
#endif
