#include "db.h"
#include "db/db.h"
#include "sql/sql.h"

namespace dballe {

const DBImportOptions DBImportOptions::defaults;

std::unique_ptr<DBImportOptions> DBImportOptions::create()
{
    return std::unique_ptr<DBImportOptions>(new DBImportOptions);
}

const DBInsertOptions DBInsertOptions::defaults;

/*
 * Cursor*
 */

Cursor::~Cursor()
{
}


/*
 * Transaction
 */

Transaction::~Transaction() {}

void Transaction::import_messages(const std::vector<std::shared_ptr<Message>>& messages, const DBImportOptions& opts)
{
    for (const auto& i: messages)
        import_message(*i, opts);
}


/*
 * DB
 */

DB::~DB()
{
}

std::shared_ptr<DB> DB::connect_from_url(const std::string& url)
{
    if (url == "mem:")
    {
        return db::DB::connect_memory();
    } else {
        std::unique_ptr<sql::Connection> conn(sql::Connection::create_from_url(url));
        return db::DB::create(move(conn));
    }
}

std::unique_ptr<CursorStation> DB::query_stations(const Query& query)
{
    auto t = transaction();
    auto res = t->query_stations(query);
    return res;
}

std::unique_ptr<CursorStationData> DB::query_station_data(const Query& query)
{
    auto t = transaction();
    auto res = t->query_station_data(query);
    return res;
}

std::unique_ptr<CursorData> DB::query_data(const Query& query)
{
    auto t = transaction();
    auto res = t->query_data(query);
    return res;
}

std::unique_ptr<CursorSummary> DB::query_summary(const Query& query)
{
    auto t = transaction();
    auto res = t->query_summary(query);
    return res;
}

void DB::remove_all()
{
    auto t = transaction();
    t->remove_all();
    t->commit();
}

void DB::remove_station_data(const Query& query)
{
    auto t = transaction();
    t->remove_station_data(query);
    t->commit();
}

void DB::remove_data(const Query& query)
{
    auto t = transaction();
    t->remove_data(query);
    t->commit();
}

void DB::import_message(const Message& message, const DBImportOptions& opts)
{
    auto t = transaction();
    t->import_message(message, opts);
    t->commit();
}

void DB::import_messages(const std::vector<std::shared_ptr<Message>>& messages, const DBImportOptions& opts)
{
    auto t = transaction();
    t->import_messages(messages, opts);
    t->commit();
}

}
