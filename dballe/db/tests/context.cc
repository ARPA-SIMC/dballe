/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/context.h"
#include "db/station.h"

using namespace dballe;
using namespace std;

namespace tut {

struct db_context_shar : public dballe::tests::db_test
{
    db::Context* co;

    db_context_shar()
    {
        if (!has_db()) return;
        co = &db->context();

        db::Station& st = db->station();

        // Insert a mobile station
        st.lat = 4500000;
        st.lon = 1100000;
        st.set_ident("ciao");
        int id = st.insert();
        ensure_equals(id, 1);

        // Insert a fixed station
        st.lat = 4600000;
        st.lon = 1200000;
        st.set_ident(NULL);
        id = st.insert();
        ensure_equals(id, 2);
    }
};
TESTGRP(db_context);

/* Insert some values and try to read them again */
template<> template<>
void to::test<1>()
{
	use_db();

	// Insert a context
	co->id_station = 1;
	co->id_report = 1;
	co->date = dballe::tests::mkts(2001, 2, 3, 4, 5, 6);
	co->ltype1 = 1;
	co->l1 = 2;
	co->ltype2 = 0;
	co->l2 = 3;
	co->pind = 4;
	co->p1 = 5;
	co->p2 = 6;
	ensure_equals(co->insert(), 1);

	// Insert another context
	co->id_station = 2;
	co->id_report = 2;
	co->date = dballe::tests::mkts(2002, 3, 4, 5, 6, 7);
	co->ltype1 = 2;
	co->l1 = 3;
	co->ltype2 = 1;
	co->l2 = 4;
	co->pind = 5;
	co->p1 = 6;
	co->p2 = 7;
	ensure_equals(co->insert(), 2);

	// Get the ID of the first context
	co->id = 0;
	co->id_station = 1;
	co->id_report = 1;
	co->date = dballe::tests::mkts(2001, 2, 3, 4, 5, 6);
	co->ltype1 = 1;
	co->l1 = 2;
	co->ltype2 = 0;
	co->l2 = 3;
	co->pind = 4;
	co->p1 = 5;
	co->p2 = 6;
	ensure_equals(co->get_id(), 1);

	// Get the ID of the second context
	co->id = 0;
	co->id_station = 2;
	co->id_report = 2;
	co->date = dballe::tests::mkts(2002, 3, 4, 5, 6, 7);
	co->ltype1 = 2;
	co->l1 = 3;
	co->ltype2 = 1;
	co->l2 = 4;
	co->pind = 5;
	co->p1 = 6;
	co->p2 = 7;
	ensure_equals(co->get_id(), 2);

	// Get info on the first context
	co->get_data(1);
	ensure_equals(co->id_station, 1);
	ensure_equals(co->id_report, 1);
	ensure_equals(co->date, dballe::tests::mkts(2001, 2, 3, 4, 5, 6));
	//ensure_equals(co->date_ind, 19);
	ensure_equals(co->ltype1, 1);
	ensure_equals(co->l1, 2);
	ensure_equals(co->ltype2, 0);
	ensure_equals(co->l2, 3);
	ensure_equals(co->pind, 4);
	ensure_equals(co->p1, 5);
	ensure_equals(co->p2, 6);

	// Get info on the second context
	co->get_data(2);
	ensure_equals(co->id_station, 2);
	ensure_equals(co->id_report, 2);
	ensure_equals(co->date, dballe::tests::mkts(2002, 3, 4, 5, 6, 7));
	//ensure_equals(co->date_ind, 19);
	ensure_equals(co->ltype1, 2);
	ensure_equals(co->l1, 3);
	ensure_equals(co->ltype2, 1);
	ensure_equals(co->l2, 4);
	ensure_equals(co->pind, 5);
	ensure_equals(co->p1, 6);
	ensure_equals(co->p2, 7);

#if 0
	// Update the second context
	co->id = 2;
	co->id_station = 2;
	co->id_report = 2;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2003, 4, 5, 6, 7, 8);
	co->ltype = 3;
	co->l1 = 4;
	co->l2 = 5;
	co->pind = 6;
	co->p1 = 7;
	co->p2 = 8;
	CHECKED(dba_db_context_update(co));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_context_get_data(pa, 1));
	ensure_equals(pa->lat, 4500000);
	ensure_equals(pa->lon, 1100000);
	ensure_equals(pa->ident, string("ciao"));
	ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_context_get_data(pa, 2));
	ensure_equals(pa->lat, 4700000);
	ensure_equals(pa->lon, 1300000);
	ensure_equals(pa->ident[0], 0);
#endif
	// Get a station metadata context entry for the first station
	co->id_station = 1;
	co->id_report = -1;
	ensure_equals(co->obtain_station_info(), 3);

	// Get a pseudoana context entry for the second station
	co->id_station = 2;
	co->id_report = -1;
	ensure_equals(co->obtain_station_info(), 4);

	// Get a pseudoana context entry for the first station again, to see that only one is created
	co->id_station = 1;
	co->id_report = -1;
	ensure_equals(co->obtain_station_info(), 3);

	// Get a pseudoana context entry for the second station again, to see that only one is created
	co->id_station = 2;
	co->id_report = -1;
	ensure_equals(co->obtain_station_info(), 4);
}

}

/* vim:set ts=4 sw=4: */
