#include "db.h"
#include "db/db.h"
#include "sql/sql.h"

namespace dballe {

Transaction::~Transaction() {}

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


}
