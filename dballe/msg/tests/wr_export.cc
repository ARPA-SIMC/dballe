/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

struct wr_export_shar
{
    wr_export_shar()
    {
    }

    ~wr_export_shar()
    {
    }
};
TESTGRP(wr_export);

#include "wr_export.h"

// Test that plain re-export of all our BUFR test files is possible
template<> template<>
void to::test<1>()
{
    // note: These were blacklisted:
    //      "bufr/obs3-3.1.bufr",
    //      "bufr/obs3-56.2.bufr",
    //      "bufr/test-buoy1.bufr", 
    //      "bufr/test-soil1.bufr", 
    set<string> bl_crex;
    // CREX tables do not contain the entries needed to encode pollution
    // messages
    bl_crex.insert("bufr/ed4.bufr");

    const char** files = dballe::tests::bufr_files;
    vector<string> fails;
    int i;
    for (i = 0; files[i] != NULL; i++)
    {
        try {
            auto_ptr<Msgs> msgs = read_msgs(files[i], BUFR);
            ensure(msgs->size() > 0);

            std::auto_ptr<msg::Exporter> exporter;

            exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
            auto_ptr<Bulletin> bbulletin(BufrBulletin::create());
            exporter->to_bulletin(*msgs, *bbulletin);

            if (bl_crex.find(files[i]) == bl_crex.end())
            {
                exporter = msg::Exporter::create(CREX/*, const Options& opts=Options()*/);
                auto_ptr<Bulletin> cbulletin(CrexBulletin::create());
                exporter->to_bulletin(*msgs, *cbulletin);
            }
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

// Test that plain re-export of all our CREX test files is possible
template<> template<>
void to::test<2>()
{
    const char** files = dballe::tests::crex_files;

    vector<string> fails;
    int i;
    for (i = 0; files[i] != NULL; i++)
    {
        try {
            auto_ptr<Msgs> msgs = read_msgs(files[i], CREX);
            ensure(msgs->size() > 0);

            std::auto_ptr<msg::Exporter> exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
            auto_ptr<Bulletin> bbulletin(BufrBulletin::create());
            exporter->to_bulletin(*msgs, *bbulletin);

            exporter = msg::Exporter::create(CREX/*, const Options& opts=Options()*/);
            auto_ptr<Bulletin> cbulletin(CrexBulletin::create());
            exporter->to_bulletin(*msgs, *cbulletin);
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

// Export a well known TEMP which used to fail
template<> template<>
void to::test<3>()
{
    auto_ptr<Msgs> msgs = read_msgs_csv("csv/temp1.csv");
    ensure(msgs->size() > 0);

    // Replace with packed levels because comparison later happens against
    // packed levels
    {
        auto_ptr<Msg> msg(new Msg);
        msg->sounding_pack_levels(*(*msgs)[0]);
        msgs.reset(new Msgs);
        msgs->acquire(msg);
    }

    // Export to BUFR
    std::auto_ptr<msg::Exporter> bufr_exporter(msg::Exporter::create(BUFR/*, const Options& opts=Options()*/));
    auto_ptr<Bulletin> bbulletin(BufrBulletin::create());
    bufr_exporter->to_bulletin(*msgs, *bbulletin);

    // Import and check the differences
    {
        std::auto_ptr<msg::Importer> bufr_importer(msg::Importer::create(BUFR/*, const Options& opts=Options()*/));
        Msgs msgs1;
        bufr_importer->from_bulletin(*bbulletin, msgs1);
        notes::Collect c(cerr);
        ensure_equals(msgs->diff(msgs1), 0);
    }

    // Export to CREX
    std::auto_ptr<msg::Exporter> crex_exporter(msg::Exporter::create(CREX/*, const Options& opts=Options()*/));
    auto_ptr<Bulletin> cbulletin(CrexBulletin::create());
    crex_exporter->to_bulletin(*msgs, *cbulletin);

    // Import and check the differences
    {
        std::auto_ptr<msg::Importer> crex_importer(msg::Importer::create(CREX/*, const Options& opts=Options()*/));
        Msgs msgs1;
        crex_importer->from_bulletin(*cbulletin, msgs1);
        notes::Collect c(cerr);
        ensure_equals(msgs->diff(msgs1), 0);
    }
}

// Re-export test for old style synops
template<> template<>
void to::test<5>()
{
    dballe::tests::TestCodec test("bufr/obs0-1.22.bufr");
    test.expected_min_vars = 34;
    test.configure_ecmwf_to_wmo_tweaks();

    TEST_reimport(test);
    TEST_convert(test, "synop-wmo");

    BufrReimportTest test1("bufr/obs0-1.22.bufr");
    run_test(test1, do_ecmwf, "synop");
}
template<> template<>
void to::test<6>()
{
    BufrReimportTest test("bufr/obs0-1.11188.bufr");
    run_test(test, do_ecmwf, "synop");
}
template<> template<>
void to::test<7>()
{
    BufrReimportTest test("bufr/obs0-3.504.bufr");
    run_test(test, do_ecmwf, "synop");
}

// Re-export test for new style synops
template<> template<>
void to::test<8>()
{
    BufrReimportTest test("bufr/synop-cloudbelow.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<9>()
{
    BufrReimportTest test("bufr/synop-evapo.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 2);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<10>()
{
    BufrReimportTest test("bufr/synop-groundtemp.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<11>()
{
    BufrReimportTest test("bufr/synop-longname.bufr");
    test.tweaks.push_back(new TruncStName());
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<12>()
{
    BufrReimportTest test("bufr/synop-oddgust.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 0);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<13>()
{
    BufrReimportTest test("bufr/synop-oddprec.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    test.wmo_tweaks.push_back(new RemoveSynopWMOOddprec());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 0);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<14>()
{
    BufrReimportTest test("bufr/synop-strayvs.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<15>()
{
    BufrReimportTest test("bufr/synop-sunshine.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}

template<> template<>
void to::test<16>()
{
    BufrReimportTest test("bufr/synop-gtscosmo.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
    ensure_equals(test.exported->type, 0);
    ensure_equals(test.exported->subtype, 2);
    ensure_equals(test.exported->localsubtype, 255);
}

// Re-export test for old style temps
template<> template<>
void to::test<17>()
{
    BufrReimportTest test("bufr/obs2-101.16.bufr");
    test.ecmwf_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_ecmwf, "temp");
}

template<> template<>
void to::test<18>()
{
    BufrReimportTest test("bufr/obs2-102.1.bufr");
    test.ecmwf_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_ecmwf, "temp");
}

template<> template<>
void to::test<19>()
{
    BufrReimportTest test("bufr/obs2-91.2.bufr");
    run_test(test, do_ecmwf, "pilot");
}
template<> template<>
void to::test<20>()
{
    BufrReimportTest test("bufr/temp-bad3.bufr");
    test.ecmwf_tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    run_test(test, do_ecmwf, "temp");
}
template<> template<>
void to::test<21>()
{
    // This has some sounding groups with undefined VSS
    BufrReimportTest test("bufr/temp-bad5.bufr");
    run_test(test, do_ecmwf, "temp");
}
template<> template<>
void to::test<22>()
{
    // This has some sounding groups with undefined VSS, and an unusual template
    BufrReimportTest test("bufr/test-temp1.bufr");
    test.ecmwf_tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    run_test(test, do_ecmwf, "temp");
    //test.wmo_tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    //test.tweaks.push_back(new StripQCAttrs());
    //run_test(test, do_wmo, "temp");
}
template<> template<>
void to::test<23>()
{
    // This has an unusual template
    BufrReimportTest test("bufr/C23000.bufr");
    //test.verbose = true;
    StripQCAttrs* sqa = new StripQCAttrs();
    sqa->codes.push_back(WR_VAR(0, 10, 3));
    test.tweaks.push_back(sqa);
    test.tweaks.push_back(new StripSubstituteAttrs());

    test.ecmwf_tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    run_test(test, do_ecmwf, "temp");
    //test.wmo_tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    //run_test(test, do_wmo, "temp");
}

// Re-export test for new style temps
template<> template<>
void to::test<24>()
{
    BufrReimportTest test("bufr/temp-gts1.bufr");
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    test.wmo_tweaks.push_back(new RoundVSS());
    run_test(test, do_wmo, "temp");
    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 4);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<25>()
{
    BufrReimportTest test("bufr/temp-gts2.bufr");
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 4);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<26>()
{
    BufrReimportTest test("bufr/temp-gts3.bufr");
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 4);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<27>()
{
    BufrReimportTest test("bufr/temp-gtscosmo.bufr");
    //test.verbose = true;
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 4);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<28>()
{
#warning This is importer with height above ground levels, but exported with pressure levels
#if 0
    // Another weird template
    BufrReimportTest test("bufr/temp-2-255.bufr");
    run_test(test, do_wmo, "temp");
    //test.output_opts.template_name = "temp-wmo";
    //run_test(test, do_test, "auto");
    //test.output_opts.template_name = "temp-ecmwf";
    //test.clear_tweaks();
    //run_test(test, do_test, "old");
#endif
}
template<> template<>
void to::test<29>()
{
    BufrReimportTest test("bufr/temp-bad1.bufr");
    StripQCAttrs* sqa;
    test.tweaks.push_back(sqa = new StripQCAttrs());
    test.tweaks.push_back(new StripSubstituteAttrs());
    sqa->codes.push_back(WR_VAR(0, 10, 3));
    run_test(test, do_ecmwf, "temp");
}
template<> template<>
void to::test<30>()
{
    BufrReimportTest test("bufr/temp-bad2.bufr");
    StripQCAttrs* sqa = new StripQCAttrs();
    sqa->codes.push_back(WR_VAR(0, 10, 3));
    test.tweaks.push_back(sqa);
    test.tweaks.push_back(new StripSubstituteAttrs());
    test.tweaks.push_back(new RemoveOddTempTemplateOnlyVars());
    run_test(test, do_ecmwf, "temp");
}
template<> template<>
void to::test<31>()
{
    BufrReimportTest test("bufr/temp-bad4.bufr");
    run_test(test, do_test, "temp-wmo");
    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 4);
    ensure_equals(test.exported->localsubtype, 255);
}
template<> template<>
void to::test<32>()
{
#warning There is no template that can export these forecast TEMPs except generic
#if 0
    // generic temp with forecast info and hybrid levels (?)
    BufrReimportTest test("bufr/tempforecast.bufr");
    run_test(test, do_test, "temp-wmo");
    test.clear_tweaks();
    test.tweaks.push_back(new StripQCAttrs());
    run_test(test, do_test, "temp-ecmwf", "temp-wmo");
#endif
}

template<> template<>
void to::test<33>()
{
#warning no documentation on new stile ACARS available yet
    //BufrReimportTest test("bufr/gts-acars1.bufr");
    //run_test(test, do_test, "acars-wmo");
}

template<> template<>
void to::test<34>()
{
#warning no documentation on new stile ACARS available yet
    //BufrReimportTest test("bufr/gts-acars2.bufr");
    //run_test(test, do_test, "acars-wmo");
}

template<> template<>
void to::test<35>()
{
#warning no documentation on new stile ACARS available yet
    //BufrReimportTest test("bufr/gts-acars-uk1.bufr");
    //run_test(test, do_test, "acars-wmo");
}

template<> template<>
void to::test<36>()
{
#warning no documentation on new stile ACARS available yet
    //BufrReimportTest test("bufr/gts-acars-us1.bufr");
    //run_test(test, do_test, "acars-wmo");
}

template<> template<>
void to::test<37>()
{
    BufrReimportTest test("bufr/gts-amdar1.bufr");
    run_test(test, do_test, "amdar-wmo");
}

template<> template<>
void to::test<38>()
{
    BufrReimportTest test("bufr/gts-amdar2.bufr");
    run_test(test, do_test, "amdar-wmo");
}

// Re-export to BUFR (simplified, full template autodetect) and see the differences
template<> template<>
void to::test<39>()
{
    const char** files = dballe::tests::bufr_files;
    // Uncomment to single out one failing file
    //const char* files[] = { "bufr/temp-2-255.bufr", NULL };
    set<string> blacklist;
    // Generics, would need a template override to avoid the per-rete
    // autodetected export
    blacklist.insert("bufr/tempforecast.bufr");
    blacklist.insert("bufr/obs255-255.0.bufr");
    // Nonstandard messages that need tweaks, tested individually above
    blacklist.insert("bufr/test-temp1.bufr");
    blacklist.insert("bufr/C23000.bufr");
    blacklist.insert("bufr/temp-2-255.bufr");
    blacklist.insert("bufr/synop-longname.bufr");
    blacklist.insert("bufr/temp-bad4.bufr");

    vector<string> fails;
    int i;
    std::auto_ptr<msg::Exporter> exporter;
    exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
    std::auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR/*, opts*/);

    for (i = 0; files[i] != NULL; i++)
    {
        if (blacklist.find(files[i]) != blacklist.end()) continue;
        try {
            // Import
            auto_ptr<Msgs> msgs = read_msgs(files[i], BUFR);
            ensure(msgs->size() > 0);

            // Export
            auto_ptr<Bulletin> bbulletin(BufrBulletin::create());
            exporter->to_bulletin(*msgs, *bbulletin);

            // Import again
            Msgs msgs1;
            importer->from_bulletin(*bbulletin, msgs1);

            // Compare
            notes::Collect c(cerr);
            int diffs = msgs->diff(msgs1);
            if (diffs)
            {
                FILE* out1 = fopen("/tmp/msg1.txt", "w");
                FILE* out2 = fopen("/tmp/msg2.txt", "w");
                msgs->print(out1);
                msgs1.print(out2);
                fclose(out1);
                fclose(out2);
            }
            ensure_equals(diffs, 0);
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

// Re-export to BUFR (not simplified, full template autodetect) and see the differences
template<> template<>
void to::test<40>()
{
    const char** files = dballe::tests::bufr_files;
    // Uncomment to single out one failing file
    //const char* files[] = { "bufr/obs0-1.22.bufr", NULL };
    set<string> blacklist;
    // Generics, would need a template override to avoid the per-rete
    // autodetected export
    blacklist.insert("bufr/tempforecast.bufr");
    blacklist.insert("bufr/obs255-255.0.bufr");
    // Nonstandard messages that need tweaks, tested individually above
    blacklist.insert("bufr/test-temp1.bufr");
    blacklist.insert("bufr/C23000.bufr");
    blacklist.insert("bufr/temp-2-255.bufr");
    blacklist.insert("bufr/synop-longname.bufr");
    blacklist.insert("bufr/temp-bad4.bufr");

    vector<string> fails;
    int i;
    std::auto_ptr<msg::Exporter> exporter;
    exporter = msg::Exporter::create(BUFR/*, const Options& opts=Options()*/);
    msg::Importer::Options import_opts;
    import_opts.simplified = false;
    std::auto_ptr<msg::Importer> importer = msg::Importer::create(BUFR, import_opts);

    for (i = 0; files[i] != NULL; i++)
    {
        if (blacklist.find(files[i]) != blacklist.end()) continue;
        try {
            // Import
            auto_ptr<Msgs> msgs = read_msgs_opts(files[i], BUFR, import_opts);
            ensure(msgs->size() > 0);

            // Export
            auto_ptr<Bulletin> bbulletin(BufrBulletin::create());
            exporter->to_bulletin(*msgs, *bbulletin);

            // Import again
            Msgs msgs1;
            importer->from_bulletin(*bbulletin, msgs1);

            // Compare
            stringstream str;
            notes::Collect c(str);
            int diffs = msgs->diff(msgs1);
            if (diffs)
            {
                string tag = str::basename(files[i]);
                dballe::tests::dump("dballe-orig-" + tag, *msgs, "original message");
                dballe::tests::dump("dballe-reenc-" + tag, *bbulletin, "reencoded message");
                dballe::tests::dump("dballe-reenc-" + tag, msgs1, "decoded reencoded message");
                dballe::tests::dump("dballe-diffs-" + tag, str.str(), "differences");
            }
            ensure_equals(diffs, 0);
        } catch (std::exception& e) {
            fails.push_back(string(files[i]) + ": " + e.what());
        }
    }
    if (!fails.empty())
        throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
}

template<> template<>
void to::test<41>()
{
    BufrReimportTest test("bufr/pilot-gts1.bufr");
    run_test(test, do_test, "pilot-wmo");

    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}

template<> template<>
void to::test<42>()
{
    BufrReimportTest test("bufr/pilot-gts1.bufr");
    run_test(test, do_test, "pilot-wmo");

    ensure_equals(test.exported->type, 2);
    ensure_equals(test.exported->subtype, 1);
    ensure_equals(test.exported->localsubtype, 255);
}

template<> template<>
void to::test<43>()
{
    BufrReimportTest test("bufr/pilot-ecmwf-geopotential.bufr");
    test.tweaks.push_back(new StripQCAttrs());
    StripVars* sv = new StripVars();
    sv->codes.push_back(WR_VAR(0, 10, 8));
    test.tweaks.push_back(sv);
    run_test(test, do_test, "pilot-wmo");
    //run_test(test, do_wmo, "pilot");
}

// Test for a bug where geopotential levels became pressure levels
template<> template<>
void to::test<44>()
{
    auto_ptr<Msgs> msgs1 = read_msgs("bufr/pilot-ecmwf-geopotential.bufr", BUFR);
    ensure_equals(msgs1->size(), 1);
    Msg& msg1 = *(*msgs1)[0];

    // Geopotential levels are converted to height above msl
    const msg::Context* c = msg1.find_context(Level(102, 900), Trange(254, 0, 0));
    ensure(c != NULL);

    // Convert to WMO template
    msg::Exporter::Options output_opts;
    output_opts.template_name = "pilot-wmo";
    //if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
    std::auto_ptr<BufrBulletin> bulletin = BufrBulletin::create();
    test_export_msgs(*msgs1, *bulletin, "towmo", output_opts);

    // Import again
    Msgs msgs2;
    std::auto_ptr<msg::Importer> imp = msg::Importer::create(BUFR);
    imp->from_bulletin(*bulletin, msgs2);
    ensure_equals(msgs2.size(), 1);
    Msg& msg2 = *msgs2[0];

    // Ensure we didn't get pressure levels
    ensure(msg2.find_context(Level(100, 900), Trange(254, 0, 0)) == NULL);
}

// Test for a range error in one specific BUFR
template<> template<>
void to::test<45>()
{
    auto_ptr<Msgs> msgs1 = read_msgs("bufr/temp-2-255.bufr", BUFR);
    ensure_equals(msgs1->size(), 1);
    Msg& msg1 = *(*msgs1)[0];

    // Convert to CREX
    msg::Exporter::Options output_opts;
    output_opts.template_name = "temp-wmo";
    std::auto_ptr<CrexBulletin> bulletin = CrexBulletin::create();
    test_export_msgs(*msgs1, *bulletin, "tocrex", output_opts);

    // Import again
    Msgs msgs2;
    std::auto_ptr<msg::Importer> imp = msg::Importer::create(BUFR);
    imp->from_bulletin(*bulletin, msgs2);
    ensure_equals(msgs2.size(), 1);
    Msg& msg2 = *msgs2[0];
}

// Test that temp ship subtype is set correctly
template<> template<>
void to::test<46>()
{
    std::auto_ptr<Msgs> msgs = read_msgs("bufr/obs2-102.1.bufr", BUFR);
    auto_ptr<Bulletin> bulletin(BufrBulletin::create());
    msg::Exporter::Options opts;
    opts.template_name = "temp-wmo";
    test_export_msgs(*msgs, *bulletin, "tempship", opts);
    ensure_equals(bulletin->type, 2);
    ensure_equals(bulletin->subtype, 5);
    ensure_equals(bulletin->localsubtype, 255);
}

// Test that pilot subtype is set correctly
template<> template<>
void to::test<47>()
{
    std::auto_ptr<Msgs> msgs = read_msgs("bufr/obs2-91.2.bufr", BUFR);
    auto_ptr<Bulletin> bulletin(BufrBulletin::create());
    msg::Exporter::Options opts;
    opts.template_name = "pilot-wmo";
    test_export_msgs(*msgs, *bulletin, "pilotwmo", opts);
    ensure_equals(bulletin->type, 2);
    ensure_equals(bulletin->subtype, 1);
    ensure_equals(bulletin->localsubtype, 255);
}

// Test conversion from amdar ECMWF to amdar WMO
template<> template<>
void to::test<48>()
{
    BufrReimportTest test("bufr/ecmwf-amdar1.bufr");
    test.tweaks.push_back(new StripQCAttrs());
    run_test(test, do_test, "amdar");
}

// Test conversion from acars ECMWF to amdar WMO
template<> template<>
void to::test<49>()
{
    BufrReimportTest test("bufr/ecmwf-acars1.bufr");
    test.tweaks.push_back(new StripQCAttrs());
    run_test(test, do_test, "acars");
}

}

/* vim:set ts=4 sw=4: */
