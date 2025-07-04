#include "db.h"
#include "config.h"
#include "dballe/file.h"
#include "dballe/message.h"
#include "dballe/sql/sql.h"
#include "dballe/sql/sqlite.h"
#include "v7/db.h"
#include <cstdlib>
#include <cstring>
#include <wreport/error.h>

using namespace dballe::db;
using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

void CursorStationData::insert_attrs(const Values& attrs)
{
    get_transaction()->attr_insert_station(attr_reference_id(), attrs);
}

void CursorStationData::remove_attrs(const db::AttrList& attrs)
{
    get_transaction()->attr_remove_station(attr_reference_id(), attrs);
}

void CursorData::insert_attrs(const Values& attrs)
{
    get_transaction()->attr_insert_data(attr_reference_id(), attrs);
}

void CursorData::remove_attrs(const db::AttrList& attrs)
{
    get_transaction()->attr_remove_data(attr_reference_id(), attrs);
}

static Format default_format = Format::V7;

std::string format_format(Format format)
{
    switch (format)
    {
        case Format::V5:       return "V5";
        case Format::V6:       return "V6";
        case Format::MEM:      return "MEM";
        case Format::MESSAGES: return "MESSAGES";
        case Format::V7:       return "V7";
        default:               return "unknown format " + std::to_string((int)format);
    }
}

Format format_parse(const std::string& str)
{
    if (str == "V7")
        return Format::V7;
    if (str == "V6")
        return Format::V6;
    if (str == "V5")
        return Format::V5;
    if (str == "MEM")
        return Format::MEM;
    if (str == "MESSAGES")
        return Format::MESSAGES;
    error_consistency::throwf("unsupported database format: '%s'", str.c_str());
}

Format DB::get_default_format() { return default_format; }
void DB::set_default_format(Format format) { default_format = format; }

bool DB::is_url(const char* str)
{
    if (strncmp(str, "mem:", 4) == 0)
        return true;
    if (strncmp(str, "sqlite:", 7) == 0)
        return true;
    if (strncmp(str, "postgresql:", 11) == 0)
        return true;
    if (strncmp(str, "mysql:", 6) == 0)
        return true;
    if (strncmp(str, "test:", 5) == 0)
        return true;
    return false;
}

std::shared_ptr<DB> DB::create(std::shared_ptr<sql::Connection> conn)
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
        format = Format::V5;
    else if (version == "V6")
        format = Format::V6;
    else if (version == "V7")
        format = Format::V7;
    else if (version == "")
        found =
            false; // Some other key exists, but the version has not been set
    else
        error_consistency::throwf("unsupported database version: '%s'",
                                  version.c_str());

    // If it failed, try looking at the existing table structure
    if (!found)
    {
        if (conn->has_table("lev_tr"))
            format = Format::V6;
        else if (conn->has_table("context"))
            format = Format::V5;
    }

    switch (format)
    {
        case Format::V5:
            throw error_unimplemented("V5 format is not supported anymore by "
                                      "this version of DB-All.e");
        case Format::V6:
            throw error_unimplemented("V6 format is not supported anymore by "
                                      "this version of DB-All.e");
        case Format::V7:
            return static_pointer_cast<DB>(make_shared<v7::DB>(conn));
        default:
            error_consistency::throwf("requested unknown format %d",
                                      (int)format);
    }
}

shared_ptr<DB> DB::connect_from_file(const char* pathname)
{
    auto conn = sql::SQLiteConnection::create();
    conn->open_file(pathname);
    return create(conn);
}

shared_ptr<DB> DB::connect_memory()
{
    auto conn = sql::SQLiteConnection::create();
    conn->open_memory();
    auto res = static_pointer_cast<DB>(make_shared<v7::DB>(conn));
    res->reset();
    return res;
}

const char* DB::default_repinfo_file()
{
    const char* repinfo_file = getenv("DBA_REPINFO");
    if (repinfo_file == 0 || repinfo_file[0] == 0)
        repinfo_file = TABLE_DIR "/repinfo.csv";
    return repinfo_file;
}

void DB::attr_query_station(
    int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_query_station(data_id, move(dest));
    t->commit();
}

void DB::attr_query_data(
    int data_id, std::function<void(std::unique_ptr<wreport::Var>)>&& dest)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_query_data(data_id, move(dest));
    t->commit();
}

void DB::attr_insert_station(int data_id, const Values& attrs)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_insert_station(data_id, attrs);
    t->commit();
}

void DB::attr_insert_data(int data_id, const Values& attrs)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_insert_data(data_id, attrs);
    t->commit();
}

void DB::attr_remove_station(int data_id, const db::AttrList& attrs)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_remove_station(data_id, attrs);
    t->commit();
}

void DB::attr_remove_data(int data_id, const db::AttrList& attrs)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->attr_remove_data(data_id, attrs);
    t->commit();
}

void DB::dump(FILE* out)
{
    auto t = dynamic_pointer_cast<db::Transaction>(transaction());
    t->dump(out);
    t->rollback();
}

void DB::print_info(FILE* out)
{
    fprintf(out, "Format: %s\n", format_format(format()).c_str());
}

} // namespace db
} // namespace dballe
