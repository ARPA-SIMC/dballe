/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-msg.h>
#include <dballe/msg/wr_codec.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/context.h>
#include <wreport/bulletin.h>
#include <cstring>

using namespace dballe;
using namespace wreport;
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
        ensure_var_equals(*var, val); \
    } while (0)
#define IS2(code, lev, tr, val) do { \
        const Var* var = msg.find(code, lev, tr); \
        ensure(((void)#code #lev #tr, var != 0)); \
        ensure_var_equals(*var, val); \
    } while (0)
#define UN(field) do { \
        const Var* var = msg.get_##field##_var(); \
        if (var != 0) \
            ensure_var_undef(*var); \
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
            auto_ptr<Msgs> msgs = read_msgs(files[i], BUFR);
            ensure(msgs->size() > 0);
        } catch (std::exception& e) {
            cerr << "Failing bulletin:";
            try {
                std::auto_ptr<Rawmsg> raw = read_rawmsg(files[i], BUFR);
                BufrBulletin bulletin;
                bulletin.decode(*raw);
                bulletin.print(stderr);
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
            auto_ptr<Msgs> msgs = read_msgs(files[i], CREX);
            ensure(msgs->size() > 0);
        } catch (std::exception& e) {
            cerr << "Failing bulletin:";
            try {
                std::auto_ptr<Rawmsg> raw = read_rawmsg(files[i], CREX);
                CrexBulletin bulletin;
                bulletin.decode(*raw);
                bulletin.print(stderr);
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
    auto_ptr<Msgs> msgs = read_msgs("crex/test-synop0.crex", CREX);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 10); IS(station, 837); IS(st_type, 1);
    IS(year, 2004); IS(month, 11); IS(day, 30); IS(hour, 12); IS(minute, 0);
    IS(latitude, 48.22); IS(longitude, 9.92);
    IS(height_station, 550.0); UN(height_baro);
    IS(press, 94340.0); IS(press_msl, 100940.0); IS(press_tend, 7.0);
    IS(wind_dir, 80.0); IS(wind_speed, 6.0);
    IS(temp_2m, 276.15); IS(dewpoint_2m, 273.85); UN(humidity);
    IS(visibility, 5000.0); IS(pres_wtr, 10); IS(past_wtr1, 2); IS(past_wtr2, 2);
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
    auto_ptr<Msgs> msgs = read_msgs("bufr/obs0-1.22.bufr", BUFR);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    IS(block, 60); IS(station, 150); IS(st_type, 1);
    IS(year, 2004); IS(month, 11); IS(day, 30); IS(hour, 12); IS(minute, 0);
    IS(latitude, 33.88); IS(longitude, -5.53);
    IS(height_station, 560.0); UN(height_baro);
    IS(press, 94190.0); IS(press_msl, 100540.0); IS(press_3h, -180.0); IS(press_tend, 8.0);
    IS(wind_dir, 80.0); IS(wind_speed, 4.0);
    IS(temp_2m, 289.2); IS(dewpoint_2m, 285.7); UN(humidity);
    IS(visibility, 8000.0); IS(pres_wtr, 2); IS(past_wtr1, 6); IS(past_wtr2, 2);
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
    auto_ptr<Msgs> msgs = read_msgs_opts("bufr/synop-cloudbelow.bufr", BUFR, opts);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    // msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    IS(year, 2009); IS(month, 12); IS(day, 3); IS(hour, 15); IS(minute, 0);
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height_station, 483.0); IS(height_baro, 490.0);
    IS(press, 95090.0); IS(press_msl, 101060.0); IS(press_3h, -110.0); IS(press_tend, 6.0);
    IS2(WR_VAR(0, 11, 1), Level(103, 10000), Trange(0, 0, 600), 0.0); // wind_dir
    IS2(WR_VAR(0, 11, 2), Level(103, 10000), Trange(0, 0, 600), 1.0); // wind_speed
    IS(temp_2m, 273.05); IS(dewpoint_2m, 271.35); IS(humidity, 88.0);
    IS(visibility, 14000.0); IS(pres_wtr, 508);
    IS2(WR_VAR(0, 20, 4), Level(1), Trange(205, 0, -10800), 10); // past_wtr1
    IS2(WR_VAR(0, 20, 5), Level(1), Trange(205, 0, -10800), 10); // past_wtr2
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
    auto_ptr<Msgs> msgs = read_msgs_opts("bufr/synop-cloudbelow.bufr", BUFR, opts);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_SYNOP);

    // msg.print(stderr);

    IS(block, 11); IS(station, 406); IS(st_type, 1);
    IS(year, 2009); IS(month, 12); IS(day, 3); IS(hour, 15); IS(minute, 0);
    IS(latitude, 50.07361); IS(longitude, 12.40333);
    IS(height_station, 483.0); IS(height_baro, 490.0);
    IS2(WR_VAR(0, 10,  4), Level(102, 490000), Trange::instant(), 95090.0); // press
    IS2(WR_VAR(0, 10, 51), Level(102, 490000), Trange::instant(), 101060.0); // press_msl
    IS2(WR_VAR(0, 10, 63), Level(102, 490000), Trange(205, 0,10800), 6.0); // press_tend
    IS2(WR_VAR(0, 10, 60), Level(102, 490000), Trange(4, 0, 10800), -110.0); // press_3h
    IS2(WR_VAR(0, 11, 1), Level(103, 10000), Trange(0, 0, 600), 0.0); // wind_dir
    IS2(WR_VAR(0, 11, 2), Level(103, 10000), Trange(0, 0, 600), 1.0); // wind_speed
    IS2(WR_VAR(0, 12, 101), Level(103, 2050), Trange::instant(), 273.05); // temp_2m
    IS2(WR_VAR(0, 12, 103), Level(103, 2050), Trange::instant(), 271.35); // dewpoint_2m
    IS2(WR_VAR(0, 13,   3), Level(103, 2050), Trange::instant(), 88.0); // humidity
    IS2(WR_VAR(0, 20, 1), Level(103, 8000), Trange::instant(), 14000.0); // visibility
    IS(pres_wtr, 508);
    IS2(WR_VAR(0, 20, 4), Level(1), Trange(205, 0, -10800), 10); // past_wtr1
    IS2(WR_VAR(0, 20, 5), Level(1), Trange(205, 0, -10800), 10); // past_wtr2
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
    auto_ptr<Msgs> msgs = read_msgs("bufr/temp-2-255.bufr", BUFR);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_TEMP);

    // No negative pressure layers please
    ensure(msg.find_context(Level(100, -1), Trange::instant()) == 0);
}

