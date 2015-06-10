/*
 * Copyright (C) 2013--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/core/query.h"
#include "station.h"
#include "results.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_station_shar
{
};

TESTGRP(memdb_station);

template<> template<>
void to::test<1>()
{
    Stations stations;

    // Insert a fixed station and check that all data is there
    const Station& stf = *stations[stations.obtain_fixed(Coords(44.0, 11.0), "synop")];
    wassert(actual(stf.coords.dlat()) == 44.0);
    wassert(actual(stf.coords.dlon()) == 11.0);
    wassert(actual(stf.ident) == "");
    wassert(actual(stf.report) == "synop");

    // Insert a mobile station and check that all data is there
    const Station& stm = *stations[stations.obtain_mobile(Coords(44.0, 11.0), "LH1234", "airep")];
    wassert(actual(stm.coords.dlat()) == 44.0);
    wassert(actual(stm.coords.dlon()) == 11.0);
    wassert(actual(stm.ident) == "LH1234");
    wassert(actual(stm.report) == "airep");

    // Check that lookup returns the same element
    const Station& stf1 = *stations[stations.obtain_fixed(Coords(44.0, 11.0), "synop")];
    wassert(actual(&stf1) == &stf);
    const Station& stm1 = *stations[stations.obtain_mobile(Coords(44.0, 11.0), "LH1234", "airep")];
    wassert(actual(&stm1) == &stm);

    // Check again, looking up records
    core::Record sfrec;
    sfrec.set("lat", 44.0);
    sfrec.set("lon", 11.0);
    sfrec.set("rep_memo", "synop");
    const Station& stf2 = *stations[stations.obtain(sfrec)];
    wassert(actual(&stf2) == &stf);

    core::Record smrec;
    smrec.set("lat", 44.0);
    smrec.set("lon", 11.0);
    smrec.set("ident", "LH1234");
    smrec.set("rep_memo", "airep");
    const Station& stm2 = *stations[stations.obtain(smrec)];
    wassert(actual(&stm2) == &stm);
}

template<> template<>
void to::test<2>()
{
    // Query by ana_id
    Stations stations;
    size_t pos = stations.obtain_fixed(Coords(44.0, 11.0), "synop");

    core::Query query;

    {
        query.ana_id = pos;
        Results<Station> res(stations);
        stations.query(query, res);
        vector<const Station*> items = get_results(res);
        wassert(actual(items.size()) == 1);
        wassert(actual(items[0]->id) == pos);
    }

    {
        query.ana_id = 100;
        Results<Station> res(stations);
        stations.query(query, res);
        wassert(actual(res.is_select_all()).isfalse());
        wassert(actual(res.is_empty()).istrue());
    }

    size_t pos1 = stations.obtain_fixed(Coords(45.0, 12.0), "synop");

    {
        query.ana_id = pos;
        Results<Station> res(stations);
        stations.query(query, res);
        vector<const Station*> items = get_results(res);
        wassert(actual(items.size()) == 1);
        wassert(actual(items[0]->id) == pos);
    }
}

template<> template<>
void to::test<3>()
{
    // Query by lat,lon
    Stations stations;
    size_t pos = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
    stations.obtain_fixed(Coords(45.0, 12.0), "synop");

    Results<Station> res(stations);
    stations.query(core_query_from_string("lat=44.0, lon=11.0"), res);

    vector<const Station*> items = get_results(res);
    wassert(actual(items.size()) == 1);
    wassert(actual(items[0]->id) == pos);
}

template<> template<>
void to::test<4>()
{
    // Query everything
    Stations stations;
    size_t pos1 = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
    size_t pos2 = stations.obtain_fixed(Coords(45.0, 12.0), "synop");

    Results<Station> res(stations);
    stations.query(core::Query(), res);

    wassert(actual(res.is_select_all()).istrue());
    wassert(actual(res.is_empty()).isfalse());
}

template<> template<>
void to::test<5>()
{
    // Query latitudes matching multiple index entries
    Stations stations;
    size_t pos1 = stations.obtain_fixed(Coords(44.0, 11.0), "synop");
    size_t pos2 = stations.obtain_fixed(Coords(45.0, 11.0), "synop");
    size_t pos3 = stations.obtain_fixed(Coords(46.0, 11.0), "synop");

    Results<Station> res(stations);
    stations.query(core_query_from_string("latmin=45.0"), res);

    vector<const Station*> items = get_results(res);
    wassert(actual(items.size()) == 2);
    wassert(actual(items[0]->id) == pos2);
    wassert(actual(items[1]->id) == pos3);
}

}

#include "results.tcc"
