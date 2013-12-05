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
#include "memdb.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct memdb_shar
{
};

TESTGRP(memdb);

static inline Record query_exact(const Datetime& dt)
{
    Record query;
    query.set(dt);
    return query;
}
static inline Record query_min(const Datetime& dt)
{
    Record query;
    query.setmin(dt);
    return query;
}
static inline Record query_max(const Datetime& dt)
{
    Record query;
    query.setmax(dt);
    return query;
}
static inline Record query_minmax(const Datetime& min, const Datetime& max)
{
    Record query;
    query.setmin(min);
    query.setmax(max);
    return query;
}

template<> template<> void to::test<1>()
{
    // Check datetime queries, with data that only differs by hour of the day
    Memdb memdb;

    // Insert one value
    const Station& stf = *memdb.stations[memdb.stations.obtain_fixed(Coord(44.0, 11.0), "synop")];
    const LevTr& levtr = *memdb.levtrs[memdb.levtrs.obtain(Level(1), Trange::instant())];
    const Value& v1 = *memdb.values[memdb.values.insert(stf, levtr, Datetime(2013, 10, 30, 23), newvar(WR_VAR(0, 12, 101), 28.5))];
    const Value& v2 = *memdb.values[memdb.values.insert(stf, levtr, Datetime(2013, 10, 30, 24), newvar(WR_VAR(0, 12, 101), 28.5))];

    // Exact match
    {
        vector<const Value*> items = get_data_results(memdb, query_exact(Datetime(2013, 10, 30, 22)));
        wassert(actual(items.size()) == 0);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_exact(Datetime(2013, 10, 30, 23)));
        wassert(actual(items.size()) == 1);
    }

    // Datemin match
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 30, 22)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 30, 23)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 30, 24)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 30, 25)));
        wassert(actual(items.size()) == 0);
    }

    // Datemax match
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 30, 25)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 30, 24)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 30, 23)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 30, 22)));
        wassert(actual(items.size()) == 0);
    }

    // Date min-max match
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 22),
                    Datetime(2013, 10, 30, 25)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 23),
                    Datetime(2013, 10, 30, 24)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 22),
                    Datetime(2013, 10, 30, 23)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 24),
                    Datetime(2013, 10, 30, 26)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 21),
                    Datetime(2013, 10, 30, 22)));
        wassert(actual(items.size()) == 0);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 30, 25),
                    Datetime(2013, 10, 30, 26)));
        wassert(actual(items.size()) == 0);
    }
}

template<> template<> void to::test<2>()
{
    // Check datetime queries, with data that only differs by day
    Memdb memdb;

    // Insert one value
    const Station& stf = *memdb.stations[memdb.stations.obtain_fixed(Coord(44.0, 11.0), "synop")];
    const LevTr& levtr = *memdb.levtrs[memdb.levtrs.obtain(Level(1), Trange::instant())];
    const Value& v1 = *memdb.values[memdb.values.insert(stf, levtr, Datetime(2013, 10, 23), newvar(WR_VAR(0, 12, 101), 28.5))];
    const Value& v2 = *memdb.values[memdb.values.insert(stf, levtr, Datetime(2013, 10, 24), newvar(WR_VAR(0, 12, 101), 28.5))];

    // Exact match
    {
        vector<const Value*> items = get_data_results(memdb, query_exact(Datetime(2013, 10, 22)));
        wassert(actual(items.size()) == 0);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_exact(Datetime(2013, 10, 23)));
        wassert(actual(items.size()) == 1);
    }

    // Datemin match
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 22)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 23)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 24)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_min(Datetime(2013, 10, 25)));
        wassert(actual(items.size()) == 0);
    }

    // Datemax match
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 25)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 24)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 23)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_max(Datetime(2013, 10, 22)));
        wassert(actual(items.size()) == 0);
    }

    // Date min-max match
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 22),
                    Datetime(2013, 10, 25)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 23),
                    Datetime(2013, 10, 24)));
        wassert(actual(items.size()) == 2);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 23),
                    Datetime(2013, 10, 23)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 24),
                    Datetime(2013, 10, 24)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 22),
                    Datetime(2013, 10, 23)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 24),
                    Datetime(2013, 10, 26)));
        wassert(actual(items.size()) == 1);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 21),
                    Datetime(2013, 10, 22)));
        wassert(actual(items.size()) == 0);
    }
    {
        vector<const Value*> items = get_data_results(memdb, query_minmax(
                    Datetime(2013, 10, 25),
                    Datetime(2013, 10, 26)));
        wassert(actual(items.size()) == 0);
    }
}

}

#include "query.tcc"
