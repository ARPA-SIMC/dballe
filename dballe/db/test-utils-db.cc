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

namespace dballe {
namespace tests {

db_test::db_test()
{
    orig_format = DB::get_default_format();
}

db_test::db_test(db::Format format, bool reset)
{
    orig_format = DB::get_default_format();
    DB::set_default_format(format);
    db = DB::connect_test();
    if (reset) db->reset();
}

db_test::~db_test()
{
    DB::set_default_format(orig_format);
}

void db_test::use_db(bool reset)
{
    if (!has_db()) throw tut::no_such_test();
    if (reset) db->reset();
}

void db_test::use_db(db::Format format, bool reset)
{
    if (!db.get())
    {
        DB::set_default_format(format);
        db = DB::connect_test();
    }
    use_db(reset);
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
    // Common data (ana)
    sampleAna.set(DBA_KEY_LAT, 12.34560);
    sampleAna.set(DBA_KEY_LON, 76.54320);
    sampleAna.set(DBA_KEY_MOBILE, 0);

    // Extra ana info
    extraAna.set(WR_VAR(0, 7, 30), 42);     // Height
    extraAna.set(WR_VAR(0, 7, 31), 234);        // Heightbaro
    extraAna.set(WR_VAR(0, 1,  1), 1);          // Block
    extraAna.set(WR_VAR(0, 1,  2), 52);     // Station
    extraAna.set(WR_VAR(0, 1, 19), "Cippo Lippo");  // Name

    // Common data
    sampleBase.set(DBA_KEY_YEAR, 1945);
    sampleBase.set(DBA_KEY_MONTH, 4);
    sampleBase.set(DBA_KEY_DAY, 25);
    sampleBase.set(DBA_KEY_HOUR, 8);
    sampleBase.set(DBA_KEY_LEVELTYPE1, 10);
    sampleBase.set(DBA_KEY_L1, 11);
    sampleBase.set(DBA_KEY_LEVELTYPE2, 15);
    sampleBase.set(DBA_KEY_L2, 22);
    sampleBase.set(DBA_KEY_PINDICATOR, 20);
    sampleBase.set(DBA_KEY_P1, 111);

    // Specific data
    sample0.set(DBA_KEY_MIN, 0);
    sample0.set(DBA_KEY_P2, 122);
    sample0.set(DBA_KEY_REP_COD, 1);
    sample0.set(DBA_KEY_PRIORITY, 101);

    sample00.set(WR_VAR(0, 1, 11), "DB-All.e!");
    sample01.set(WR_VAR(0, 1, 12), 300);

    sample1.set(DBA_KEY_MIN, 30);
    sample1.set(DBA_KEY_P2, 123);
    sample1.set(DBA_KEY_REP_COD, 2);
    sample1.set(DBA_KEY_PRIORITY, 81);

    sample10.set(WR_VAR(0, 1, 11), "Arpa-Sim!");
    sample11.set(WR_VAR(0, 1, 12), 400);
}

void DB_test_base::populate_database()
{
    /* Start with an empty database */
    db->reset();

    /* Insert the ana station */
    insert.clear();
    insert.set_ana_context();
    insert.key(DBA_KEY_REP_MEMO).setc("synop");
    insert.add(sampleAna);
    insert.add(extraAna);
    /* Insert the anagraphical record */
    db->insert(insert, false, true);

    /* Insert the ana info also for rep_cod 2 */
    insert.key(DBA_KEY_REP_MEMO).setc("metar");
    insert.unset(DBA_KEY_CONTEXT_ID);
    db->insert(insert, false, true);

    // Insert a record
    insert.clear();
    insert.add(sampleAna);
    insert.add(sampleBase);
    insert.add(sample0);
    insert.add(sample00);
    insert.add(sample01);
    db->insert(insert, false, false);

    // Insert another record (similar but not the same)
    insert.clear();
    insert.add(sampleAna);
    insert.add(sampleBase);
    insert.add(sample1);
    insert.add(sample10);
    insert.add(sample11);
    db->insert(insert, false, false);
}

} // namespace tests
} // namespace dballe