template<> template<>
void to::test<8>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/synop-longname.bufr", BUFR);
    ensure_equals(msgs->size(), 7u);
    const Msg& msg = *(*msgs)[2];
    ensure_equals(msg.type, MSG_SYNOP);

    // Check that the long station name has been correctly truncated on import
    const Var* var = msg.get_st_name_var();
    ensure(var != NULL);
    ensure_equals(string(var->enqc()), "Budapest Pestszentlorinc-kulter>");
}

template<> template<>
void to::test<9>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/temp-bad1.bufr", BUFR);
    ensure_equals(msgs->size(), 1u);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<10>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/temp-bad2.bufr", BUFR);
    ensure_equals(msgs->size(), 1u);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<11>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/temp-bad3.bufr", BUFR);
    ensure_equals(msgs->size(), 1u);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_TEMP);
}

template<> template<>
void to::test<12>()
{
    auto_ptr<Msgs> msgs = read_msgs("bufr/temp-bad4.bufr", BUFR);
    ensure_equals(msgs->size(), 1u);
    const Msg& msg = *(*msgs)[0];
    ensure_equals(msg.type, MSG_TEMP);
}


#if 0
/* Check that a BUFR from a synop high-level station correctly reports isobaric
 * surface and geopotential */
template<> template<>
void to::test<7>()
{
    dba_msgs msgs = read_test_msg("bufr/obs0-1.11188.bufr", BUFR);
    dba_msg src = msgs->msgs[0];
    dba_var var;

    //gen_ensure((var = dba_msg_get_isobaric_surface_var(src)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    gen_ensure((var = dba_msg_get_geopotential_var(src)) != NULL);
    gen_ensure(dba_var_value(var) != NULL);

    dba_msgs_delete(msgs);
}

