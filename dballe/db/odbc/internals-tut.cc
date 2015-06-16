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

#include "db/tests.h"
#include "db/odbc/internals.h"
#include <sql.h>

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct Fixture : public dballe::tests::DBFixture
{
    ODBCConnection conn;

    Fixture()
    {
        conn.connect_test();
    }

    void reset()
    {
        conn.drop_table_if_exists("dballe_test");
        conn.exec("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};

typedef dballe::tests::test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("query_int", [](Fixture& f) {
        // Test querying int values
        auto& c = f.conn;
        c.exec("INSERT INTO dballe_test VALUES (42)");

        auto s = c.odbcstatement("SELECT val FROM dballe_test");
        int val = 0;
        s->bind_out(1, val);
        s->execute();
        unsigned count = 0;
        while (s->fetch())
            ++count;
        wassert(actual(val) == 42);
        wassert(actual(count) == 1);
    }),
    Test("query_int_null", [](Fixture& f) {
        // Test querying int values, with indicators
        auto& c = f.conn;
        c.exec("INSERT INTO dballe_test VALUES (42)");

        auto s = c.odbcstatement("SELECT val FROM dballe_test");
        int val = 0;
        SQLLEN ind = 0;
        s->bind_out(1, val, ind);
        s->execute();
        unsigned count = 0;
        while (s->fetch())
            ++count;
        wassert(actual(val) == 42);
        wassert(actual(ind) != SQL_NULL_DATA);
        wassert(actual(count) == 1);
    }),
    Test("query_unsigned", [](Fixture& f) {
        // Test querying unsigned values
        auto& c = f.conn;
        c.exec("INSERT INTO dballe_test VALUES (42)");

        auto s = c.odbcstatement("SELECT val FROM dballe_test");
        unsigned val = 0;
        s->bind_out(1, val);
        s->execute();
        unsigned count = 0;
        while (s->fetch())
            ++count;
        wassert(actual(val) == 42);
        wassert(actual(count) == 1);
    }),
    Test("query_unsigned_null", [](Fixture& f) {
        // Test querying unsigned values, with indicators
        auto& c = f.conn;
        c.exec("INSERT INTO dballe_test VALUES (42)");

        auto s = c.odbcstatement("SELECT val FROM dballe_test");
        unsigned val = 0;
        SQLLEN ind = 0;
        s->bind_out(1, val, ind);
        s->execute();
        unsigned count = 0;
        while (s->fetch())
            ++count;
        wassert(actual(val) == 42);
        wassert(actual(ind) != SQL_NULL_DATA);
        wassert(actual(count) == 1);
    }),
    Test("query_unsigned_short", [](Fixture& f) {
        // Test querying unsigned short values
        auto& c = f.conn;
        c.exec("INSERT INTO dballe_test VALUES (42)");
        auto s = c.odbcstatement("SELECT val FROM dballe_test");
        unsigned short val = 0;
        s->bind_out(1, val);
        s->execute();
        unsigned count = 0;
        while (s->fetch())
            ++count;
        wassert(actual(val) == 42);
        wassert(actual(count) == 1);
    }),
    Test("has_tables", [](Fixture& f) {
        // Test has_tables
        auto& c = f.conn;
        ensure(!c.has_table("this_should_not_exist"));
        ensure(c.has_table("dballe_test"));
    }),
    Test("settings", [](Fixture& f) {
        // Test settings
        auto& c = f.conn;
        c.drop_table_if_exists("dballe_settings");
        ensure(!c.has_table("dballe_settings"));

        ensure_equals(c.get_setting("test_key"), "");

        c.set_setting("test_key", "42");
        ensure(c.has_table("dballe_settings"));

        ensure_equals(c.get_setting("test_key"), "42");
    }),
};

test_group tg1("db_internals_odbc", tests);

}
