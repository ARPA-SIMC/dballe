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

#include "db/test-utils-db.h"
#include "db/v6/db.h"
#include "db/sql/repinfo.h"
#include "db/sql/station.h"
#include "db/sql/levtr.h"
#include "db/sql/datav6.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct Fixture : dballe::tests::DriverFixture
{
    unique_ptr<db::sql::DataV6> data;

    Fixture()
    {
        reset_data();

        auto st = driver->create_stationv6();
        auto lt = driver->create_levtrv6();

        int added, deleted, updated;
        driver->create_repinfov6()->update(nullptr, &added, &deleted, &updated);

        // Insert a mobile station
        wassert(actual(st->obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st->obtain_id(4600000, 1200000)) == 2);

        // Insert a lev_tr
        wassert(actual(lt->obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

        // Insert another lev_tr
        wassert(actual(lt->obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);
    }

    void reset_data()
    {
        driver->exec_no_data("DELETE FROM data");
        data = driver->create_datav6();
    }

    void reset()
    {
        dballe::tests::DriverFixture::reset();
        reset_data();
    }
};

typedef dballe::tests::driver_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("insert", [](Fixture& f) {
        auto& da = *f.data;

        // Insert a datum
        da.set_context(1, 1, 1);
        da.set_date(2001, 2, 3, 4, 5, 6);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 123));

        // Insert another datum
        da.set_context(2, 2, 2);
        da.set_date(2002, 3, 4, 5, 6, 7);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 234));

        // Reinsert a datum: it should fail
        da.set_context(1, 1, 1);
        da.set_date(2001, 2, 3, 4, 5, 6);
        try {
            da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 123));
            ensure(false);
        } catch (db::error& e) {
            //ensure_contains(e.what(), "uplicate");
        }

        // Reinsert the other datum: it should fail
        da.set_context(2, 2, 2);
        da.set_date(2002, 3, 4, 5, 6, 7);
        try {
            da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 234));
            ensure(false);
        } catch (db::error& e) {
            //ensure_contains(e.what(), "uplicate");
        }

        // Reinsert a datum with overwrite: it should work
        da.set_context(1, 1, 1);
        da.set_date(2001, 2, 3, 4, 5, 6);
        da.insert_or_overwrite(Var(varinfo(WR_VAR(0, 1, 2)), 123));

        // Reinsert the other datum with overwrite: it should work
        da.set_context(2, 2, 2);
        da.set_date(2002, 3, 4, 5, 6, 7);
        da.insert_or_overwrite(Var(varinfo(WR_VAR(0, 1, 2)), 234));

        // Insert a new datum with ignore: it should insert
        da.set_context(2, 2, 3);
        wassert(actual(da.insert_or_ignore(Var(varinfo(WR_VAR(0, 1, 2)), 234))) == true);

        // Reinsert the same datum with ignore: it should ignore
        wassert(actual(da.insert_or_ignore(Var(varinfo(WR_VAR(0, 1, 2)), 234))) == false);

        // Reinsert a nonexisting datum with overwrite: it should work
        da.set_context(1, 1, 1);
        da.set_date(2005, 2, 3, 4, 5, 6);
        da.insert_or_overwrite(Var(varinfo(WR_VAR(0, 1, 2)), 123));
    }),
};

test_group tg1("db_sql_data_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg2("db_sql_data_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg3("db_sql_data_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg4("db_sql_data_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
