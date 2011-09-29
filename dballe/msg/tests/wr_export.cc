/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

// Common machinery for import -> export -> reimport tests
template<class B>
struct ReimportTest
{
    typedef dballe::tests::MessageTweaker Tweaker;

    string fname;
    Encoding type;
    auto_ptr<Msgs> msgs1;
    auto_ptr<Msgs> msgs2;
    msg::Importer::Options input_opts;
    msg::Exporter::Options output_opts;
    vector<Tweaker*> tweaks;
    vector<Tweaker*> ecmwf_tweaks;
    vector<Tweaker*> wmo_tweaks;
    bool do_ecmwf_tweaks;
    bool do_wmo_tweaks;
    bool do_ignore_context_attrs;
    bool do_round_geopotential;
    bool verbose;

    void clear_tweaks()
    {
        for (typename vector<Tweaker*>::iterator i = tweaks.begin();
                i != tweaks.end(); ++i)
            delete *i;
        tweaks.clear();
        for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin();
                i != ecmwf_tweaks.end(); ++i)
            delete *i;
        ecmwf_tweaks.clear();
        for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin();
                i != wmo_tweaks.end(); ++i)
            delete *i;
        wmo_tweaks.clear();
    }

    ReimportTest(const std::string& fname, Encoding type=BUFR)
        : fname(fname), type(type), do_ecmwf_tweaks(false), do_wmo_tweaks(false),
          do_ignore_context_attrs(false), do_round_geopotential(false), verbose(false)
    {
        ecmwf_tweaks.push_back(new StripQCAttrs());
        wmo_tweaks.push_back(new RoundLegacyVars());
    }
    ~ReimportTest()
    {
        clear_tweaks();
    }

    void do_test(const dballe::tests::Location& loc, const char* tname1, const char* tname2=NULL)
    {
        if (verbose) cerr << "Running test " << loc.locstr() << endl;

        std::auto_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));

        // Import
        if (verbose) cerr << "Importing " << fname << " " << input_opts.to_string() << endl;
        msgs1 = inner_read_msgs_opts(fname.c_str(), type, input_opts);
        inner_ensure(msgs1->size() > 0);

        // Run tweaks
        for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running tweak " << (*i)->desc() << endl;
            (*i)->tweak(*msgs1);
        }
        if (do_ecmwf_tweaks)
            for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin(); i != ecmwf_tweaks.end(); ++i)
            {
                if (verbose) cerr << "Running ecmwf tweak " << (*i)->desc() << endl;
                (*i)->tweak(*msgs1);
            }
        if (do_wmo_tweaks)
            for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin(); i != wmo_tweaks.end(); ++i)
            {
                if (verbose) cerr << "Running wmo tweak " << (*i)->desc() << endl;
                (*i)->tweak(*msgs1);
            }

        // Export
        auto_ptr<Bulletin> bulletin(B::create());
        try {
            if (tname1 != NULL)
                output_opts.template_name = tname1;
            else
                output_opts.template_name.clear();
            if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
            std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
            exporter->to_bulletin(*msgs1, *bulletin);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *bulletin);
            dballe::tests::dump("msg1", *msgs1);
            throw tut::failure(loc.msg(string("exporting to bulletin (first template): ") + e.what()));
        }

        // Encode
        Rawmsg rawmsg;
        try {
            bulletin->encode(rawmsg);
            //exporter->to_rawmsg(*msgs1, rawmsg);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *bulletin);
            dballe::tests::dump("msg1", *msgs1);
            throw tut::failure(loc.msg(string("encoding to rawmsg (first template): ") + e.what()));
        }

        // Import again
        if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
        msgs2.reset(new Msgs);
        try {
            importer->from_rawmsg(rawmsg, *msgs2);
        } catch (std::exception& e) {
            dballe::tests::dump("msg1", *msgs1);
            dballe::tests::dump("msg", rawmsg);
            throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
        }

        auto_ptr<Msgs> msgs3;
        if (tname2)
        {
            // Export
            auto_ptr<Bulletin> bulletin(B::create());
            try {
                output_opts.template_name = tname2;
                if (verbose) cerr << "Reexporting " << output_opts.to_string() << endl;
                std::auto_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
                exporter->to_bulletin(*msgs2, *bulletin);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", *msgs1);
                throw tut::failure(loc.msg(string("exporting to bulletin (second template): ") + e.what()));
            }

            // Encode
            rawmsg.clear();
            try {
                bulletin->encode(rawmsg);
                //exporter->to_rawmsg(*msgs1, rawmsg);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", *msgs1);
                throw tut::failure(loc.msg(string("encoding to rawmsg (second template): ") + e.what()));
            }

            // Import again
            msgs3.reset(new Msgs);
            try {
                if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
                importer->from_rawmsg(rawmsg, *msgs3);
            } catch (std::exception& e) {
                dballe::tests::dump("msg2", *msgs2);
                dballe::tests::dump("raw2", rawmsg);
                throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
            }
        } else
            msgs3 = msgs2;

#if 0
        // Run tweaks
        for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
            (*i)->clean_second(*msgs3);
