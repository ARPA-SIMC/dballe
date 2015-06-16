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

#include "dballe/core/tests.h"
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
    unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);

    vector<int> a;
    a.push_back(1); a.push_back(2); a.push_back(3);
    sequences->add(a);

    vector<int> b;
    b.push_back(2); b.push_back(3);
    sequences->add(b);

    vector<int> c;
    c.push_back(0); c.push_back(2);
    sequences->add(c);

    Intersection<int> intersection;

    Intersection<int>::const_iterator i = intersection.begin(sequences);
    wassert(actual(i != intersection.end()).istrue());
    wassert(actual(*i) == 2);
    ++i;
    wassert(actual(i == intersection.end()).istrue());
}

// Test Intersection
template<> template<> void to::test<2>()
{
    unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);

    vector<int> a;
    a.push_back(1);
    sequences->add(a);

    vector<int> b;
    b.push_back(1);
    sequences->add(b);

    Intersection<int> intersection;
    Intersection<int>::const_iterator i = intersection.begin(sequences);
    wassert(actual(i != intersection.end()).istrue());
    wassert(actual(*i) == 1);
    ++i;
    wassert(actual(i == intersection.end()).istrue());
}

// Test SetIntersection
template<> template<> void to::test<3>()
{
    stl::SetIntersection<int> intersection;

    set<int> a;
    a.insert(1); a.insert(2); a.insert(3);
    intersection.add(a);

    set<int> b;
    b.insert(2); b.insert(3);
    intersection.add(b);

    set<int> c;
    c.insert(0); c.insert(2);
    intersection.add(c);

    stl::SetIntersection<int>::const_iterator i = intersection.begin();
    wassert(actual(i != intersection.end()).istrue());
    wassert(actual(*i) == 2);
    ++i;
    wassert(actual(i == intersection.end()).istrue());
}

// Test Union
template<> template<> void to::test<4>()
{
    unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
    vector<int> a;
    sequences->add(a);
    vector<int> b;
    sequences->add(b);

    Union<int> ab;
    Union<int>::const_iterator i = ab.begin(sequences);
    wassert(actual(i == ab.end()).istrue());
}

// Test Union
template<> template<> void to::test<5>()
{
    unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
    vector<int> a;
    a.push_back(1);
    sequences->add(a);
    vector<int> b;
    sequences->add(b);

    Union<int> ab;
    Union<int>::const_iterator i = ab.begin(sequences);
    wassert(actual(i != ab.end()).istrue());
    wassert(actual(*i) == 1);
    ++i;
    wassert(actual(i == ab.end()).istrue());
}

// Test Union
template<> template<> void to::test<6>()
{
    unique_ptr<stl::Sequences<int> > sequences(new stl::Sequences<int>);
    vector<int> a;
    a.push_back(1);
    a.push_back(2);
    sequences->add(a);
    vector<int> b;
    b.push_back(2);
    b.push_back(3);
    sequences->add(b);

    Union<int> ab;
    Union<int>::const_iterator i = ab.begin(sequences);
    wassert(actual(i != ab.end()).istrue());
    wassert(actual(*i) == 1);
    ++i;
    wassert(actual(i != ab.end()).istrue());
    wassert(actual(*i) == 2);
    ++i;
    wassert(actual(i != ab.end()).istrue());
    wassert(actual(*i) == 3);
    ++i;
    wassert(actual(i == ab.end()).istrue());
}

}

#include "stlutils.tcc"
