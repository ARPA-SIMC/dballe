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
    dballe::tests::TestCodec test("bufr/temp-huge.bufr");
    test.expected_min_vars = 30000;
    TEST_reimport(test);

    BufrReimportTest testold("bufr/temp-huge.bufr");
    run_test(testold, do_test, "temp-wmo");
}

// Test import/export of ECMWF synop ship
template<> template<>
void to::test<2>()
{
    dballe::tests::TestCodec test("bufr/ecmwf-ship-1-11.bufr");
    test.expected_min_vars = 34;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "ship-wmo");
}

// Test import/export of ECMWF synop ship record 2
template<> template<>
void to::test<3>()
{
    dballe::tests::TestCodec test("bufr/ecmwf-ship-1-12.bufr");
    test.expected_min_vars = 21;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "ship-wmo");
}

// Test import/export of ECMWF synop ship (auto)
template<> template<>
void to::test<4>()
{
    dballe::tests::TestCodec test("bufr/ecmwf-ship-1-13.bufr");
    test.expected_min_vars = 30;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "ship-wmo");
}

// Test import/export of ECMWF synop ship (auto) record 2
template<> template<>
void to::test<5>()
{
    dballe::tests::TestCodec test("bufr/ecmwf-ship-1-14.bufr");
    test.expected_min_vars = 28;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "ship-wmo");
}

// Test import/export of WMO synop ship
template<> template<>
void to::test<6>()
{
    dballe::tests::TestCodec test("bufr/wmo-ship-1.bufr");
    test.expected_min_vars = 50;

    TEST_reimport(test);
    TEST_convert(test, "ship-wmo");
}

// Test import/export of ECMWF pilot with pressure levels
template<> template<>
void to::test<7>()
{
    dballe::tests::TestCodec test("bufr/pilot-gts3.bufr");
    test.expected_min_vars = 50;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "pilot-wmo");
}

// Test import/export of ECMWF pilot with geopotential levels
template<> template<>
void to::test<8>()
{
    dballe::tests::TestCodec test("bufr/pilot-gts4.bufr");
    test.expected_min_vars = 50;
    test.configure_ecmwf_to_wmo_tweaks();
    test.after_convert_reimport.add(new dballe::tests::tweaks::HeightToGeopotential);
    test.after_convert_reimport_on_orig.add(new dballe::tests::tweaks::RemoveContext(
                Level(100, 70000), Trange::instant()));
    test.after_convert_reimport_on_orig.add(new dballe::tests::tweaks::RemoveContext(
                Level(100, 85000), Trange::instant()));

    TEST_reimport(test);
    TEST_convert(test, "pilot-wmo");
}

// Test import/export of GTS synop with radiation information
template<> template<>
void to::test<9>()
{
    dballe::tests::TestCodec test("bufr/synop-rad1.bufr");
    test.expected_subsets = 25;
    test.expected_min_vars = 50;

    TEST_reimport(test);
    TEST_convert(test, "synop-wmo");
}

// Test import/export of GTS synop without pressure of standard level
template<> template<>
void to::test<10>()
{
    dballe::tests::TestCodec test("bufr/synop-rad2.bufr");
    test.expected_min_vars = 50;
    test.verbose = true;

    wruntest(test.run_reimport);
    wruntest(test.run_convert, "synop-wmo");
}

// Test import/export of GTS synop with temperature change information
template<> template<>
void to::test<11>()
{
    dballe::tests::TestCodec test("bufr/synop-tchange.bufr");
    test.expected_min_vars = 50;

    wruntest(test.run_reimport);
    TEST_convert(test, "synop-wmo");
}

}

/* vim:set ts=4 sw=4: */
