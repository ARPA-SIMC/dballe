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
#include "value.h"
#include "station.h"
#include "levtr.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_value_shar
{
};

TESTGRP(memdb_value);

template<> template<> void to::test<1>()
{
    Stations stations;
    const Station& stf = stations.obtain(44, 11, "synop");

    LevTrs levtrs;
    const LevTr& levtr = levtrs.obtain(Level(1), Trange::instant());

    Datetime datetime(2013, 10, 30, 23);

    // Insert a station value and check that all data is there
    Values values;
    const Value& v = values.insert_or_replace(stf, levtr, datetime, newvar(WR_VAR(0, 12, 101), 28.5));
    wassert(actual(&v.station) == &stf);
    wassert(actual(&v.levtr) == &levtr);
    wassert(actual(v.datetime) == datetime);
    wassert(actual(v.var->code()) == WR_VAR(0, 12, 101));
    wassert(actual(v.var->enqd()) == 28.5);

    // Replacing a value should reuse an existing one
    const Value& v1 = values.insert_or_replace(stf, levtr, datetime, newvar(WR_VAR(0, 12, 101), 29.5));
    wassert(actual(&v1.station) == &stf);
    wassert(actual(&v1.levtr) == &levtr);
    wassert(actual(v1.datetime) == datetime);
    wassert(actual(v1.var->code()) == WR_VAR(0, 12, 101));
    wassert(actual(v1.var->enqd()) == 29.5);
}

}
