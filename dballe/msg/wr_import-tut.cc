/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "tests.h"
#include "wr_codec.h"
#include "msg.h"
#include "context.h"
#include <wreport/bulletin.h>
#include <wreport/options.h>
#include <cstring>

using namespace dballe;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct wr_import_shar
{
    wr_import_shar()
    {
    }

    ~wr_import_shar()
    {
    }
};
TESTGRP(wr_import);

#define IS(field, val) do { \
        const Var* var = msg.get_##field##_var(); \
        ensure(((void)#field, var != 0)); \
        wassert(actual(*var) == val); \
    } while (0)
#define IS2(code, lev, tr, val) do { \
        const Var* var = msg.get(code, lev, tr); \
        ensure(((void)#code #lev #tr, var != 0)); \
        wassert(actual(*var) == val); \
    } while (0)
#define UN(field) do { \
        const Var* var = msg.get_##field##_var(); \
        if (var != 0) \
            wassert(actual(*var).is_undef()); \
    } while (0)

// Test plain import of all our BUFR test files
template<> template<>
void to::test<1>()
{
    // note: These were blacklisted:
    //      "bufr/obs3-3.1.bufr",
    //      "bufr/obs3-56.2.bufr",
    //      "bufr/test-buoy1.bufr", 
    //      "bufr/test-soil1.bufr", 
    const char** files = dballe::tests::bufr_files;

    for (int i = 0; files[i] != NULL; i++)
    {
        try {
            Messages msgs = read_msgs(files[i], File::BUFR);
            ensure(msgs.size() > 0);
        } catch (std::exception& e) {
            cerr << "Failing bulletin:";
            try {
                BinaryMessage raw = read_rawmsg(files[i], File::BUFR);
                unique_ptr<Bulletin> bulletin(BufrBulletin::decode(raw.data));
                bulletin->print(stderr);
            } catch (std::exception& e1) {
                cerr << "Cannot display failing bulletin: " << e1.what() << endl;
            }
            throw tut::failure(string("[") + files[i] + "] " + e.what());
        }
    }
}

// Test plain import of all our CREX test files
template<> template<>
void to::test<2>()
{
    const char** files = dballe::tests::crex_files;

    for (int i = 0; files[i] != NULL; i++)
    {
        try {
            Messages msgs = read_msgs(files[i], File::CREX);
            ensure(msgs.size() > 0);
        } catch (std::exception& e) {
            cerr << "Failing bulletin:";
            try {
                BinaryMessage raw = read_rawmsg(files[i], File::CREX);
                unique_ptr<Bulletin> bulletin(CrexBulletin::decode(raw.data));
                bulletin->print(stderr);
            } catch (std::exception& e1) {
                cerr << "Cannot display failing bulletin: " << e1.what() << endl;
            }
            throw tut::failure(string("[") + files[i] + "] " + e.what());
        }
    }
}

template<> template<>
void to::test<3>()
{
    Messages msgs = read_msgs("crex/test-synop0.crex", File::CREX);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 10); IS(station, 837); IS(st_type, 1);
    wassert(actual(msg.get_datetime()) == Datetime(2004, 11, 30, 12, 0));
    IS(latitude, 48.22); IS(longitude, 9.92);
    IS(height_station, 550.0); UN(height_baro);
    IS(press, 94340.0); IS(press_msl, 100940.0); IS(press_tend, 7.0);
    IS(wind_dir, 80.0); IS(wind_speed, 6.0);
    IS(temp_2m, 276.15); IS(dewpoint_2m, 273.85); UN(humidity);
    IS(visibility, 5000.0); IS(pres_wtr, 10); IS(past_wtr1_6h, 2); IS(past_wtr2_6h, 2);
    IS(cloud_n, 100); IS(cloud_nh, 8); IS(cloud_hh, 450.0);
    IS(cloud_cl, 35); IS(cloud_cm, 61); IS(cloud_ch, 60);
    IS(cloud_n1, 8); IS(cloud_c1, 6); IS(cloud_h1, 350.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

template<> template<>
void to::test<4>()
{
    Messages msgs = read_msgs("bufr/obs0-1.22.bufr", File::BUFR);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 60); IS(station, 150); IS(st_type, 1);
    wassert(actual(msg.get_datetime()) == Datetime(2004, 11, 30, 12, 0));
    IS(latitude, 33.88); IS(longitude, -5.53);
    IS(height_station, 560.0); UN(height_baro);
    IS(press, 94190.0); IS(press_msl, 100540.0); IS(press_3h, -180.0); IS(press_tend, 8.0);
    IS(wind_dir, 80.0); IS(wind_speed, 4.0);
    IS(temp_2m, 289.2); IS(dewpoint_2m, 285.7); UN(humidity);
    IS(visibility, 8000.0); IS(pres_wtr, 2); IS(past_wtr1_6h, 6); IS(past_wtr2_6h, 2);
    IS(cloud_n, 100); IS(cloud_nh, 8); IS(cloud_hh, 250.0);
    IS(cloud_cl, 39); IS(cloud_cm, 61); IS(cloud_ch, 60);
    IS(cloud_n1, 2); IS(cloud_c1, 8); IS(cloud_h1, 320.0);
    IS(cloud_n2, 5); IS(cloud_c2, 8); IS(cloud_h2, 620.0);
    IS(cloud_n3, 2); IS(cloud_c3, 9); IS(cloud_h3, 920.0);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    IS(tot_prec12, 0.5); UN(tot_snow);
}

template<> template<>
void to::test<5>()
{
    msg::Importer::Options opts;
    opts.simplified = true;
    Messages msgs = read_msgs_opts("bufr/synop-cloudbelow.bufr", File::BUFR, opts);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_SYNOP);

    // msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    wassert(actual(msg.get_datetime()) == Datetime(2009, 12, 3, 15, 0));
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height_station, 483.0); IS(height_baro, 490.0);
    IS(press, 95090.0); IS(press_msl, 101060.0); IS(press_3h, -110.0); IS(press_tend, 6.0);
    IS(wind_dir, 0.0); IS(wind_speed, 1.0);
    IS(temp_2m, 273.05); IS(dewpoint_2m, 271.35); IS(humidity, 88.0);
    IS(visibility, 14000.0); IS(pres_wtr, 508);
    IS2(WR_VAR(0, 20, 4), Level(1), Trange(205, 0, 10800), 10); // past_wtr1
    IS2(WR_VAR(0, 20, 5), Level(1), Trange(205, 0, 10800), 10); // past_wtr2
    IS(cloud_n, 38); IS(cloud_nh, 0); IS(cloud_hh, 6000.0);
    IS(cloud_cl, 30); IS(cloud_cm, 20); IS(cloud_ch, 12);
    IS(cloud_n1, 3); IS(cloud_c1, 0); IS(cloud_h1, 6000.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

template<> template<>
void to::test<6>()
{
    msg::Importer::Options opts;
    opts.simplified = false;
    Messages msgs = read_msgs_opts("bufr/synop-cloudbelow.bufr", File::BUFR, opts);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_SYNOP);

    // msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    wassert(actual(msg.get_datetime()) == Datetime(2009, 12, 3, 15, 0));
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height_station, 483.0); IS(height_baro, 490.0);
    IS2(WR_VAR(0, 10,  4), Level(102, 490000), Trange::instant(), 95090.0); // press
    IS2(WR_VAR(0, 10, 51), Level(102, 490000), Trange::instant(), 101060.0); // press_msl
    IS2(WR_VAR(0, 10, 63), Level(102, 490000), Trange(205, 0,10800), 6.0); // press_tend
    IS2(WR_VAR(0, 10, 60), Level(102, 490000), Trange(4, 0, 10800), -110.0); // press_3h
    IS2(WR_VAR(0, 11, 1), Level(103, 10000), Trange(200, 0, 600), 0.0); // wind_dir
    IS2(WR_VAR(0, 11, 2), Level(103, 10000), Trange(200, 0, 600), 1.0); // wind_speed
    IS2(WR_VAR(0, 12, 101), Level(103, 2050), Trange::instant(), 273.05); // temp_2m
    IS2(WR_VAR(0, 12, 103), Level(103, 2050), Trange::instant(), 271.35); // dewpoint_2m
    IS2(WR_VAR(0, 13,   3), Level(103, 2050), Trange::instant(), 88.0); // humidity
    IS2(WR_VAR(0, 20, 1), Level(103, 8000), Trange::instant(), 14000.0); // visibility
    IS(pres_wtr, 508);
    IS2(WR_VAR(0, 20, 4), Level(1), Trange(205, 0, 10800), 10); // past_wtr1
    IS2(WR_VAR(0, 20, 5), Level(1), Trange(205, 0, 10800), 10); // past_wtr2
    IS(cloud_n, 38); IS(cloud_nh, 0); IS(cloud_hh, 6000.0);
    IS(cloud_cl, 30); IS(cloud_cm, 20); IS(cloud_ch, 12);
    IS(cloud_n1, 3); IS(cloud_c1, 0); IS(cloud_h1, 6000.0);
    UN(cloud_n2); UN(cloud_c2); UN(cloud_h2);
    UN(cloud_n3); UN(cloud_c3); UN(cloud_h3);
    UN(cloud_n4); UN(cloud_c4); UN(cloud_h4);
    UN(tot_prec24); UN(tot_snow);
}

