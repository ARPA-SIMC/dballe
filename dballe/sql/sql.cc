#include "sql.h"
#include "dballe/types.h"
#include "dballe/db.h"
#include "querybuf.h"
#include "sqlite.h"
#include "config.h"
#ifdef HAVE_LIBPQ
#include "postgresql.h"
#endif
#ifdef HAVE_MYSQL
#include "mysql.h"
#endif
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

// We cannot call rollback here, because the child class will have already been
// destructed, and rollback_nothrow will only be a abstract virtual method at
// this point
Transaction::~Transaction() {}

const char* format_server_type(ServerType type)
{
    switch (type)
    {
        case ServerType::MYSQL: return "mysql";
        case ServerType::SQLITE: return "sqlite";
        case ServerType::ORACLE: return "oracle";
        case ServerType::POSTGRES: return "postgresql";
        default: return "unknown";
    }
}

Connection::Connection()
{
}

Connection::~Connection()
{
}

static std::vector<std::weak_ptr<Connection>> atfork_connections;

void Connection::atfork_prepare_hook()
{
    try {
        for (auto& c: atfork_connections)
            if (!c.expired())
                c.lock()->fork_prepare();
    } catch (std::exception& e) {
        fprintf(stderr, "pre-fork error: %s\n", e.what());
    }
}

void Connection::atfork_parent_hook()
{
    try {
        for (auto& c: atfork_connections)
            if (!c.expired())
                c.lock()->fork_parent();
    } catch (std::exception& e) {
        fprintf(stderr, "post-fork parent error: %s\n", e.what());
    }
}

void Connection::atfork_child_hook()
{
    try {
        for (auto& c: atfork_connections)
            if (!c.expired())
                c.lock()->fork_child();
    } catch (std::exception& e) {
        fprintf(stderr, "post-fork child error: %s\n", e.what());
    }
}

void Connection::register_atfork()
{
    for (auto& c: atfork_connections)
        if (c.expired())
        {
            c = shared_from_this();
            return;
        }
    atfork_connections.emplace_back(shared_from_this());

    static bool atfork_registered = false;
    if (!atfork_registered)
    {
        if (pthread_atfork(atfork_prepare_hook, atfork_parent_hook, atfork_child_hook) != 0)
            throw error_system("cannot register atfork handlers for db connections");
        atfork_registered = true;
    }
}

void Connection::fork_prepare() {}
void Connection::fork_parent() {}
void Connection::fork_child() {}

void Connection::add_datetime(Querybuf& qb, const Datetime& dt) const
{
    qb.appendf("'%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu'",
            dt.year, dt.month, dt.day,
            dt.hour, dt.minute, dt.second);
}

std::shared_ptr<Connection> Connection::create(const DBConnectOptions& options)
{
    const char* url = options.url.c_str();
    if (strncmp(url, "sqlite://", 9) == 0)
    {
        auto conn = SQLiteConnection::create();
        conn->open_file(url + 9);
        return conn;
    }
    if (strncmp(url, "sqlite:", 7) == 0)
    {
        auto conn = SQLiteConnection::create();
        conn->open_file(url + 7);
        return conn;
    }
    if (strncmp(url, "mem:", 4) == 0)
    {
        throw error_consistency("SQL connections are not available on mem: databases");
    }
    if (strncmp(url, "postgresql:", 11) == 0)
    {
#ifdef HAVE_LIBPQ
        auto conn = PostgreSQLConnection::create();
        conn->open_url(url);
        return conn;
#else
        throw error_unimplemented("PostgreSQL support is not available");
#endif
    }
    if (strncmp(url, "mysql:", 6) == 0)
    {
#ifdef HAVE_MYSQL
        auto conn = MySQLConnection::create();
        conn->open_url(url);
        return conn;
#else
        throw error_unimplemented("MySQL support is not available");
#endif
    }
    error_consistency::throwf("unsupported url \"%s\"", url);
}

}
}
