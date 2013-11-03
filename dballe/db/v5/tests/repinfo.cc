/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/v5/db.h"
#include "db/v5/repinfo.h"

using namespace dballe;
using namespace dballe::db::v5;
using namespace std;
using namespace wibble::tests;

namespace tut {

struct dbv5_repinfo_shar : public dballe::tests::db_test
{
    Repinfo* ri;

    dbv5_repinfo_shar() : dballe::tests::db_test(db::V5)
	{
		if (!has_db()) return;
		ri = &v5().repinfo();
	}
};
TESTGRP(dbv5_repinfo);

/* Test simple queries */
template<> template<>
void to::test<1>()
{
	use_db();

	ensure_equals(ri->get_id("synop"), 1);
	ensure_equals(ri->get_id("generic"), 255);
	ensure_equals(ri->has_id(1), true);
	ensure_equals(ri->has_id(199), false);
}

/* Test update */
template<> template<>
void to::test<2>()
{
	use_db();

	ensure_equals(ri->get_id("synop"), 1);

	int added, deleted, updated;
	ri->update(NULL, &added, &deleted, &updated);

	ensure_equals(added, 0);
	ensure_equals(deleted, 0);
	ensure_equals(updated, 13);

	ensure_equals(ri->get_id("synop"), 1);
}

/* Test update from a file that was known to fail */
template<> template<>
void to::test<3>()
{
	use_db();

	ensure_equals(ri->get_id("synop"), 1);

	int added, deleted, updated;
	ri->update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

	ensure_equals(added, 2);
	ensure_equals(deleted, 10);
	ensure_equals(updated, 3);

	ensure_equals(ri->get_id("synop"), 1);
	ensure_equals(ri->get_id("FIXspnpo"), 200);
}

/* Test update from a file with a negative priority */
template<> template<>
void to::test<4>()
{
	use_db();

	ensure_equals(ri->get_by_memo("generic")->prio, 1000);

	int added, deleted, updated;
	ri->update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

	ensure_equals(added, 2);
	ensure_equals(deleted, 10);
	ensure_equals(updated, 3);

	ensure_equals(ri->get_by_memo("generic")->prio, -5);
}

// Test automatic repinfo creation
template<> template<>
void to::test<5>()
{
    use_db();

    wassert(actual(ri->obtain_id("foobar")) > 0);
    const repinfo::Cache *c = ri->get_by_memo("foobar");
    wassert(actual(c) != (const repinfo::Cache *)NULL);
    wassert(actual(c->id) == 256);
    wassert(actual(c->memo) == "foobar");
    wassert(actual(c->desc) == "foobar");
    wassert(actual(c->prio) == 1001);

    wassert(actual(ri->obtain_id("barbaz")) > 0);
    c = ri->get_by_memo("barbaz");
    wassert(actual(c) != (const repinfo::Cache *)NULL);
    wassert(actual(c->id) == 257);
    wassert(actual(c->memo) == "barbaz");
    wassert(actual(c->desc) == "barbaz");
    wassert(actual(c->prio) == 1002);
}

}

/* vim:set ts=4 sw=4: */
