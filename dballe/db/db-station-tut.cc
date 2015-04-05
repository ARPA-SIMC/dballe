/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"
#include "db/test-utils-db.h"
#include "db/v5/db.h"
#include "db/v6/db.h"
#include "db/v5/station.h"
#include <sql.h>

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct db_tests_station : public dballe::tests::db_test
{
    db::v5::Station& station()
    {
        if (db::v5::DB* db5 = dynamic_cast<db::v5::DB*>(db.get()))
            return db5->station();
        if (db::v6::DB* db6 = dynamic_cast<db::v6::DB*>(db.get()))
            return db6->station();
        throw error_consistency("cannot test stations on the current DB");
    }
};

}

namespace tut {

typedef db_tg<db_tests_station> tg;
typedef tg::object to;

// Insert some values and try to read them again
template<> template<>
void to::test<1>()
{
    use_db();
    db::v5::Station& st = station();
    bool inserted;

    // Insert a mobile station
    wassert(actual(st.obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
    wassert(actual(inserted).istrue());
    wassert(actual(st.obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
    wassert(actual(inserted).isfalse());

    // Insert a fixed station
    wassert(actual(st.obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
    wassert(actual(inserted).istrue());
    wassert(actual(st.obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
    wassert(actual(inserted).isfalse());

    // Get the ID of the first station
    wassert(actual(st.get_id(4500000, 1100000, "ciao")) == 1);

    // Get the ID of the second station
    wassert(actual(st.get_id(4600000, 1200000)) == 2);

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

namespace {

#ifdef HAVE_ODBC
tut::tg db_tests_query_v5_tg("db_station_v5", db::V5);
#endif
tut::tg db_tests_query_v6_tg("db_station_v6", db::V6);

}
