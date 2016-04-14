#include "driver.h"
#include "config.h"
#include "dballe/db/v7/sqlite/driver.h"
#include "dballe/sql/sqlite.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/v7/postgresql/driver.h"
#include "dballe/sql/postgresql.h"
#endif
#ifdef HAVE_MYSQL
#include "dballe/db/v7/mysql/driver.h"
#include "dballe/sql/mysql.h"
#endif
#include <cstring>
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Driver::Driver(sql::Connection& connection)
    : connection(connection)
{
}

Driver::~Driver()
{
}

void Driver::create_tables(db::Format format)
{
    switch (format)
    {
        case V7: create_tables_v7(); break;
        default: throw wreport::error_consistency("cannot create tables on the given DB format");
    }
}

void Driver::delete_tables(db::Format format)
{
    switch (format)
    {
        case V7: delete_tables_v7(); break;
        default: throw wreport::error_consistency("cannot delete tables on the given DB format");
    }
}

void Driver::remove_all(db::Format format)
{
    switch (format)
    {
        case V7: remove_all_v7(); break;
        default: throw wreport::error_consistency("cannot empty a database with the given format");
    }
}

void Driver::remove_all_v7()
{
    connection.execute("DELETE FROM station_data");
    connection.execute("DELETE FROM data");
    connection.execute("DELETE FROM levtr");
    connection.execute("DELETE FROM station");
}

std::unique_ptr<Driver> Driver::create(dballe::sql::Connection& conn)
{
    using namespace dballe::sql;

    if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<Driver>(new sqlite::Driver(*c));
#ifdef HAVE_LIBPQ
    else if (PostgreSQLConnection* c = dynamic_cast<PostgreSQLConnection*>(&conn))
        return unique_ptr<Driver>(new postgresql::Driver(*c));
#endif
#ifdef HAVE_MYSQL
    else if (MySQLConnection* c = dynamic_cast<MySQLConnection*>(&conn))
        return unique_ptr<Driver>(new mysql::Driver(*c));
#endif
    else
        throw error_unimplemented("DB drivers only implemented for "
#ifdef HAVE_LIBPQ
                "PostgreSQL, "
#endif
#ifdef HAVE_MYSQL
                "MySQL, "
#endif
                "SQLite connectors");
}

}
}
}
