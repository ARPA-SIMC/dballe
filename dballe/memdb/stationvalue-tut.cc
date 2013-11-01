/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/core/var.h>
#include "memdb/tests.h"
#include "stationvalue.h"
#include "station.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_stationvalue_shar
{
};

TESTGRP(memdb_stationvalue);

template<> template<> void to::test<1>()
{
    Stations stations;
    const Station& stf = stations.obtain(Coord(44.0, 11.0), "synop");

    // Insert a station value and check that all data is there
    StationValues svalues;
    const StationValue& sv = svalues.insert_or_replace(stf, newvar(WR_VAR(0, 12, 101), 28.5));
    wassert(actual(&sv.station) == &stf);
    wassert(actual(sv.var->code()) == WR_VAR(0, 12, 101));
    wassert(actual(sv.var->enqd()) == 28.5);

    // Replacing a value should reuse an existing one
    const StationValue& sv1 = svalues.insert_or_replace(stf, newvar(WR_VAR(0, 12, 101), 29.5));
    wassert(actual(&sv1) == &sv);
    wassert(actual(&sv1.station) == &stf);
    wassert(actual(sv1.var->code()) == WR_VAR(0, 12, 101));
    wassert(actual(sv1.var->enqd()) == 29.5);
}

}

