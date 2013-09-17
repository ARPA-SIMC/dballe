/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "test-utils-db.h"
#include <dballe/db/internals.h>
#include "dballe/db/v5/db.h"
#include "dballe/db/v6/db.h"
#include <wreport/error.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace tests {

void TestRecord::insert(DB& db, bool can_replace)
{
    if (!station_data.vars().empty())
    {
        Record insert(station_data);
        insert.copy(data, DBA_KEY_REP_MEMO);
        insert.copy(data, DBA_KEY_LAT);
        insert.copy(data, DBA_KEY_LON);
        insert.set_ana_context();
        db.insert(insert, can_replace, true);
    }

    if (!data.vars().empty())
    {
        db.insert(data, can_replace, true);
    }

    for (std::map<wreport::Varcode, Record>::const_iterator i = attrs.begin();
            i != attrs.end(); ++i)
    {
        db.attr_insert(i->first, i->second, can_replace);
    }
}

bool TestRecord::match_station_keys(const Record& rec)
{
    if (!data.contains(rec, DBA_KEY_LAT)) return false;
    if (!data.contains(rec, DBA_KEY_LON)) return false;
    if (!data.contains(rec, DBA_KEY_IDENT)) return false;
    return true;
}

bool TestRecord::match_context_keys(const Record& rec)
{
    if (!match_station_keys(rec)) return false;
    if (!data.contains(rec, DBA_KEY_REP_MEMO)) return false;
    if (!data.contains(rec, DBA_KEY_LEVELTYPE1)) return false;
    if (!data.contains(rec, DBA_KEY_L1)) return false;
    if (!data.contains(rec, DBA_KEY_LEVELTYPE2)) return false;
    if (!data.contains(rec, DBA_KEY_L2)) return false;
    if (!data.contains(rec, DBA_KEY_PINDICATOR)) return false;
    if (!data.contains(rec, DBA_KEY_P1)) return false;
    if (!data.contains(rec, DBA_KEY_P2)) return false;
    if (!data.contains(rec, DBA_KEY_YEAR)) return false;
    if (!data.contains(rec, DBA_KEY_MONTH)) return false;
    if (!data.contains(rec, DBA_KEY_DAY)) return false;
    if (!data.contains(rec, DBA_KEY_HOUR)) return false;
    if (!data.contains(rec, DBA_KEY_MIN)) return false;
    if (!data.contains(rec, DBA_KEY_SEC)) return false;
    return true;
}

bool TestRecord::match_data_var(wreport::Varcode code, const Record& rec)
{
    return data.contains(rec, code);
}

db_test::db_test()
{
    orig_format = DB::get_default_format();
}

db_test::db_test(db::Format format, bool reset)
{
    orig_format = DB::get_default_format();
    if (reset) disappear();
    DB::set_default_format(format);
    db = DB::connect_test();
    if (reset) db->reset();
}

db_test::~db_test()
{
    DB::set_default_format(orig_format);
}

void db_test::disappear()
{
    auto_ptr<DB> db(DB::connect_test());
    db->disappear();
}

void db_test::use_db()
{
    if (!has_db()) throw tut::no_such_test();
}

void db_test::use_db(db::Format format, bool reset)
{
    if (getenv("DBALLE_SKIP_DB_TESTS")) throw tut::no_such_test();

    if (!db.get())
    {
        if (reset) disappear();
        DB::set_default_format(format);
        db = DB::connect_test();
    }
    if (db->format() != format)
        throw error_consistency("DB format mismatch in test init");
    use_db();
    if (reset) db->reset();
}

db::v5::DB& db_test::v5()
{
    if (db::v5::DB* d = dynamic_cast<db::v5::DB*>(db.get()))
        return *d;
    else
        throw error_consistency("test DB is not a v5 DB");
}

db::v6::DB& db_test::v6()
{
    if (db::v6::DB* d = dynamic_cast<db::v6::DB*>(db.get()))
        return *d;
    else
        throw error_consistency("test DB is not a v6 DB");
}

DB_test_base::DB_test_base()
{
    init_records();
}
DB_test_base::DB_test_base(db::Format format) : db_test(format)
{
    init_records();
}

void DB_test_base::init_records()
{
    dataset1.data.set(DBA_KEY_REP_MEMO, "metar");
    dataset1.data.set(DBA_KEY_MIN, 30);
    dataset1.data.set(DBA_KEY_P2, 123);
    dataset1.data.clear_vars();
    dataset1.data.set(WR_VAR(0, 1, 11), "Arpa-Sim!");
    dataset1.data.set(WR_VAR(0, 1, 12), 400);
}

void DB_test_base::populate_database()
{
    /* Start with an empty database */
    db->reset();

    dataset0.insert(*db);
    dataset1.insert(*db);
}

} // namespace tests
} // namespace dballe
