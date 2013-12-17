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
#include "serializer.h"
#include "core/defs.h"
#include "core/var.h"
#include <wibble/sys/fs.h>

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace wibble::tests;
using namespace wibble;
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

template<> template<> void to::test<1>()
{
    reset_test_dir();

    // Create a memdb and write it out
    {
        Memdb memdb;
        memdb.insert(Coord(45, 11), string(), "synop", Level(1), Trange::instant(), Datetime(2013, 12, 15), newvar(WR_VAR(0, 12, 101), 274.0));
        memdb.insert(Coord(45, 12), "LH1234", "airep", Level(1), Trange::instant(), Datetime(2013, 12, 15), newvar(WR_VAR(0, 12, 101), 273.0));

        serialize::CSVWriter serializer(testdir);
        serializer.write(memdb);
    }

    // Read it back
}

}

