#include "config.h"
#include "db.h"
#include "v7/db.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/sqlite.h"
#include "dballe/message.h"
#include "dballe/core/values.h"
#include "dballe/file.h"
#include <wreport/error.h>
#include <cstring>
#include <cstdlib>

using namespace dballe::db;
using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

static Format default_format = V7;

std::string format_format(Format format)
{
    switch (format)
    {
        case V5: return "V5";
        case V6: return "V6";
        case MEM: return "MEM";
        case MESSAGES: return "MESSAGES";
        case V7: return "V7";
        default: return "unknown format " + std::to_string((int)format);
    }
}

Format format_parse(const std::string& str)
{
    if (str == "V7") return V7;
    if (str == "V6") return V6;
    if (str == "V5") return V5;
    if (str == "MEM") return MEM;
    if (str == "MESSAGES") return MESSAGES;
    error_consistency::throwf("unsupported database format: '%s'", str.c_str());
}

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

void Transaction::import_msgs(const std::vector<std::shared_ptr<Message>>& msgs, const char* repmemo, int flags)
{
    for (const auto& i: msgs)
        import_msg(*i, repmemo, flags);
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
    if (strncmp(str, "test:", 5) == 0) return true;
    return false;
}

shared_ptr<DB> DB::create(unique_ptr<sql::Connection> conn)
{
    // Autodetect format
    Format format = default_format;

    const char* format_override = getenv("DBA_DB_FORMAT");
    if (format_override)
        format = format_parse(format_override);

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
    }

    switch (format)
    {
        case V5: throw error_unimplemented("V5 format is not supported anymore by this version of DB-All.e");
        case V6: throw error_unimplemented("V6 format is not supported anymore by this version of DB-All.e");
        case V7: return static_pointer_cast<DB>(make_shared<v7::DB>(move(conn)));
        default: error_consistency::throwf("requested unknown format %d", (int)format);
    }
}

shared_ptr<DB> DB::connect_from_file(const char* pathname)
{
    unique_ptr<sql::SQLiteConnection> conn(new sql::SQLiteConnection);
    conn->open_file(pathname);
    return create(unique_ptr<sql::Connection>(conn.release()));
}

shared_ptr<DB> DB::connect_from_url(const char* url)
{
    if (strncmp(url, "mem:", 4) == 0)
    {
        return connect_memory(url + 4);
    } else {
        unique_ptr<sql::Connection> conn(sql::Connection::create_from_url(url));
        return create(move(conn));
    }
}

shared_ptr<DB> DB::connect_memory(const std::string& arg)
{
    sql::SQLiteConnection* sqlite_conn;

    unique_ptr<sql::Connection> conn(sqlite_conn = new sql::SQLiteConnection);
    sqlite_conn->open_memory();
    auto res = static_pointer_cast<DB>(make_shared<v7::DB>(move(conn)));
    res->reset();
    return res;
}

shared_ptr<DB> DB::connect_test()
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

std::unique_ptr<db::CursorStation> DB::query_stations(const Query& query)
{
    auto t = transaction();
    auto res = t->query_stations(query);
    return res;
}

std::unique_ptr<db::CursorStationData> DB::query_station_data(const Query& query)
{
    auto t = transaction();
    auto res = t->query_station_data(query);
    return res;
}

std::unique_ptr<db::CursorData> DB::query_data(const Query& query)
{
    auto t = transaction();
    auto res = t->query_data(query);
    return res;
}

std::unique_ptr<db::CursorSummary> DB::query_summary(const Query& query)
{
    auto t = transaction();
    auto res = t->query_summary(query);
    return res;
}

void DB::attr_query_station(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    auto t = transaction();
    t->attr_query_station(data_id, move(dest));
    t->commit();
}

void DB::attr_query_data(int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    auto t = transaction();
    t->attr_query_data(data_id, move(dest));
    t->commit();
}

void DB::insert_station_data(StationValues& vals, bool can_replace, bool station_can_add)
{
    auto t = transaction();
    t->insert_station_data(vals, can_replace, station_can_add);
    t->commit();
}

void DB::insert_data(DataValues& vals, bool can_replace, bool station_can_add)
{
    auto t = transaction();
    t->insert_data(vals, can_replace, station_can_add);
    t->commit();
}

void DB::remove_station_data(const Query& query)
{
    auto t = transaction();
    t->remove_station_data(query);
    t->commit();
}

void DB::remove(const Query& query)
{
    auto t = transaction();
    t->remove(query);
    t->commit();
}

void DB::remove_all()
{
    auto t = transaction();
    t->remove_all();
    t->commit();
}

void DB::attr_insert_station(int data_id, const Values& attrs)
{
    auto t = transaction();
    t->attr_insert_station(data_id, attrs);
    t->commit();
}

void DB::attr_insert_data(int data_id, const Values& attrs)
{
    auto t = transaction();
    t->attr_insert_data(data_id, attrs);
    t->commit();
}

void DB::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    auto t = transaction();
    t->attr_remove_station(data_id, attrs);
    t->commit();
}

void DB::attr_remove_data(int data_id, const db::AttrList& attrs)
{
    auto t = transaction();
    t->attr_remove_data(data_id, attrs);
    t->commit();
}

void DB::import_msg(const Message& msg, const char* repmemo, int flags)
{
    auto t = transaction();
    t->import_msg(msg, repmemo, flags);
    t->commit();
}

void DB::import_msgs(const Messages& msgs, const char* repmemo, int flags)
{
    auto t = transaction();
    t->import_msgs(msgs, repmemo, flags);
    t->commit();
}

bool DB::export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest)
{
    auto t = transaction();
    bool res = t->export_msgs(query, dest);
    t->commit();
    return res;
}

void DB::dump(FILE* out)
{
    auto t = transaction();
    t->dump(out);
    t->rollback();
}

void DB::print_info(FILE* out)
{
    fprintf(out, "Format: %s\n", format_format(format()).c_str());
}

}
