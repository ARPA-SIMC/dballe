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

struct db_postgresql_internals_shar
{
    PostgreSQLConnection conn;

    db_postgresql_internals_shar()
    {
        // http://www.postgresql.org/docs/9.3/static/libpq-connect.html#LIBPQ-CONNSTRING
        conn.open("postgresql://enrico@/enrico");
    }

    ~db_postgresql_internals_shar()
    {
    }

    void reset()
    {
        conn.drop_table_if_exists("dballe_test");
        conn.exec_no_data("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};
TESTGRP(db_postgresql_internals);

// Test querying int values
template<> template<>
void to::test<1>()
{
    reset();

    conn.exec_no_data("INSERT INTO dballe_test VALUES (1)");
    conn.exec_no_data("INSERT INTO dballe_test VALUES (2)");

    auto s = conn.exec("SELECT val FROM dballe_test");
    wassert(actual(s.rowcount()) == 2);

    int val = 0;
    for (unsigned row = 0; row < 2; ++row)
        val += s.get_int4(row, 0);

    wassert(actual(val) == 3);
}

// Test querying int values, with potential NULLs
template<> template<>
void to::test<2>()
{
    reset();

    conn.drop_table_if_exists("dballe_testnull");
    conn.exec_no_data("CREATE TABLE dballe_testnull (val INTEGER)");
    conn.exec_no_data("INSERT INTO dballe_testnull VALUES (NULL)");
    conn.exec_no_data("INSERT INTO dballe_testnull VALUES (42)");

    auto s = conn.exec("SELECT val FROM dballe_testnull");
    wassert(actual(s.rowcount()) == 2);

    int val = 0;
    unsigned countnulls = 0;
    for (unsigned row = 0; row < 2; ++row)
    {
        if (s.is_null(row, 0))
            ++countnulls;
        else
            val += s.get_int4(row, 0);
    }

    wassert(actual(val) == 42);
    wassert(actual(countnulls) == 1);
}

// Test querying unsigned values
template<> template<>
void to::test<3>()
{
    reset();

    conn.drop_table_if_exists("dballe_testbig");
    conn.exec_no_data("CREATE TABLE dballe_testbig (val BIGINT)");
    conn.exec_no_data("INSERT INTO dballe_testbig VALUES (x'FFFFFFFE'::bigint)");

    auto s = conn.exec("SELECT val FROM dballe_testbig");
    wassert(actual(s.rowcount()) == 1);

    unsigned val = 0;
    for (unsigned row = 0; row < 1; ++row)
        val += s.get_int8(row, 0);
    wassert(actual(val) == 0xFFFFFFFE);
}

// Test querying unsigned short values
template<> template<>
void to::test<5>()
{
    reset();

    conn.drop_table_if_exists("dballe_testshort");
    conn.exec_no_data("CREATE TABLE dballe_testshort (val SMALLINT)");
    conn.exec_no_data("INSERT INTO dballe_testshort VALUES (123)");

    auto s = conn.exec("SELECT val FROM dballe_testshort");
    wassert(actual(s.rowcount()) == 1);

    Varcode val = 0;
    for (unsigned row = 0; row < 1; ++row)
        val += s.get_int2(row, 0);
    wassert(actual(val) == 123);
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
    wassert(actual(conn.get_setting("test_key1")) == "");
}

// Test auto_increment
template<> template<>
void to::test<8>()
{
    conn.drop_table_if_exists("dballe_testai");
    conn.exec_no_data("CREATE TABLE dballe_testai (id SERIAL PRIMARY KEY, val INTEGER)");
    auto r1 = conn.exec_one_row("INSERT INTO dballe_testai (id, val) VALUES (DEFAULT, 42) RETURNING id");
    wassert(actual(r1.get_int4(0, 0)) == 1);
    auto r2 = conn.exec_one_row("INSERT INTO dballe_testai (id, val) VALUES (DEFAULT, 43) RETURNING id");
    wassert(actual(r2.get_int4(0, 0)) == 2);
}

}
