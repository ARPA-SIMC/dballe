/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/odbc/internals.h"
#include "db/v5/db.h"
#include <sql.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace tut {

struct db_internals_shar : public dballe::tests::db_test
{
    db_internals_shar() : dballe::tests::db_test(db::V5)
    {
        if (!has_db()) return;
    }

	~db_internals_shar()
	{
	}

    db::Connection& connection()
    {
        if (db::v5::DB* d = dynamic_cast<db::v5::DB*>(db.get()))
            return *(d->conn);
        else
            throw error_consistency("test DB is not a v5 DB");
    }

	void reset()
	{
		db::Connection& c = connection();
		
		c.drop_table_if_exists("dballe_test");

		db::Statement s(c);
		s.exec_direct("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
	}
};
TESTGRP(db_internals);

// Test querying DBALLE_SQL_C_SINT_TYPE values
template<> template<>
void to::test<1>()
{
	use_db();
	reset();

	db::Connection& c = connection();
	db::Statement s(c);

	s.exec_direct("INSERT INTO dballe_test VALUES (42)");

	s.prepare("SELECT val FROM dballe_test");
	DBALLE_SQL_C_SINT_TYPE val = 0;
	s.bind_out(1, val);
	s.execute();
	unsigned count = 0;
	while (s.fetch())
		++count;
	ensure_equals(val, 42);
	ensure_equals(count, 1);
}

// Test querying DBALLE_SQL_C_SINT_TYPE values, with indicators
template<> template<>
void to::test<2>()
{
	use_db();
	reset();

	db::Connection& c = connection();
	db::Statement s(c);

	s.exec_direct("INSERT INTO dballe_test VALUES (42)");

	s.prepare("SELECT val FROM dballe_test");
	DBALLE_SQL_C_SINT_TYPE val = 0;
	SQLLEN ind = 0;
	s.bind_out(1, val, ind);
	s.execute();
	unsigned count = 0;
	while (s.fetch())
		++count;
	ensure_equals(val, 42);
	ensure(ind != SQL_NULL_DATA);
	ensure_equals(count, 1);
}

// Test querying DBALLE_SQL_C_UINT_TYPE values
template<> template<>
void to::test<3>()
{
	use_db();
	reset();

	db::Connection& c = connection();
	db::Statement s(c);

	s.exec_direct("INSERT INTO dballe_test VALUES (42)");

	s.prepare("SELECT val FROM dballe_test");
	DBALLE_SQL_C_UINT_TYPE val = 0;
	s.bind_out(1, val);
	s.execute();
	unsigned count = 0;
	while (s.fetch())
		++count;
	ensure_equals(val, 42);
	ensure_equals(count, 1);
}

// Test querying DBALLE_SQL_C_UINT_TYPE values, with indicators
template<> template<>
void to::test<4>()
{
	use_db();
	reset();

	db::Connection& c = connection();
	db::Statement s(c);

	s.exec_direct("INSERT INTO dballe_test VALUES (42)");

	s.prepare("SELECT val FROM dballe_test");
	DBALLE_SQL_C_UINT_TYPE val = 0;
	SQLLEN ind = 0;
	s.bind_out(1, val, ind);
	s.execute();
	unsigned count = 0;
	while (s.fetch())
		++count;
	ensure_equals(val, 42);
	ensure(ind != SQL_NULL_DATA);
	ensure_equals(count, 1);
}

// Test querying unsigned short values
template<> template<>
void to::test<5>()
{
	use_db();
	reset();

	db::Connection& c = connection();
	db::Statement s(c);

	s.exec_direct("INSERT INTO dballe_test VALUES (42)");

	s.prepare("SELECT val FROM dballe_test");
	unsigned short val = 0;
	s.bind_out(1, val);
	s.execute();
	unsigned count = 0;
	while (s.fetch())
		++count;
	ensure_equals(val, 42);
	ensure_equals(count, 1);
}

// Test has_tables
template<> template<>
void to::test<6>()
{
    use_db();
    reset();

    db::Connection& c = connection();
    ensure(!c.has_table("this_should_not_exist"));
    ensure(c.has_table("dballe_test"));
}

// Test settings
template<> template<>
void to::test<7>()
{
    use_db();
    db::Connection& c = connection();
    c.drop_table_if_exists("dballe_settings");
    ensure(!c.has_table("dballe_settings"));

    ensure_equals(c.get_setting("test_key"), "");

    c.set_setting("test_key", "42");
    ensure(c.has_table("dballe_settings"));

    ensure_equals(c.get_setting("test_key"), "42");
}

    //void bind_out(int idx, char* val, SQLLEN buflen);
    //void bind_out(int idx, char* val, SQLLEN buflen, SQLLEN& ind);
    //void bind_out(int idx, SQL_TIMESTAMP_STRUCT& val);

}

/* vim:set ts=4 sw=4: */