template<> template<>
void to::test<7>()
{
    Messages msgs = read_msgs("bufr/temp-2-255.bufr", File::BUFR);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);

    // No negative pressure layers please
    ensure(msg.find_context(Level(100, -1), Trange::instant()) == 0);
}

template<> template<>
void to::test<8>()
{
    Messages msgs = read_msgs("bufr/synop-longname.bufr", File::BUFR);
    ensure_equals(msgs.size(), 7u);
    const Msg& msg = Msg::downcast(msgs[2]);
    ensure_equals(msg.type, MSG_SYNOP);

    // Check that the long station name has been correctly truncated on import
    const Var* var = msg.get_st_name_var();
    ensure(var != NULL);
    ensure_equals(string(var->enqc()), "Budapest Pestszentlorinc-kulteru");
}

template<> template<>
void to::test<9>()
{
    Messages msgs = read_msgs("bufr/temp-bad1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<10>()
{
    Messages msgs = read_msgs("bufr/temp-bad2.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<11>()
{
    Messages msgs = read_msgs("bufr/temp-bad3.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<12>()
{
    Messages msgs = read_msgs("bufr/temp-bad4.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

// ECWMF AIREP
template<> template<>
void to::test<13>()
{
    Messages msgs = read_msgs("bufr/obs4-142.1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_AIREP);
    IS(ident, "ACA872");
}

// ECWMF AMDAR
template<> template<>
void to::test<14>()
{
    Messages msgs = read_msgs("bufr/obs4-144.4.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_AMDAR);
    IS(ident, "EU4444");
}

// ECWMF ACARS
template<> template<>
void to::test<15>()
{
    Messages msgs = read_msgs("bufr/obs4-145.4.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_ACARS);
    IS(ident, "JBNYR3RA");
}

// WMO ACARS
template<> template<>
void to::test<16>()
{
    Messages msgs = read_msgs("bufr/gts-acars1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_ACARS);
    IS(ident, "EU5331");
}

// WMO ACARS
template<> template<>
void to::test<17>()
{
    Messages msgs = read_msgs("bufr/gts-acars2.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_ACARS);
    IS(ident, "FJCYR4RA");
}

// WMO ACARS UK
template<> template<>
void to::test<18>()
{
    Messages msgs = read_msgs("bufr/gts-acars-uk1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    // This contains the same data as an AMDAR and has undefined subtype and
    // localsubtype, so it gets identified as an AMDAR
    ensure_equals(msg.type, MSG_AMDAR);
    IS(ident, "EU3375");
}

// WMO ACARS US
template<> template<>
void to::test<19>()
{
    Messages msgs = read_msgs("bufr/gts-acars-us1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_ACARS);
    IS(ident, "FJCYR4RA");
}

// WMO AMDAR
template<> template<>
void to::test<20>()
{
    Messages msgs = read_msgs("bufr/gts-amdar1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_AMDAR);
    IS(ident, "EU0274");
}

// WMO AMDAR
template<> template<>
void to::test<21>()
{
    Messages msgs = read_msgs("bufr/gts-amdar2.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_AMDAR);
    IS(ident, "EU7866");
}

// BUFR that has a variable that goes out of range when converted to local B
// table
template<> template<>
void to::test<22>()
{
    try
    {
        // Read and interpretate the message
        BinaryMessage raw = read_rawmsg("bufr/interpreted-range.bufr", File::BUFR);
        std::unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR);
        Messages msgs = importer->from_binary(raw);
        ensure(false);
    } catch (wreport::error_domain& e) {
        //cerr << e.code() << "--" << e.what() << endl;
    }

    {
        wreport::options::LocalOverride<bool> o(wreport::options::var_silent_domain_errors, true);
        Messages msgs = read_msgs("bufr/interpreted-range.bufr", File::BUFR);
        ensure_equals(msgs.size(), 1u);
        const Msg& msg = Msg::downcast(msgs[0]);
        ensure_equals(msg.type, MSG_SHIP);
        IS(ident, "DBBC");
    }
}

// WMO PILOT, with geopotential levels
template<> template<>
void to::test<23>()
{
    Messages msgs = read_msgs("bufr/pilot-gts1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_PILOT);
}

// WMO PILOT, with pressure levels
template<> template<>
void to::test<24>()
{
    Messages msgs = read_msgs("bufr/pilot-gts2.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_PILOT);
}

// WMO PILOT, with pressure levels
template<> template<>
void to::test<25>()
{
    // FIXME: this still fails
    Messages msgs = read_msgs("bufr/temp-tsig-2.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

// WMO pilot pressure
template<> template<>
void to::test<26>()
{
    Messages msgs = read_msgs("bufr/pilot-gts3.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_PILOT);
}

// WMO pilot geopotential
template<> template<>
void to::test<27>()
{
    Messages msgs = read_msgs("bufr/pilot-gts4.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_PILOT);
}

template<> template<>
void to::test<28>()
{
    Messages msgs = read_msgs("bufr/vad.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

// Wind profiler
template<> template<>
void to::test<29>()
{
    Messages msgs = read_msgs("bufr/temp-windprof1.bufr", File::BUFR);
    ensure_equals(msgs.size(), 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    ensure_equals(msg.type, MSG_TEMP);
}

// Precise import
template<> template<>
void to::test<30>()
{
    msg::Importer::Options opts;
    opts.simplified = false;
    Messages msgs = read_msgs_opts("bufr/gts-synop-linate.bufr", File::BUFR, opts);
    wassert(actual(msgs.size()) == 1u);
    const Msg& msg = Msg::downcast(msgs[0]);
    const Var* v = msg.get(WR_VAR(0, 12, 101), Level(103, 2000), Trange(3, 0, 43200));
    wassert(actual(v).istrue());
    wassert(actual(v->enqd()) == 284.75);
}

}
