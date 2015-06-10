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

#include "core/test-utils-core.h"
#include "core/defs.h"

using namespace dballe;
using namespace wibble::tests;
using namespace std;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("datetimerange", [](Fixture& f) {
        Datetime missing;
        Datetime dt_2010(2010, 1, 1, 0, 0, 0);
        Datetime dt_2011(2011, 1, 1, 0, 0, 0);
        Datetime dt_2012(2012, 1, 1, 0, 0, 0);
        Datetime dt_2013(2013, 1, 1, 0, 0, 0);

        // Test equality
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(missing, missing)).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2011) == DatetimeRange(dt_2010, dt_2011)).istrue());
        wassert(actual(DatetimeRange(dt_2010, missing) == DatetimeRange(missing, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, dt_2010) == DatetimeRange(missing, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(dt_2010, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(missing, dt_2010)).isfalse());
        wassert(actual(DatetimeRange(dt_2010, dt_2011) == DatetimeRange(dt_2012, dt_2013)).isfalse());

        // Test contains
        wassert(actual(DatetimeRange(missing, missing).contains(DatetimeRange(missing, missing))).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2011).contains(DatetimeRange(dt_2010, dt_2011))).istrue());
        wassert(actual(DatetimeRange(missing, missing).contains(DatetimeRange(dt_2011, dt_2012))).istrue());
        wassert(actual(DatetimeRange(dt_2011, dt_2012).contains(DatetimeRange(missing, missing))).isfalse());
        wassert(actual(DatetimeRange(dt_2010, dt_2013).contains(DatetimeRange(dt_2011, dt_2012))).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2012).contains(DatetimeRange(dt_2011, dt_2013))).isfalse());
        wassert(actual(DatetimeRange(missing, dt_2010).contains(DatetimeRange(dt_2011, missing))).isfalse());
    }),
    Test("coords", [](Fixture& f) {
        wassert(actual(Coords(44.0, 11.0)) == Coords(44.0, 360.0+11.0));
    }),
};

test_group newtg("dballe_core_defs", tests);

}
