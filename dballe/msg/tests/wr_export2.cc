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

#include "msg/test-utils-msg.h"
#include "msg/wr_codec.h"
#include "msg/msgs.h"
#include "msg/context.h"
#include <wreport/bulletin.h>
#include <wreport/conv.h>
#include <wreport/notes.h>
#include <wibble/string.h>
#include <set>
#include <cstring>

using namespace dballe;
using namespace wreport;
using namespace wibble;
using namespace std;
using namespace dballe::tests::tweaks;

namespace tut {

// There is a limit of 50 test per fixture, so wr_export tests overflow here
struct wr_export2_shar
{
    wr_export2_shar()
    {
    }

    ~wr_export2_shar()
    {
    }
};
TESTGRP(wr_export2);

#include "wr_export.h"

// Test correct import/export of a temp with thousands of levels
template<> template<>
void to::test<1>()
{
    BufrReimportTest test("bufr/temp-huge.bufr");
    run_test(test, do_test, "temp-wmo");
}

// Test import/export of ECMWF synop ship
template<> template<>
void to::test<2>()
{
    BufrReimportTest test("bufr/ecmwf-ship-1-11.bufr");
    run_test(test, do_test, "ship-wmo");
}

// Test import/export of ECMWF synop ship record 2
template<> template<>
void to::test<3>()
{
    BufrReimportTest test("bufr/ecmwf-ship-1-12.bufr");
    run_test(test, do_test, "ship-wmo");
}

// Test import/export of ECMWF synop ship (auto)
template<> template<>
void to::test<4>()
{
    BufrReimportTest test("bufr/ecmwf-ship-1-13.bufr");
    run_test(test, do_test, "ship-wmo");
}

// Test import/export of ECMWF synop ship (auto) record 2
template<> template<>
void to::test<5>()
{
    BufrReimportTest test("bufr/ecmwf-ship-1-14.bufr");
    run_test(test, do_test, "ship-wmo");
}

}

/* vim:set ts=4 sw=4: */
