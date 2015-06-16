/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/tests.h"
#include "db/v6/db.h"
#include "db/sql.h"
#include "db/sql/driver.h"
#include "db/sql/repinfo.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace std;
using namespace wreport;
using namespace wibble::tests;

namespace {

struct Fixture : dballe::tests::DriverFixture
{
    unique_ptr<sql::Repinfo> repinfo;

    Fixture()
    {
        reset_repinfo();
    }

    void reset_repinfo()
    {
        if (conn->has_table("repinfo"))
            driver->exec_no_data("DELETE FROM repinfo");

        switch (format)
        {
            case V5: throw error_unimplemented("v5 db is not supported");
            case V6:
                repinfo = driver->create_repinfov6();
                break;
            default:
                throw error_consistency("cannot test repinfo on the current DB format");
        }
        int added, deleted, updated;
        repinfo->update(nullptr, &added, &deleted, &updated);
    }

    void reset()
    {
        dballe::tests::DriverFixture::reset();
        reset_repinfo();
    }

};

typedef dballe::tests::driver_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    // Test simple queries
    Test("query", [](Fixture& f) {
        auto& ri = *f.repinfo;
        wassert(actual(ri.get_id("synop")) == 1);
        wassert(actual(ri.get_id("generic")) == 255);
        wassert(actual(ri.get_rep_memo(1)) == "synop");
        wassert(actual(ri.get_priority(199)) == INT_MAX);
    }),
    // Test update
    Test("update", [](Fixture& f) {
        auto& ri = *f.repinfo;

        wassert(actual(ri.get_id("synop")) == 1);

        int added, deleted, updated;
        ri.update(NULL, &added, &deleted, &updated);

        wassert(actual(added) == 0);
        wassert(actual(deleted) == 0);
        wassert(actual(updated) == 13);

        wassert(actual(ri.get_id("synop")) == 1);
    }),
    // Test update from a file that was known to fail
    Test("fail", [](Fixture& f) {
        auto& ri = *f.repinfo;

        wassert(actual(ri.get_id("synop")) == 1);

        int added, deleted, updated;
        ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

        wassert(actual(added) == 3);
        wassert(actual(deleted) == 11);
        wassert(actual(updated) == 2);

        wassert(actual(ri.get_id("synop")) == 1);
        wassert(actual(ri.get_id("FIXspnpo")) == 201);
    }),
    // Test update from a file with a negative priority
    Test("fail", [](Fixture& f) {
        auto& ri = *f.repinfo;

        int id = ri.get_id("generic");
        wassert(actual(ri.get_priority(id)) == 1000);

        int added, deleted, updated;
        ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

        wassert(actual(added) == 3);
        wassert(actual(deleted) == 11);
        wassert(actual(updated) == 2);

        wassert(actual(ri.get_priority(id)) == -5);
    }),
    // Test automatic repinfo creation
    Test("fail", [](Fixture& f) {
        auto& ri = *f.repinfo;

        int id = ri.obtain_id("foobar");
        wassert(actual(id) > 0);
        wassert(actual(ri.get_rep_memo(id)) == "foobar");
        wassert(actual(ri.get_priority(id)) == 1001);

        id = ri.obtain_id("barbaz");
        wassert(actual(id) > 0);
        wassert(actual(ri.get_rep_memo(id)) == "barbaz");
        wassert(actual(ri.get_priority(id)) == 1002);
    }),
};

test_group tg2("db_sql_repinfo_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_sql_repinfo_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_sql_repinfo_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_sql_repinfo_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
