/*
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/v5/station.h"
#include <sql.h>

using namespace dballe;
using namespace dballe::db::v5;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct dbv5_station_shar : public dballe::tests::db_test
{
    Station* st;

    dbv5_station_shar() : dballe::tests::db_test(db::V5)
    {
        if (!has_db()) return;
        st = &v5().station();
    }
};
TESTGRP(dbv5_station);

/* Test dba_db_pseudoana_set_ident */
template<> template<>
void to::test<1>()
{
#if 0
    // FIXME: these are now unaccessible internals
	use_db();

	// Set to a valid value
	st->set_ident("ciao");
	ensure_equals(st->ident, string("ciao"));
	ensure_equals(st->ident_ind, 4);

	// Set to NULL
	st->set_ident(NULL);
	ensure_equals(st->ident[0], 0);
	ensure_equals(st->ident_ind, SQL_NULL_DATA);
#endif
}

/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
    use_db();
    bool inserted;

    // Insert a mobile station
    wassert(actual(st->obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
    wassert(actual(inserted).istrue());
    wassert(actual(st->obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
    wassert(actual(inserted).isfalse());

    // Insert a fixed station
    wassert(actual(st->obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
    wassert(actual(inserted).istrue());
    wassert(actual(st->obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
    wassert(actual(inserted).isfalse());

    // Get the ID of the first station
    wassert(actual(st->get_id(4500000, 1100000, "ciao")) == 1);

    // Get the ID of the second station
    wassert(actual(st->get_id(4600000, 1200000)) == 2);

#if 0
    // FIXME: unused functions now unaccessible
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
#endif
}

}

/* vim:set ts=4 sw=4: */
