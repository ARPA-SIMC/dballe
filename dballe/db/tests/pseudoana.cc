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
#include <dballe/db/db.h>
#include <dballe/db/pseudoana.h>

namespace tut {
using namespace tut_dballe;

struct pseudoana_shar : public db_test
{
	TestMsgEnv testenv;

	dba_db_pseudoana pa;

	pseudoana_shar()
	{
		if (!has_db()) return;

		CHECKED(dba_db_need_pseudoana(db));
		pa = db->pseudoana;
		gen_ensure(pa != NULL);
	}
};
TESTGRP(pseudoana);

/* Test dba_db_pseudoana_set_ident */
template<> template<>
void to::test<1>()
{
	use_db();

	// Set to a valid value
	dba_db_pseudoana_set_ident(pa, "ciao");
	gen_ensure_equals(pa->ident, string("ciao"));
	gen_ensure_equals(pa->ident_ind, 4);

	// Set to NULL
	dba_db_pseudoana_set_ident(pa, NULL);
	gen_ensure_equals(pa->ident[0], 0);
}

/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
	use_db();

	// Insert a fixed station
	int id;
	pa->lat = 4500000;
	pa->lon = 1100000;
	dba_db_pseudoana_set_ident(pa, "ciao");
	CHECKED(dba_db_pseudoana_insert(pa, &id));
	gen_ensure_equals(id, 1);

	// Insert a mobile station
	pa->lat = 4600000;
	pa->lon = 1200000;
	dba_db_pseudoana_set_ident(pa, NULL);
	CHECKED(dba_db_pseudoana_insert(pa, &id));
	gen_ensure_equals(id, 2);

	// Get the ID of the first station
	pa->id = 0;
	pa->lat = 4500000;
	pa->lon = 1100000;
	dba_db_pseudoana_set_ident(pa, "ciao");
	CHECKED(dba_db_pseudoana_get_id(pa, &id));
	gen_ensure_equals(id, 1);

	// Get the ID of the second station
	pa->id = 0;
	pa->lat = 4600000;
	pa->lon = 1200000;
	dba_db_pseudoana_set_ident(pa, NULL);
	CHECKED(dba_db_pseudoana_get_id(pa, &id));
	gen_ensure_equals(id, 2);

	// Get info on the first station
	CHECKED(dba_db_pseudoana_get_data(pa, 1));
	gen_ensure_equals(pa->lat, 4500000);
	gen_ensure_equals(pa->lon, 1100000);
	gen_ensure_equals(pa->ident, string("ciao"));
	gen_ensure_equals(pa->ident_ind, 4);

	// Get info on the second station
	CHECKED(dba_db_pseudoana_get_data(pa, 2));
	gen_ensure_equals(pa->lat, 4600000);
	gen_ensure_equals(pa->lon, 1200000);
	gen_ensure_equals(pa->ident[0], 0);

	// Update the second station
	pa->id = 2;
	pa->lat = 4700000;
	pa->lon = 1300000;
	CHECKED(dba_db_pseudoana_update(pa));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_pseudoana_get_data(pa, 1));
	gen_ensure_equals(pa->lat, 4500000);
	gen_ensure_equals(pa->lon, 1100000);
	gen_ensure_equals(pa->ident, string("ciao"));
	gen_ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_pseudoana_get_data(pa, 2));
	gen_ensure_equals(pa->lat, 4700000);
	gen_ensure_equals(pa->lon, 1300000);
	gen_ensure_equals(pa->ident[0], 0);
}

}

/* vim:set ts=4 sw=4: */
