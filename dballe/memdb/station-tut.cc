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

#include "memdb/tests.h"
#include "station.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_station_shar
{
};

TESTGRP(memdb_station);

template<> template<> void to::test<1>()
{
    Stations stations;

    // Insert a fixed station and check that all data is there
    const Station& stf = stations.get_station(44, 11, "synop");
    wassert(actual(stf.coords.lat) == 44.0);
    wassert(actual(stf.coords.lon) == 11.0);
    wassert(actual(stf.ident) == "");
    wassert(actual(stf.report) == "synop");

    // Insert a mobile station and check that all data is there
    const Station& stm = stations.get_station(44, 11, "LH1234", "airep");
    wassert(actual(stm.coords.lat) == 44.0);
    wassert(actual(stm.coords.lon) == 11.0);
    wassert(actual(stm.ident) == "LH1234");
    wassert(actual(stm.report) == "airep");

    // Check that lookup returns the same element
    const Station& stf1 = stations.get_station(44, 11, "synop");
    wassert(actual(&stf1) == &stf);
    const Station& stm1 = stations.get_station(44, 11, "LH1234", "airep");
    wassert(actual(&stm1) == &stm);

}

}