/* Test import of environment BUFR4 messages */
template<> template<>
void to::test<5>()
{
    dba_msgs msgs = read_test_msg("bufr/ed4.bufr", BUFR);
    dba_msg src = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    gen_ensure(dba_var_value(var) != NULL);
    CHECKED(dba_var_enqd(var, &val));
    gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<6>()
{
    dba_msgs msgs = read_test_msg("bufr/ed4-compr-string.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<7>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-cloudbelow.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<8>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-groundtemp.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<9>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-sunshine.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    // Check the context information for the wind data
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 1), 103, 10000, 0, 0, 0, 0, 600)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 140);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 41), 103, 10000, 0, 0, 205, 0, 600)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 15.4);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 41), 103, 10000, 0, 0, 205, 0, 10800)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 15.4);

    dba_msgs_delete(msgs);
}

/* Test import of a WMO GTS synop message with a stray vertical significance */
template<> template<>
void to::test<10>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-strayvs.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);

    //gen_ensure((var = dba_msg_find(src, DBA_VAR(0, 15, 193), 103, 3000, 0, 0, 0, -3600, 3600)) != NULL);
    //gen_ensure(dba_var_value(var) != NULL);
    //CHECKED(dba_var_enqd(var, &val));
    //gen_ensure_equals(val, 2700000e-14);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<11>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-evapo.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 14);

    msg = msgs->msgs[4];
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<12>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-oddprec.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 1);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS synop messages */
template<> template<>
void to::test<13>()
{
    dba_msgs msgs = read_test_msg("bufr/synop-oddgust.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_SYNOP);
    gen_ensure_equals(msgs->len, 26);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<14>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts1.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 1);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 56);

    // Ensure we got the wind shear section
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 61), 100, 35560, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 13.1);
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 11, 62), 100, 35560, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 5.6);

    // Ensure the extended vertical significances are put in the right
    // level, since they appear before the pressure context
    gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 8, 42), 100, 100000, 0, 0, 254, 0, 0)) != NULL);
    CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 65536);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<15>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts2.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 6);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 45);


    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* Test import of WMO GTS temp messages */
template<> template<>
void to::test<16>()
{
    dba_msgs msgs = read_test_msg("bufr/temp-gts3.bufr", BUFR);
    dba_msg msg = msgs->msgs[0];
    dba_var var;
    double val;

    gen_ensure_equals(msg->type, MSG_TEMP);
    gen_ensure_equals(msgs->len, 1);

    // Ensure we decoded all the sounding levels
    int pres_lev_count = 0;
    for (int i = 0; i < msg->data_count; ++i)
        if (msg->data[i]->ltype1 == 100)
            ++pres_lev_count;
    gen_ensure_equals(pres_lev_count, 26);

    //msg = msgs->msgs[4];
    //gen_ensure((var = dba_msg_find(msg, DBA_VAR(0, 13, 33), 1, 0, 0, 0, 1, -86400, 86400)) != NULL);
    //CHECKED(dba_var_enqd(var, &val)); gen_ensure_equals(val, 0.8);

    dba_msgs_delete(msgs);
}

/* TODO: add entries for more of the sample messages, taking data from another decoder */
#endif

}

/* vim:set ts=4 sw=4: */
