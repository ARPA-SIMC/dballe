#include "db.h"
#include "db/db.h"
#include "sql/sql.h"
#include "core/string.h"

namespace dballe {

void DBConnectOptions::reset_actions()
{
    wipe = false;
}

std::unique_ptr<DBConnectOptions> DBConnectOptions::create(const std::string& url)
{
    std::unique_ptr<DBConnectOptions> res(new DBConnectOptions);
    res->url = url;
    std::string wipe = url_pop_query_string(res->url, "wipe");
    res->wipe = !wipe.empty();
    return res;
}


const DBImportOptions DBImportOptions::defaults;

std::unique_ptr<DBImportOptions> DBImportOptions::create()
{
    return std::unique_ptr<DBImportOptions>(new DBImportOptions);
}


const DBInsertOptions DBInsertOptions::defaults;

std::unique_ptr<DBInsertOptions> DBInsertOptions::create()
{
    return std::unique_ptr<DBInsertOptions>(new DBInsertOptions);
}

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

std::shared_ptr<DB> DB::connect(const DBConnectOptions& opts)
{
    if (opts.url == "mem:")
    {
        return db::DB::connect_memory();
    } else {
        std::unique_ptr<sql::Connection> conn(sql::Connection::create_from_url(opts.url));
        auto res = db::DB::create(move(conn));
        if (opts.wipe)
            res->reset();
        return res;
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

std::unique_ptr<CursorMessage> DB::query_messages(const Query& query)
{
    auto t = transaction();
    auto res = t->query_messages(query);
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

void DB::insert_station_data(Data& vals, const DBInsertOptions& opts)
{
    auto t = transaction();
    t->insert_station_data(vals, opts);
    t->commit();
}

void DB::insert_data(Data& vals, const DBInsertOptions& opts)
{
    auto t = transaction();
    t->insert_data(vals, opts);
    t->commit();
}

}
