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
#include "levtr.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_levtr_shar
{
};

TESTGRP(memdb_levtr);

template<> template<> void to::test<1>()
{
    LevTrs values;

    // Insert a levtr and check that all data is there
    const LevTr& val = values.obtain(Level(1), Trange::instant());
    wassert(actual(val.level) == Level(1));
    wassert(actual(val.trange) == Trange::instant());

    // Check that lookup returns the same element
    const LevTr& val1 = values.obtain(Level(1), Trange::instant());
    wassert(actual(&val1) == &val);
}

}
