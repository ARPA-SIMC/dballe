#include "sql/sql.h"
#include "dballe/types.h"
#include "config.h"
#include "sql/sqlite.h"
#ifdef HAVE_ODBC
#include "sql/odbc.h"
#endif
#ifdef HAVE_LIBPQ
#include "sql/postgresql.h"
#endif
#ifdef HAVE_MYSQL
#include "sql/mysql.h"
#endif
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace wreport;

namespace dballe {
namespace sql {

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
    profile = getenv("DBA_PROFILE") != nullptr;
}

Connection::~Connection()
{
    if (profile)
        fprintf(stderr, "%s: %d queries\n", format_server_type(server_type), profile_query_count);
}

void Connection::add_datetime(Querybuf& qb, const Datetime& dt) const
{
    qb.appendf("'%04hu-%02hhu-%02hhu %02hhu:%02hhu:%02hhu'",
            dt.year, dt.month, dt.day,
            dt.hour, dt.minute, dt.second);
}

std::unique_ptr<Connection> Connection::create_from_url(const std::string& url)
{
    return create_from_url(url.c_str());
}

std::unique_ptr<Connection> Connection::create_from_url(const char* url)
{
    if (strncmp(url, "sqlite://", 9) == 0)
    {
        unique_ptr<SQLiteConnection> conn(new SQLiteConnection);
        conn->open_file(url + 9);
        return unique_ptr<Connection>(conn.release());
    }
    if (strncmp(url, "sqlite:", 7) == 0)
    {
        unique_ptr<SQLiteConnection> conn(new SQLiteConnection);
        conn->open_file(url + 7);
        return unique_ptr<Connection>(conn.release());
    }
    if (strncmp(url, "mem:", 4) == 0)
    {
        throw error_consistency("SQL connections are not available on mem: databases");
    }
    if (strncmp(url, "postgresql:", 11) == 0)
    {
#ifdef HAVE_LIBPQ
        unique_ptr<PostgreSQLConnection> conn(new PostgreSQLConnection);
        conn->open_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("PostgreSQL support is not available");
#endif
    }
    if (strncmp(url, "mysql:", 6) == 0)
    {
#ifdef HAVE_MYSQL
        unique_ptr<MySQLConnection> conn(new MySQLConnection);
        conn->open_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("MySQL support is not available");
#endif
    }
    if (strncmp(url, "odbc://", 7) == 0)
    {
#ifdef HAVE_ODBC
        unique_ptr<ODBCConnection> conn(new ODBCConnection);
        conn->connect_url(url);
        return unique_ptr<Connection>(conn.release());
#else
        throw error_unimplemented("ODBC support is not available");
#endif
    }
    if (strncmp(url, "test:", 5) == 0)
    {
        const char* envurl = getenv("DBA_DB");
        if (envurl != NULL)
            return create_from_url(envurl);
        else
            return create_from_url("sqlite://test.sqlite");
    }
    error_consistency::throwf("unsupported url \"%s\"", url);
}

}
}
