#include "driver.h"
#include "config.h"
#include "dballe/db/v7/sqlite/driver.h"
#include "dballe/sql/sqlite.h"
#ifdef HAVE_LIBPQ
#include "dballe/db/v7/postgresql/driver.h"
#include "dballe/sql/postgresql.h"
#endif
#if 0
#ifdef HAVE_MYSQL
#include "dballe/db/v7/mysql/driver.h"
#include "dballe/sql/mysql.h"
#endif
#endif
#include <cstring>
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

bool SQLRecordV7::querybest_fields_are_the_same(const SQLRecordV7& r)
{
    if (out_lat != r.out_lat) return false;
    if (out_lon != r.out_lon) return false;
    if (strcmp(out_ident, r.out_ident) != 0) return false;
    if (out_id_ltr != r.out_id_ltr) return false;
    if (out_datetime != r.out_datetime) return false;
    if (out_varcode != r.out_varcode) return false;
    return true;
}

void SQLRecordV7::dump(FILE* out)
{
    fprintf(out, "st: %d %6d %6d ", out_ana_id, out_lat, out_lon);
    if (out_ident_size == -1)
        fputs("fixed, ", out);
    else
        fprintf(out, "%.*s, ", out_ident_size, out_ident);
    fprintf(out, "rc: %d %d, ltr %d, did %d, ", out_rep_cod, priority, out_id_ltr, out_id_data);
    fprintf(out, "dt: ");
    out_datetime.print_iso8601(out, ' ', " ");
    out_datetimemax.print_iso8601(out, ' ', ", ");
    fprintf(out, "%d%02d%03d %s\n", WR_VAR_FXY(out_varcode), out_value);
}

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
#if 0
#ifdef HAVE_MYSQL
    else if (MySQLConnection* c = dynamic_cast<MySQLConnection*>(&conn))
        return unique_ptr<Driver>(new mysql::Driver(*c));
#endif
#endif
    else
        throw error_unimplemented("DB drivers only implemented for "
#ifdef HAVE_LIBPQ
                "PostgreSQL, "
#endif
#if 0
#ifdef HAVE_MYSQL
                "MySQL, "
#endif
#endif
                " and SQLite connectors");
}

}
}
}
