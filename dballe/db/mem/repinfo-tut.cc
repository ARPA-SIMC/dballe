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

#include "db/test-utils-db.h"
#include "db/mem/db.h"
#include "db/mem/repinfo.h"

using namespace dballe;
using namespace dballe::db::mem;
using namespace std;
using namespace wibble::tests;

namespace tut {

struct mem_repinfo_shar
{
};
TESTGRP(mem_repinfo);

/* Test simple queries */
template<> template<>
void to::test<1>()
{
    Repinfo ri;
    ri.load();

    wassert(actual(ri.get_prio("synop")) == 101);
    wassert(actual(ri.get_prio("generic")) == 1000);
}

/* Test update */
template<> template<>
void to::test<2>()
{
    Repinfo ri;
    ri.load();

    wassert(actual(ri.get_prio("synop")) == 101);

    int added, deleted, updated;
    ri.update(NULL, &added, &deleted, &updated);

    ensure_equals(added, 0);
    ensure_equals(deleted, 0);
    ensure_equals(updated, 13);

    wassert(actual(ri.get_prio("synop")) == 101);
}

/* Test update from a file that was known to fail */
template<> template<>
void to::test<3>()
{
    Repinfo ri;
    ri.load();

    wassert(actual(ri.get_prio("synop")) == 101);

    int added, deleted, updated;
    ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

    ensure_equals(added, 3);
    ensure_equals(deleted, 11);
    ensure_equals(updated, 2);

    wassert(actual(ri.get_prio("synop")) == 101);
    wassert(actual(ri.get_prio("FIXspnpo")) == 200);
}

/* Test update from a file with a negative priority */
template<> template<>
void to::test<4>()
{
    Repinfo ri;
    ri.load();

    wassert(actual(ri.get_prio("generic")) == 1000);

    int added, deleted, updated;
    ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

    ensure_equals(added, 3);
    ensure_equals(deleted, 11);
    ensure_equals(updated, 2);

    wassert(actual(ri.get_prio("generic")) == -5);
}

// Test automatic repinfo creation
template<> template<>
void to::test<5>()
{
    Repinfo ri;
    ri.load();

    wassert(actual(ri.get_prio("foobar")) == 1001);
    wassert(actual(ri.get_prio("foobar")) == 1001);
    wassert(actual(ri.get_prio("barbaz")) == 1002);
    wassert(actual(ri.get_prio("barbaz")) == 1002);
    wassert(actual(ri.get_prio("foobar")) == 1001);
}

}

/* vim:set ts=4 sw=4: */

