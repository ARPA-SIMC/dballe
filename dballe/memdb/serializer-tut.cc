/*
 * Copyright (C) 2013--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "serializer.h"
#include "core/defs.h"
#include "core/var.h"
#include <wibble/sys/fs.h>

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace wibble::tests;
using namespace wibble;
using namespace wreport;
using namespace std;

namespace tut {

struct memdb_serialize_shar
{
    string testdir;

    memdb_serialize_shar() : testdir("serializer_test_dir") {}

    void reset_test_dir()
    {
        if (sys::fs::isdir(testdir))
            sys::fs::rmtree(testdir);
    }
};

TESTGRP(memdb_serialize);

// Test a simple serialize/deserialize round
template<> template<> void to::test<1>()
{
    reset_test_dir();

    Var var_st_1(varinfo(WR_VAR(0, 7, 7)), 100.0);
    var_st_1.seta(newvar(WR_VAR(0, 33, 7), 30));
    Var var_st_2(varinfo(WR_VAR(0, 7, 7)), 5000.0);
    var_st_2.seta(newvar(WR_VAR(0, 33, 7), 40));
    Var var_1(varinfo(WR_VAR(0, 12, 101)), 274.0);
    var_1.seta(newvar(WR_VAR(0, 33, 7), 50));
    Var var_2(varinfo(WR_VAR(0, 12, 101)), 273.0);
    var_2.seta(newvar(WR_VAR(0, 33, 7), 60));

    // Create a memdb and write it out
    {
        Memdb memdb;

        memdb.insert(Coord(45, 11), string(), "synop", Level(1), Trange::instant(), Datetime(2013, 12, 15), var_1);
        memdb.stationvalues.insert(*memdb.stations[0], var_st_1);
        memdb.insert(Coord(45, 12), "LH1234", "airep", Level(1), Trange::instant(), Datetime(2013, 12, 16), var_2);
        memdb.stationvalues.insert(*memdb.stations[1], var_st_2);

        serialize::CSVWriter serializer(testdir);
        serializer.write(memdb);
        serializer.commit();
    }

    // Read it back
    {
        Memdb memdb;
        serialize::CSVReader reader(testdir, memdb);
        reader.read();
        wassert(actual(memdb.stations.element_count()) == 2);
        wassert(actual(memdb.stations[0]->coords) == Coord(45, 11));
        wassert(actual(memdb.stations[0]->mobile).isfalse());
        wassert(actual(memdb.stations[0]->ident) == "");
        wassert(actual(memdb.stations[0]->report) == "synop");
        wassert(actual(memdb.stations[1]->coords) == Coord(45, 12));
        wassert(actual(memdb.stations[1]->mobile).istrue());
        wassert(actual(memdb.stations[1]->ident) == "LH1234");
        wassert(actual(memdb.stations[1]->report) == "airep");

        wassert(actual(memdb.stationvalues.element_count()) == 2);
        wassert(actual(memdb.stationvalues[0]->station.id) == memdb.stations[0]->id);
        wassert(actual(*(memdb.stationvalues[0]->var)) == var_st_1);
        wassert(actual(memdb.stationvalues[1]->station.id) == memdb.stations[1]->id);
        wassert(actual(*(memdb.stationvalues[1]->var)) == var_st_2);

        wassert(actual(memdb.levtrs.element_count()) == 1);
        wassert(actual(memdb.levtrs[0]->level) == Level(1));
        wassert(actual(memdb.levtrs[0]->trange) == Trange::instant());

        wassert(actual(memdb.values.element_count()) == 2);
        wassert(actual(memdb.values[0]->station.id) == memdb.stations[0]->id);
        wassert(actual(memdb.values[0]->levtr.level) == Level(1));
        wassert(actual(memdb.values[0]->levtr.trange) == Trange::instant());
        wassert(actual(memdb.values[0]->datetime) == Datetime(2013, 12, 15));
        wassert(actual(*(memdb.values[0]->var)) == var_1);
        wassert(actual(memdb.values[1]->station.id) == memdb.stations[1]->id);
        wassert(actual(memdb.values[1]->levtr.level) == Level(1));
        wassert(actual(memdb.values[1]->levtr.trange) == Trange::instant());
        wassert(actual(memdb.values[1]->datetime) == Datetime(2013, 12, 16));
        wassert(actual(*(memdb.values[1]->var)) == var_2);
    }
}

// Test deserializing a nonexisting data dir
template<> template<> void to::test<2>()
{
    reset_test_dir();
    Memdb memdb;
    serialize::CSVReader reader(testdir, memdb);
    reader.read();
    wassert(actual(memdb.stations.element_count()) == 0);
    wassert(actual(memdb.stationvalues.element_count()) == 0);
    wassert(actual(memdb.levtrs.element_count()) == 0);
    wassert(actual(memdb.values.element_count()) == 0);
}

// Test nasty chars in values
template<> template<> void to::test<3>()
{
    reset_test_dir();

    const char* str_ident = "\"'\n,";
    const char* str_report = "\n\"',";
    Var var_st_1(varinfo(WR_VAR(0, 1, 19)), "'\"\n,");
    var_st_1.seta(newvar(WR_VAR(0, 33, 7), 30));
    Var var_1(varinfo(WR_VAR(0, 12, 101)), 274.0);
    var_1.seta(newvar(WR_VAR(0, 1, 212), "'\"\n,"));

    {
        Memdb memdb;
        memdb.insert(Coord(45, 11), str_ident, str_report, Level(1), Trange::instant(), Datetime(2013, 12, 15), var_1);
        memdb.stationvalues.insert(*memdb.stations[0], var_st_1);

        serialize::CSVWriter serializer(testdir);
        serializer.write(memdb);
        serializer.commit();
    }

    {
        Memdb memdb;
        serialize::CSVReader reader(testdir, memdb);
        reader.read();
        wassert(actual(memdb.stations.element_count()) == 1);
        wassert(actual(memdb.stations[0]->coords) == Coord(45, 11));
        wassert(actual(memdb.stations[0]->mobile).istrue());
        wassert(actual(memdb.stations[0]->ident) == str_ident);
        wassert(actual(memdb.stations[0]->report) == str_report);

        wassert(actual(memdb.stationvalues.element_count()) == 1);
        wassert(actual(memdb.stationvalues[0]->station.id) == memdb.stations[0]->id);
        wassert(actual(*(memdb.stationvalues[0]->var)) == var_st_1);

        wassert(actual(memdb.levtrs.element_count()) == 1);
        wassert(actual(memdb.levtrs[0]->level) == Level(1));
        wassert(actual(memdb.levtrs[0]->trange) == Trange::instant());

        wassert(actual(memdb.values.element_count()) == 1);
        wassert(actual(memdb.values[0]->station.id) == memdb.stations[0]->id);
        wassert(actual(memdb.values[0]->levtr.level) == Level(1));
        wassert(actual(memdb.values[0]->levtr.trange) == Trange::instant());
        wassert(actual(memdb.values[0]->datetime) == Datetime(2013, 12, 15));
        wassert(actual(*(memdb.values[0]->var)) == var_1);
    }
}

}

