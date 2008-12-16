/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-db.h>
#include <dballe/db/querybuf.h>
#include <dballe/db/db.h>
#include <dballe/db/internals.h>
#include <dballe/db/repinfo.h>


namespace tut {
using namespace tut_dballe;

struct repinfo_shar : public db_test
{
	TestMsgEnv testenv;

	dba_db_repinfo ri;

	repinfo_shar()
	{
		if (!has_db()) return;

		CHECKED(dba_db_need_repinfo(db));
		ri = db->repinfo;
	}
};
TESTGRP(repinfo);

/* Test simple queries */
template<> template<>
void to::test<1>()
{
	use_db();

	int id, i;

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);

	CHECKED(dba_db_repinfo_get_id(ri, "generic", &id));
	gen_ensure_equals(id, 255);

	CHECKED(dba_db_repinfo_has_id(ri, 1, &i));
	gen_ensure_equals((bool)i, true);

	CHECKED(dba_db_repinfo_has_id(ri, 199, &i));
	gen_ensure_equals((bool)i, false);
}

/* Test update */
template<> template<>
void to::test<2>()
{
	use_db();

	int id, added, deleted, updated;

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);

	CHECKED(dba_db_repinfo_update(ri, NULL, &added, &deleted, &updated));

	gen_ensure_equals(added, 0);
	gen_ensure_equals(deleted, 0);
	gen_ensure_equals(updated, 15);

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);
}

/* Test update from a file that was known to fail */
template<> template<>
void to::test<3>()
{
	use_db();

	int id, added, deleted, updated;

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);

	CHECKED(dba_db_repinfo_update(ri, (string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated));

	gen_ensure_equals(added, 2);
	gen_ensure_equals(deleted, 12);
	gen_ensure_equals(updated, 3);

	CHECKED(dba_db_repinfo_get_id(ri, "synop", &id));
	gen_ensure_equals(id, 1);
	CHECKED(dba_db_repinfo_get_id(ri, "FIXspnpo", &id));
	gen_ensure_equals(id, 200);
}

}

/* vim:set ts=4 sw=4: */
