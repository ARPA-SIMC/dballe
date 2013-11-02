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

#include "dballe/core/test-utils-core.h"
#include "stlutils.h"

using namespace dballe;
using namespace dballe::stl;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct core_stlutils_shar
{
};

TESTGRP(core_stlutils);

// Test Intersection
template<> template<> void to::test<1>()
{
    vector<int> a;
    a.push_back(1); a.push_back(2); a.push_back(3);
    vector<int> b;
    b.push_back(2); b.push_back(3);
    vector<int> c;
    c.push_back(0); c.push_back(2);

    Intersection<vector<int>::const_iterator> intersection;
    intersection.add(a.begin(), a.end());
    intersection.add(b.begin(), b.end());
    intersection.add(c.begin(), c.end());

    Intersection<vector<int>::const_iterator>::const_iterator i = intersection.begin();
    wassert(actual(i != intersection.end()).istrue());
    wassert(actual(*i) == 2);
    ++i;
    wassert(actual(i == intersection.end()).istrue());
}

}


