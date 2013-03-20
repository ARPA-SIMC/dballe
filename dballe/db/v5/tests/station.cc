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
#include "db/v5/station.h"
#include <sql.h>

using namespace dballe;
using namespace std;

namespace tut {

struct station_shar : public dballe::tests::db_test
{
        db::Station* st;

	station_shar()
	{
		if (!has_db()) return;
		st = &db->station();
	}
};
TESTGRP(station);

/* Test dba_db_pseudoana_set_ident */
template<> template<>
void to::test<1>()
{
	use_db();

	// Set to a valid value
	st->set_ident("ciao");
	ensure_equals(st->ident, string("ciao"));
	ensure_equals(st->ident_ind, 4);

	// Set to NULL
	st->set_ident(NULL);
	ensure_equals(st->ident[0], 0);
	ensure_equals(st->ident_ind, SQL_NULL_DATA);
}

/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
	use_db();

	// Insert a mobile station
	st->lat = 4500000;
	st->lon = 1100000;
        st->set_ident("ciao");
        ensure_equals(st->insert(), 1);

	// Insert a fixed station
	st->lat = 4600000;
	st->lon = 1200000;
	st->set_ident(NULL);
	ensure_equals(st->insert(), 2);

	// Get the ID of the first station
	st->id = 0;
	st->lat = 4500000;
	st->lon = 1100000;
	st->set_ident("ciao");
	ensure_equals(st->get_id(), 1);

	// Get the ID of the second station
	st->id = 0;
	st->lat = 4600000;
	st->lon = 1200000;
	st->set_ident(NULL);
	ensure_equals(st->get_id(), 2);

	// Get info on the first station
	st->get_data(1);
	ensure_equals(st->lat, 4500000);
	ensure_equals(st->lon, 1100000);
	ensure_equals(st->ident, string("ciao"));
	ensure_equals(st->ident_ind, 4);

	// Get info on the second station
	st->get_data(2);
	ensure_equals(st->lat, 4600000);
	ensure_equals(st->lon, 1200000);
	ensure_equals(st->ident[0], 0);

	// Update the second station
	st->id = 2;
	st->lat = 4700000;
	st->lon = 1300000;
	st->update();

	// Get info on the first station: it should be unchanged
	st->get_data(1);
	ensure_equals(st->lat, 4500000);
	ensure_equals(st->lon, 1100000);
	ensure_equals(st->ident, string("ciao"));
	ensure_equals(st->ident_ind, 4);

	// Get info on the second station: it should be updated
	st->get_data(2);
	ensure_equals(st->lat, 4700000);
	ensure_equals(st->lon, 1300000);
	ensure_equals(st->ident[0], 0);
}

}

/* vim:set ts=4 sw=4: */
