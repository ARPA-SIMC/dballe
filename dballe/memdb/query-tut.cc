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
#include "query.h"
#include "station.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_query_shar
{
    Stations stations;
    size_t pos[10];

    memdb_query_shar()
    {
        // 10 stations in a line from latitude 40.0 to 50.0
        for (unsigned i = 0; i < 10; ++i)
            pos[i] = stations.obtain_fixed(Coord(40.0 + i, 11.0), "synop");
    }
};

TESTGRP(memdb_query);

template<> template<> void to::test<1>()
{
    // Test not performing any selection: all should be selected
    Results<Station> res(stations);

    wassert(actual(res.is_select_all()).istrue());
    wassert(actual(res.is_empty()).isfalse());

    vector<const Station*> items;
    res.copy_valptrs_to(back_inserter(items));
    wassert(actual(items.size()) == 0);
}

template<> template<> void to::test<2>()
{
    // Test setting to no results
    Results<Station> res(stations);
    res.set_to_empty();

    wassert(actual(res.is_select_all()).isfalse());
    wassert(actual(res.is_empty()).istrue());

    vector<const Station*> items;
    res.copy_valptrs_to(back_inserter(items));
    wassert(actual(items.size()) == 0);
}

template<> template<> void to::test<3>()
{
    // Test selecting a singleton
    Results<Station> res(stations);
    res.add(pos[0]);

    wassert(actual(res.is_select_all()).isfalse());
    wassert(actual(res.is_empty()).isfalse());

    vector<const Station*> items;
    res.copy_valptrs_to(back_inserter(items));
    wassert(actual(items.size()) == 1);
    wassert(actual(items[0]->id) == pos[0]);
}

}

#include "query.tcc"
