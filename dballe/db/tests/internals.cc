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
#include "db/internals.h"

using namespace dballe;
using namespace wreport;
using namespace std;

namespace tut {

struct db_internals_shar : public dballe::tests::db_test
{
	db_internals_shar()
		: dballe::tests::db_test(false)
	{
		if (!has_db()) return;
	}

	~db_internals_shar()
	{
	}
};
TESTGRP(db_internals);

// Ensure that reset will work on an empty database
template<> template<>
void to::test<1>()
{
	use_db();

	db::Connection& c = *(db->conn);
	
	c.drop_table_if_exists("dballe_test");

	db::Statement s(c);
	s.exec_direct("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
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

}

/* vim:set ts=4 sw=4: */
