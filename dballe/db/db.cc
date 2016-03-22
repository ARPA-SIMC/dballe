#include "config.h"
#include "db.h"
#include "sql.h"
#include "v6/db.h"
#include "v7/db.h"
#include "mem/db.h"
#include "sql/sqlite.h"
#ifdef HAVE_ODBC
#include "sql/odbc.h"
#endif
#include "dballe/message.h"
#include "dballe/core/record.h"
#include "dballe/core/values.h"
#include <wreport/error.h>
#include <cstring>
#include <cstdlib>

using namespace dballe::db;
using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

static Format default_format = V6;

Cursor::~Cursor()
{
}

unsigned Cursor::test_iterate(FILE* dump)
{
    unsigned count;
    for (count = 0; next(); ++count)
        ;
    return count;
}

}

DB::~DB()
{
}

Format DB::get_default_format() { return default_format; }
void DB::set_default_format(Format format) { default_format = format; }

bool DB::is_url(const char* str)
{
    if (strncmp(str, "mem:", 4) == 0) return true;
    if (strncmp(str, "sqlite:", 7) == 0) return true;
    if (strncmp(str, "postgresql:", 11) == 0) return true;
    if (strncmp(str, "mysql:", 6) == 0) return true;
    if (strncmp(str, "odbc://", 7) == 0) return true;
    if (strncmp(str, "test:", 5) == 0) return true;
    return false;
}

unique_ptr<DB> DB::create(unique_ptr<sql::Connection> conn)
{
    // Autodetect format
    Format format = default_format;

    const char* format_override = getenv("DBA_DB_FORMAT");
    if (format_override)
    {
        if (strcmp(format_override, "V6") == 0)
            format = V6;
        else if (strcmp(format_override, "V7") == 0)
            format = V7;
    }

    bool found = true;

    // Try with reading it from the settings table
    string version = conn->get_setting("version");
    if (version == "V5")
        format = V5;
    else if (version == "V6")
        format = V6;
    else if (version == "V7")
        format = V7;
    else if (version == "")
        found = false;// Some other key exists, but the version has not been set
    else
        error_consistency::throwf("unsupported database version: '%s'", version.c_str());

    // If it failed, try looking at the existing table structure
    if (!found)
    {
        if (conn->has_table("lev_tr"))
            format = V6;
        else if (conn->has_table("context"))
            format = V5;
        else
            format = default_format;
    }

    switch (format)
    {
        case V5: throw error_unimplemented("V5 format is not supported anymore by this version of DB-All.e");
        case V6: return unique_ptr<DB>(new v6::DB(move(conn)));
        case V7: return unique_ptr<DB>(new v7::DB(move(conn)));
        default: error_consistency::throwf("requested unknown format %d", (int)format);
    }
}

unique_ptr<DB> DB::connect(const char* dsn, const char* user, const char* password)
{
#ifdef HAVE_ODBC
    unique_ptr<sql::ODBCConnection> conn(new sql::ODBCConnection);
    conn->connect(dsn, user, password);
    return create(move(conn));
#else
    throw error_unimplemented("ODBC support is not available");
#endif
}

unique_ptr<DB> DB::connect_from_file(const char* pathname)
{
    unique_ptr<sql::SQLiteConnection> conn(new sql::SQLiteConnection);
    conn->open_file(pathname);
    return create(unique_ptr<sql::Connection>(conn.release()));
}

unique_ptr<DB> DB::connect_from_url(const char* url)
{
    if (strncmp(url, "mem:", 4) == 0)
    {
        return connect_memory(url + 4);
    } else {
        unique_ptr<sql::Connection> conn(sql::Connection::create_from_url(url));
        return create(move(conn));
    }
}

unique_ptr<DB> DB::connect_memory(const std::string& arg)
{
    if (arg.empty())
        return unique_ptr<DB>(new mem::DB());
    else
        return unique_ptr<DB>(new mem::DB(arg));
}

unique_ptr<DB> DB::connect_test()
{
    if (default_format == MEM)
        return connect_memory();

    const char* envurl = getenv("DBA_DB");
    if (envurl != NULL)
        return connect_from_url(envurl);
    else
        return connect_from_file("test.sqlite");
}

const char* DB::default_repinfo_file()
{
    const char* repinfo_file = getenv("DBA_REPINFO");
    if (repinfo_file == 0 || repinfo_file[0] == 0)
        repinfo_file = TABLE_DIR "/repinfo.csv";
    return repinfo_file;
}

void DB::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    auto t = transaction();
    insert_station_data(*t, vals, can_replace, station_can_add);
    t->commit();
}

void DB::insert_data(DataValues& vals, bool can_replace, bool station_can_add)
{
    auto t = transaction();
    insert_data(*t, vals, can_replace, station_can_add);
    t->commit();
}

void DB::remove_station_data(const Query& query)
{
    auto t = transaction();
    remove_station_data(*t, query);
    t->commit();
}

void DB::remove(const Query& query)
{
    auto t = transaction();
    remove(*t, query);
    t->commit();
}

void DB::remove_all()
{
    auto t = transaction();
    remove_all(*t);
    t->commit();
}

void DB::attr_insert_station(int data_id, const Values& attrs)
{
    auto t = transaction();
    attr_insert_station(*t, data_id, attrs);
    t->commit();
}

void DB::attr_insert_data(int data_id, const Values& attrs)
{
    auto t = transaction();
    attr_insert_data(*t, data_id, attrs);
    t->commit();
}

void DB::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    auto t = transaction();
    attr_remove_station(*t, data_id, attrs);
    t->commit();
}

void DB::attr_remove_data(int data_id, const db::AttrList& attrs)
{
    auto t = transaction();
    attr_remove_data(*t, data_id, attrs);
    t->commit();
}

void DB::import_msg(const Message& msg, const char* repmemo, int flags)
{
    auto t = transaction();
    import_msg(*t, msg, repmemo, flags);
    t->commit();
}

void DB::import_msgs(const Messages& msgs, const char* repmemo, int flags)
{
    auto t = transaction();
    import_msgs(*t, msgs, repmemo, flags);
    t->commit();
}

void DB::import_msgs(dballe::Transaction& transaction, const Messages& msgs, const char* repmemo, int flags)
{
    for (const auto& i: msgs)
        import_msg(transaction, i, repmemo, flags);
}

bool DB::export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest)
{
    auto t = transaction();
    bool res = export_msgs(*t, query, dest);
    t->commit();
    return res;
}

}
