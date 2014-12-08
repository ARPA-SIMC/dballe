/*
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "internals.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db_sqlite_internals_shar
{
    SQLiteConnection conn;

    db_sqlite_internals_shar()
    {
        conn.open_memory();
    }

    ~db_sqlite_internals_shar()
    {
    }

    void reset()
    {
        conn.drop_table_if_exists("dballe_test");
        conn.exec("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};
TESTGRP(db_sqlite_internals);

// Test querying int values
template<> template<>
void to::test<1>()
{
    reset();

    conn.exec("INSERT INTO dballe_test VALUES (1)");
    conn.exec("INSERT INTO dballe_test VALUES (?)", 2);

    auto s = conn.sqlitestatement("SELECT val FROM dballe_test");

    int val = 0;
    unsigned count = 0;
    s->execute([&]() {
        val += s->column_int(0);
        ++count;
    });
    wassert(actual(count) == 2);
    wassert(actual(val) == 3);
}

// Test querying int values, with potential NULLs
template<> template<>
void to::test<2>()
{
    reset();

    conn.exec("CREATE TABLE dballe_testnull (val INTEGER)");
    conn.exec("INSERT INTO dballe_testnull VALUES (NULL)");
    conn.exec("INSERT INTO dballe_testnull VALUES (42)");

    auto s = conn.sqlitestatement("SELECT val FROM dballe_testnull");

    int val = 0;
    unsigned count = 0;
    unsigned countnulls = 0;
    s->execute([&]() {
        if (s->column_isnull(0))
            ++countnulls;
        else
            val += s->column_int(0);
        ++count;
    });

    wassert(actual(val) == 42);
    wassert(actual(count) == 2);
    wassert(actual(countnulls) == 1);
}

// Test querying unsigned values
template<> template<>
void to::test<3>()
{
    reset();

    conn.exec("INSERT INTO dballe_test VALUES (?)", 0xFFFFFFFE);

    auto s = conn.sqlitestatement("SELECT val FROM dballe_test");

    unsigned val = 0;
    unsigned count = 0;
    s->execute([&]() {
        val += s->column_int64(0);
        ++count;
    });
    wassert(actual(count) == 1);
    wassert(actual(val) == 0xFFFFFFFE);
}

// Test querying unsigned short values
template<> template<>
void to::test<5>()
{
    reset();

    conn.exec("INSERT INTO dballe_test VALUES (?)", WR_VAR(3, 1, 12));

    auto s = conn.sqlitestatement("SELECT val FROM dballe_test");

    Varcode val = 0;
    unsigned count = 0;
    s->execute([&]() {
        val = (Varcode)s->column_int(0);
        ++count;
    });
    wassert(actual(count) == 1);
    wassert(actual(val) == WR_VAR(3, 1, 12));
}

// Test has_tables
template<> template<>
void to::test<6>()
{
    reset();

    wassert(actual(conn.has_table("this_should_not_exist")).isfalse());
    wassert(actual(conn.has_table("dballe_test")).istrue());
}

// Test settings
template<> template<>
void to::test<7>()
{
    conn.drop_table_if_exists("dballe_settings");
    wassert(actual(conn.has_table("dballe_settings")).isfalse());

    wassert(actual(conn.get_setting("test_key")) == "");

    conn.set_setting("test_key", "42");
    wassert(actual(conn.has_table("dballe_settings")).istrue());

    wassert(actual(conn.get_setting("test_key")) == "42");
}

// Test auto_increment
template<> template<>
void to::test<8>()
{
    conn.exec("CREATE TABLE dballe_testai (id INTEGER PRIMARY KEY, val INTEGER)");
    conn.exec("INSERT INTO dballe_testai (val) VALUES (42)");
    wassert(actual(conn.get_last_insert_id()) == 1);
    conn.exec("INSERT INTO dballe_testai (val) VALUES (43)");
    wassert(actual(conn.get_last_insert_id()) == 2);
}

}
