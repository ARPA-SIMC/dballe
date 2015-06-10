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
    Test("coords", [](Fixture& f) {
        wassert(actual(Coords(44.0, 11.0)) == Coords(44.0, 360.0+11.0));
    }),
    Test("latrange", [](Fixture& f) {
        double dmin, dmax;
        LatRange lr;
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == LatRange::IMIN);
        wassert(actual(lr.imax) == LatRange::IMAX);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == LatRange::DMIN);
        wassert(actual(dmax) == LatRange::DMAX);
        wassert(actual(lr.contains(0)).istrue());

        lr = LatRange(40.0, 50.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 40.0);
        wassert(actual(dmax) == 50.0);
        wassert(actual(lr.contains(39.9)).isfalse());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).istrue());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).isfalse());
        wassert(actual(lr.contains(4500000)).istrue());
        wassert(actual(lr.contains(5500000)).isfalse());

        lr.set(-10.0, 10.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 1000000);
        wassert(actual(lr) == LatRange(-10.0, 10.0));

        lr.set(4000000, 5000000);
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        wassert(actual(lr) == LatRange(40.0, 50.0));
    }),
    Test("lonrange", [](Fixture& f) {
        double dmin, dmax;
        LonRange lr;
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == -180.0);
        wassert(actual(dmax) == 180.0);
        wassert(actual(lr.contains(0)).istrue());

        lr = LonRange(-18000000, 18000000);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr = LonRange(-180.0, 180.0);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr.set(-18000000, 18000000);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr.set(-180.0, 180.0);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr = LonRange(40.0, 50.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 40.0);
        wassert(actual(dmax) == 50.0);
        wassert(actual(lr.contains(39.9)).isfalse());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).istrue());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).isfalse());
        wassert(actual(lr.contains(4500000)).istrue());
        wassert(actual(lr.contains(5500000)).isfalse());

        lr = LonRange(50.0, 40.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 5000000);
        wassert(actual(lr.imax) == 4000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 50.0);
        wassert(actual(dmax) == 40.0);
        wassert(actual(lr.contains(39.9)).istrue());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).isfalse());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).istrue());
        wassert(actual(lr.contains(4500000)).isfalse());
        wassert(actual(lr.contains(5500000)).istrue());

        lr.set(-10.0, 10.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 1000000);
        wassert(actual(lr) == LonRange(-10.0, 10.0));

        lr.set(350.0, 360.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 0);
        wassert(actual(lr) == LonRange(-10.0, 0.0));
    }),
};

test_group newtg("dballe_core_defs", tests);

}
