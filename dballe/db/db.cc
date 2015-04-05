/*
 * dballe/db - Archive for point-based meteorological data
 *
 * Copyright (C) 2013--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "config.h"
#include "db.h"
#include "v5/db.h"
#include "v6/db.h"
#include "mem/db.h"
#include "sqlite/internals.h"
#ifdef HAVE_ODBC
#include "odbc/internals.h"
#endif
#ifdef HAVE_LIBPQ
#include "postgresql/internals.h"
#endif
#include "dballe/msg/msgs.h"
#include <wreport/error.h>
#include <cstring>
#include <cstdlib>

using namespace dballe::db;
using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

#ifdef HAVE_ODBC
static Format default_format = V6;
#else
static Format default_format = MEM;
#endif

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

void DB::import_msgs(const Msgs& msgs, const char* repmemo, int flags)
{
    for (Msgs::const_iterator i = msgs.begin(); i != msgs.end(); ++i)
        import_msg(**i, repmemo, flags);
}

Format DB::get_default_format() { return default_format; }
void DB::set_default_format(Format format) { default_format = format; }

bool DB::is_url(const char* str)
{
    if (strncmp(str, "mem:", 4) == 0) return true;
    if (strncmp(str, "sqlite:", 7) == 0) return true;
    if (strncmp(str, "odbc://", 7) == 0) return true;
    if (strncmp(str, "postgresql:", 11) == 0) return true;
    if (strncmp(str, "test:", 5) == 0) return true;
    return false;
}

unique_ptr<DB> DB::instantiate_db(unique_ptr<Connection> conn)
{
    // Autodetect format
    Format format = default_format;

    bool found = true;

    // Try with reading it from the settings table
    string version = conn->get_setting("version");
    if (version == "V5")
        format = V5;
    else if (version == "V6")
        format = V6;
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
        case V5:
#ifdef HAVE_ODBC
            if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(conn.get()))
            {
                conn.release();
                unique_ptr<ODBCConnection> oc(c);
                return unique_ptr<DB>(new v5::DB(move(oc)));
            } else {
                throw error_consistency("cannot open a v5 DB with a non-ODBC connector; for example, cannot open a v5 database in a SQLite file or PostgreSQL database");
            }
#else
            throw error_unimplemented("ODBC support is not available");
#endif
        case V6: return unique_ptr<DB>(new v6::DB(unique_ptr<Connection>(conn.release())));
        default: error_consistency::throwf("requested unknown format %d", (int)format);
    }
}

unique_ptr<DB> DB::connect(const char* dsn, const char* user, const char* password)
{
#ifdef HAVE_ODBC
    unique_ptr<ODBCConnection> conn(new ODBCConnection);
    conn->connect(dsn, user, password);
    return instantiate_db(move(conn));
#else
    throw error_unimplemented("ODBC support is not available");
#endif
}

unique_ptr<DB> DB::connect_from_file(const char* pathname)
{
    unique_ptr<SQLiteConnection> conn(new SQLiteConnection);
    conn->open_file(pathname);
    return instantiate_db(unique_ptr<Connection>(conn.release()));
}

unique_ptr<DB> DB::connect_from_url(const char* url)
{
    if (strncmp(url, "sqlite://", 9) == 0)
    {
        return connect_from_file(url + 9);
    }
    if (strncmp(url, "sqlite:", 7) == 0)
    {
        return connect_from_file(url + 7);
    }
    if (strncmp(url, "mem:", 4) == 0)
    {
        return connect_memory(url + 4);
    }
    if (strncmp(url, "postgresql:", 11) == 0)
    {
        unique_ptr<PostgreSQLConnection> conn(new PostgreSQLConnection);
        conn->open(url);
        return instantiate_db(unique_ptr<Connection>(conn.release()));
    }
    if (strncmp(url, "odbc://", 7) == 0)
    {
#ifdef HAVE_ODBC
        string buf(url + 7);
        size_t pos = buf.find('@');
        if (pos == string::npos)
        {
            return connect(buf.c_str(), "", ""); // odbc://dsn
        }
        // Split the string at '@'
        string userpass = buf.substr(0, pos);
        string dsn = buf.substr(pos + 1);

        pos = userpass.find(':');
        if (pos == string::npos)
        {
            return connect(dsn.c_str(), userpass.c_str(), ""); // odbc://user@dsn
        }

        string user = userpass.substr(0, pos);
        string pass = userpass.substr(pos + 1);

        return connect(dsn.c_str(), user.c_str(), pass.c_str()); // odbc://user:pass@dsn
#else
        throw error_unimplemented("ODBC support is not available");
#endif
    }
    if (strncmp(url, "test:", 5) == 0)
    {
        return connect_test();
    }
    error_consistency::throwf("unknown url \"%s\"", url);
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

}

/* vim:set ts=4 sw=4: */
