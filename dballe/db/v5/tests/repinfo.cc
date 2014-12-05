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

    wassert(actual(ri->get_id("synop")) == 1);
    wassert(actual(ri->get_id("generic")) == 255);
    wassert(actual(ri->get_rep_memo(1)) == "synop");
    wassert(actual(ri->get_priority(199)) == INT_MAX);
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

	ensure_equals(added, 3);
	ensure_equals(deleted, 11);
	ensure_equals(updated, 2);

	ensure_equals(ri->get_id("synop"), 1);
	ensure_equals(ri->get_id("FIXspnpo"), 201);
}

/* Test update from a file with a negative priority */
template<> template<>
void to::test<4>()
{
	use_db();

    int id = ri->get_id("generic");
    wassert(actual(ri->get_priority(id)) == 1000);

	int added, deleted, updated;
	ri->update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

	ensure_equals(added, 3);
	ensure_equals(deleted, 11);
	ensure_equals(updated, 2);

    wassert(actual(ri->get_priority(id)) == -5);
}

// Test automatic repinfo creation
template<> template<>
void to::test<5>()
{
    use_db();

    int id = ri->obtain_id("foobar");
    wassert(actual(id) > 0);
    wassert(actual(ri->get_rep_memo(id)) == "foobar");
    wassert(actual(ri->get_priority(id)) == 1001);

    id = ri->obtain_id("barbaz");
    wassert(actual(id) > 0);
    wassert(actual(ri->get_rep_memo(id)) == "barbaz");
    wassert(actual(ri->get_priority(id)) == 1002);
}

}
