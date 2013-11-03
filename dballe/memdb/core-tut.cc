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
#include "core.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_core_shar
{
};

TESTGRP(memdb_core);

// Test Positions
template<> template<> void to::test<1>()
{
#if 0
    Positions pa;
    pa.insert(1);
    pa.insert(2);
    pa.insert(3);

    Positions pb;
    pb.insert(1);
    pb.insert(3);
    pb.insert(4);

    pa.inplace_intersect(pb);
    wassert(actual(pa.size()) == 2u);

    wassert(actual(pa.contains(1)).istrue());
    wassert(actual(pa.contains(3)).istrue());
#endif
}

}