#endif
        if (do_ignore_context_attrs)
        {
            StripContextAttrs sca;
            sca.tweak(*msgs1);
        }
        if (do_round_geopotential)
        {
            RoundGeopotential rg;
            rg.tweak(*msgs1);
            rg.tweak(*msgs3);
        }

        // Compare
        stringstream str;
        notes::Collect c(str);
        int diffs = msgs1->diff(*msgs3);
        if (diffs)
        {
            dballe::tests::dump("msg1", *msgs1);
            if (msgs2.get())
                dballe::tests::dump("msg2", *msgs2);
            dballe::tests::dump("msg3", *msgs3);
            dballe::tests::dump("msg", rawmsg);
            dballe::tests::dump("diffs", str.str(), "details of differences");
            throw tut::failure(loc.msg(str::fmtf("found %d differences", diffs)));
        }
    }

#define inner_do_test(name, ...) do_test(wibble::tests::Location(loc, __FILE__, __LINE__, name), __VA_ARGS__)
    void do_ecmwf(const dballe::tests::Location& loc, const char* template_type="synop")
    {
        string ecmwf_template_name = string(template_type) + "-ecmwf";
        string wmo_template_name = string(template_type) + "-wmo";
        do_wmo_tweaks = false;

        input_opts.simplified = true;
        do_round_geopotential = false;

        do_ecmwf_tweaks = false;
        inner_do_test("simp-ecmwf-through-auto", NULL);
        inner_do_test("simp-ecmwf-through-ecmwf", ecmwf_template_name.c_str());

        do_ecmwf_tweaks = true;
        do_round_geopotential = true;
        inner_do_test("simp-ecmwf-through-wmo", wmo_template_name.c_str());

        input_opts.simplified = false;
        do_round_geopotential = false;

        do_ecmwf_tweaks = false;
        inner_do_test("real-ecmwf-through-auto", NULL);
        inner_do_test("real-ecmwf-through-ecmwf", ecmwf_template_name.c_str());

        do_ecmwf_tweaks = true;
        do_round_geopotential = true;
        inner_do_test("real-ecmwf-through-wmo", wmo_template_name.c_str());
    }
    void do_wmo(const dballe::tests::Location& loc, const char* template_type="synop")
    {
        string ecmwf_template_name = string(template_type) + "-ecmwf";
        string wmo_template_name = string(template_type) + "-wmo";
        do_ecmwf_tweaks = false;

        input_opts.simplified = true;
        do_round_geopotential = false;

        do_wmo_tweaks = false;
        inner_do_test("simp-wmo-through-auto", NULL);
        inner_do_test("simp-wmo-through-wmo", wmo_template_name.c_str());

        do_wmo_tweaks = true;
        do_ignore_context_attrs = true;
        do_round_geopotential = true;
        inner_do_test("simp-wmo-through-ecmwf", ecmwf_template_name.c_str());
        do_ignore_context_attrs = false;

        input_opts.simplified = false;
        do_round_geopotential = false;

        do_wmo_tweaks = false;
        inner_do_test("real-wmo-through-auto", NULL);
        inner_do_test("real-wmo-through-wmo", wmo_template_name.c_str());

        // There doesn't seem much sense testing this at the moment
        //do_tweaks = true;
        //inner_do_test("real-wmo-through-ecmwf", ecmwf_template_name.c_str());
    }
#undef inner_do_test
};
typedef ReimportTest<BufrBulletin> BufrReimportTest;
typedef ReimportTest<CrexBulletin> CrexReimportTest;
#define run_test(obj, meth, ...) obj.meth(wibble::tests::Location(__FILE__, __LINE__, (obj.fname + " " #__VA_ARGS__).c_str()), __VA_ARGS__)



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
    BufrReimportTest test("bufr/obs0-1.22.bufr");
    run_test(test, do_ecmwf, "synop");
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
}
template<> template<>
void to::test<9>()
{
    BufrReimportTest test("bufr/synop-evapo.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<10>()
{
    BufrReimportTest test("bufr/synop-groundtemp.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<11>()
{
    BufrReimportTest test("bufr/synop-longname.bufr");
    test.tweaks.push_back(new TruncStName());
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<12>()
{
    BufrReimportTest test("bufr/synop-oddgust.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<13>()
{
    BufrReimportTest test("bufr/synop-oddprec.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    test.wmo_tweaks.push_back(new RemoveSynopWMOOddprec());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<14>()
{
    BufrReimportTest test("bufr/synop-strayvs.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}
template<> template<>
void to::test<15>()
{
    BufrReimportTest test("bufr/synop-sunshine.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
}

template<> template<>
void to::test<16>()
{
    BufrReimportTest test("bufr/synop-gtscosmo.bufr");
    test.wmo_tweaks.push_back(new RemoveSynopWMOOnlyVars());
    run_test(test, do_wmo, "synop");
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
    run_test(test, do_ecmwf, "temp");
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
}
template<> template<>
void to::test<25>()
{
    BufrReimportTest test("bufr/temp-gts2.bufr");
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
}
template<> template<>
void to::test<26>()
{
    BufrReimportTest test("bufr/temp-gts3.bufr");
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
}
template<> template<>
void to::test<27>()
{
    BufrReimportTest test("bufr/temp-gtscosmo.bufr");
    //test.verbose = true;
    test.wmo_tweaks.push_back(new RemoveTempWMOOnlyVars());
    run_test(test, do_wmo, "temp");
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

}

/* vim:set ts=4 sw=4: */
