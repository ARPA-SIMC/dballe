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

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

// Common machinery for import -> export -> reimport tests
template<class B>
struct ReimportTest
{
    typedef dballe::tests::MessageTweaker Tweaker;

    string fname;
    File::Encoding type;
    Messages msgs1;
    Messages msgs2;
    unique_ptr<Bulletin> exported;
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

    ReimportTest(const std::string& fname, File::Encoding type=File::BUFR)
        : fname(fname), type(type), do_ecmwf_tweaks(false), do_wmo_tweaks(false),
          do_ignore_context_attrs(false), do_round_geopotential(false), verbose(false)
    {
        ecmwf_tweaks.push_back(new StripQCAttrs());
        ecmwf_tweaks.push_back(new StripDatetimeVars());
        wmo_tweaks.push_back(new RoundLegacyVars());
    }
    ~ReimportTest()
    {
        clear_tweaks();
    }

    void do_test(const dballe::tests::Location& loc, const char* tname1, const char* tname2=NULL)
    {
        if (verbose) cerr << "Running test " << loc.locstr() << endl;

        std::unique_ptr<msg::Importer> importer(msg::Importer::create(type, input_opts));

        // Import
        if (verbose) cerr << "Importing " << fname << " " << input_opts.to_string() << endl;
        msgs1 = inner_read_msgs_opts(fname.c_str(), type, input_opts);
        inner_ensure(msgs1.size() > 0);

        // Run tweaks
        for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
        {
            if (verbose) cerr << "Running tweak " << (*i)->desc() << endl;
            (*i)->tweak(msgs1);
        }
        if (do_ecmwf_tweaks)
            for (typename vector<Tweaker*>::iterator i = ecmwf_tweaks.begin(); i != ecmwf_tweaks.end(); ++i)
            {
                if (verbose) cerr << "Running ecmwf tweak " << (*i)->desc() << endl;
                (*i)->tweak(msgs1);
            }
        if (do_wmo_tweaks)
            for (typename vector<Tweaker*>::iterator i = wmo_tweaks.begin(); i != wmo_tweaks.end(); ++i)
            {
                if (verbose) cerr << "Running wmo tweak " << (*i)->desc() << endl;
                (*i)->tweak(msgs1);
            }

        // Export
        exported.reset(B::create().release());
        try {
            if (tname1 != NULL)
                output_opts.template_name = tname1;
            else
                output_opts.template_name.clear();
            if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
            std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
            exporter->to_bulletin(msgs1, *exported);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *exported);
            dballe::tests::dump("msg1", msgs1);
            throw tut::failure(loc.msg(string("exporting to bulletin (first template): ") + e.what()));
        }

        // Encode
        BinaryMessage rawmsg(type);
        try {
            exported->encode(rawmsg.data);
            //exporter->to_rawmsg(*msgs1, rawmsg);
        } catch (std::exception& e) {
            dballe::tests::dump("bul1", *exported);
            dballe::tests::dump("msg1", msgs1);
            throw tut::failure(loc.msg(string("encoding to rawmsg (first template): ") + e.what()));
        }

        // Import again
        if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
        msgs2.clear();
        try {
            msgs2 = importer->from_binary(rawmsg);
        } catch (std::exception& e) {
            dballe::tests::dump("msg1", msgs1);
            dballe::tests::dump("msg", rawmsg);
            throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
        }

        Messages msgs3;
        if (tname2)
        {
            // Export
            unique_ptr<Bulletin> bulletin(B::create());
            try {
                output_opts.template_name = tname2;
                if (verbose) cerr << "Reexporting " << output_opts.to_string() << endl;
                std::unique_ptr<msg::Exporter> exporter(msg::Exporter::create(type, output_opts));
                exporter->to_bulletin(msgs2, *bulletin);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", msgs1);
                throw tut::failure(loc.msg(string("exporting to bulletin (second template): ") + e.what()));
            }

            // Encode
            rawmsg = BinaryMessage(type);
            try {
                bulletin->encode(rawmsg.data);
                //exporter->to_rawmsg(*msgs1, rawmsg);
            } catch (std::exception& e) {
                dballe::tests::dump("bul2", *bulletin);
                dballe::tests::dump("msg2", msgs1);
                throw tut::failure(loc.msg(string("encoding to rawmsg (second template): ") + e.what()));
            }

            // Import again
            try {
                if (verbose) cerr << "Reimporting " << input_opts.to_string() << endl;
                msgs3 = importer->from_binary(rawmsg);
            } catch (std::exception& e) {
                dballe::tests::dump("msg2", msgs2);
                dballe::tests::dump("raw2", rawmsg);
                throw tut::failure(loc.msg(string("importing from rawmsg (first template): ") + e.what()));
            }
        } else
            msgs3 = move(msgs2);

#if 0
        // Run tweaks
        for (typename vector<Tweaker*>::iterator i = tweaks.begin(); i != tweaks.end(); ++i)
            (*i)->clean_second(*msgs3);
#endif
        if (do_ignore_context_attrs)
        {
            StripContextAttrs sca;
            sca.tweak(msgs1);
        }
        if (do_round_geopotential)
        {
            RoundGeopotential rg;
            rg.tweak(msgs1);
            rg.tweak(msgs3);
        }

        // Compare
        stringstream str;
        notes::Collect c(str);
        int diffs = msgs1.diff(msgs3);
        if (diffs)
        {
            dballe::tests::dump("msg1", msgs1);
            //if (msgs2.get())
                //dballe::tests::dump("msg2", msgs2);
            dballe::tests::dump("msg3", msgs3);
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

std::vector<Test> tests {
    Test("all_bufr", [](Fixture& f) {
        // Test that plain re-export of all our BUFR test files is possible
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
                Messages msgs = read_msgs(files[i], File::BUFR);
                ensure(msgs.size() > 0);

                std::unique_ptr<msg::Exporter> exporter;

                exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
                unique_ptr<Bulletin> bbulletin(BufrBulletin::create());
                exporter->to_bulletin(msgs, *bbulletin);

                if (bl_crex.find(files[i]) == bl_crex.end())
                {
                    exporter = msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/);
                    unique_ptr<Bulletin> cbulletin(CrexBulletin::create());
                    exporter->to_bulletin(msgs, *cbulletin);
                }
            } catch (std::exception& e) {
                fails.push_back(string(files[i]) + ": " + e.what());
            }
        }
        if (!fails.empty())
            throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
    }),
    Test("all_crex", [](Fixture& f) {
        // Test that plain re-export of all our CREX test files is possible
        const char** files = dballe::tests::crex_files;

        vector<string> fails;
        int i;
        for (i = 0; files[i] != NULL; i++)
        {
            try {
                Messages msgs = read_msgs(files[i], File::CREX);
                ensure(msgs.size() > 0);

                std::unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
                unique_ptr<Bulletin> bbulletin(BufrBulletin::create());
                exporter->to_bulletin(msgs, *bbulletin);

                exporter = msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/);
                unique_ptr<Bulletin> cbulletin(CrexBulletin::create());
                exporter->to_bulletin(msgs, *cbulletin);
            } catch (std::exception& e) {
                fails.push_back(string(files[i]) + ": " + e.what());
            }
        }
        if (!fails.empty())
            throw tut::failure(str::fmtf("%zd/%d errors:\n", fails.size(), i) + str::join(fails.begin(), fails.end(), "\n"));
    }),
    Test("reproduce_temp", [](Fixture& f) {
        // Export a well known TEMP which used to fail
        Messages msgs = read_msgs_csv("csv/temp1.csv");
        ensure(msgs.size() > 0);

        // Replace with packed levels because comparison later happens against
        // packed levels
        {
            unique_ptr<Msg> msg(new Msg);
            msg->sounding_pack_levels(Msg::downcast(msgs[0]));
            msgs.clear();
            msgs.append(move(msg));
        }

        // Export to BUFR
        std::unique_ptr<msg::Exporter> bufr_exporter(msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/));
        unique_ptr<Bulletin> bbulletin(BufrBulletin::create());
        bufr_exporter->to_bulletin(msgs, *bbulletin);

        // Import and check the differences
        {
            std::unique_ptr<msg::Importer> bufr_importer(msg::Importer::create(File::BUFR/*, const Options& opts=Options()*/));
            Messages msgs1 = wcallchecked(bufr_importer->from_bulletin(*bbulletin));
            notes::Collect c(cerr);
            ensure_equals(msgs.diff(msgs1), 0);
        }

        // Export to CREX
        std::unique_ptr<msg::Exporter> crex_exporter(msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/));
        unique_ptr<Bulletin> cbulletin(CrexBulletin::create());
        crex_exporter->to_bulletin(msgs, *cbulletin);

        // Import and check the differences
        {
            std::unique_ptr<msg::Importer> crex_importer(msg::Importer::create(File::CREX/*, const Options& opts=Options()*/));
            Messages msgs1 = wcallchecked(crex_importer->from_bulletin(*cbulletin));
            notes::Collect c(cerr);
            ensure_equals(msgs.diff(msgs1), 0);
        }
    }),
    // Re-export tests for old style synops
    Test("old_synop1", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs0-1.22.bufr");
        test.expected_min_vars = 34;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");

        test.after_convert_reimport.add(new StripDatetimeVars());
        TEST_convert(test, "synop");
    }),
    Test("old_synop2", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs0-1.11188.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        TEST_reimport(test);
        TEST_convert(test, "synop-ecmwf");
    }),
    Test("old_synop3", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs0-3.504.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        TEST_reimport(test);
        TEST_convert(test, "synop-ecmwf");
    }),
    // Re-export test for new style synops
    Test("new_synop1", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-cloudbelow.bufr");
        test.expected_subsets = 22;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop2", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-evapo.bufr");
        test.expected_subsets = 14;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 2;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop3", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-groundtemp.bufr");
        test.expected_subsets = 26;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop4", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-longname.bufr");
        test.after_reimport_reimport.add(new TruncStName());
        test.after_convert_reimport.add(new TruncStName());
        test.expected_subsets = 7;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop5", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-oddgust.bufr");
        test.expected_subsets = 26;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 0;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop6", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-oddprec.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 0;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop7", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-strayvs.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop8", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-sunshine.bufr");
        test.expected_subsets = 26;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop9", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/synop-gtscosmo.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 0;
        test.expected_subtype = 2;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop10", [](Fixture& f) {
        // Test import/export of GTS synop with radiation information
        dballe::tests::TestCodec test("bufr/synop-rad1.bufr");
        test.expected_subsets = 25;
        test.expected_min_vars = 50;

        TEST_reimport(test);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop11", [](Fixture& f) {
        // Test import/export of GTS synop without pressure of standard level
        dballe::tests::TestCodec test("bufr/synop-rad2.bufr");
        test.expected_min_vars = 50;
        test.verbose = true;

        wruntest(test.run_reimport);
        wruntest(test.run_convert, "synop-wmo");
    }),
    Test("new_synop12", [](Fixture& f) {
        // Test import/export of GTS synop with temperature change information
        dballe::tests::TestCodec test("bufr/synop-tchange.bufr");
        test.expected_min_vars = 50;

        wruntest(test.run_reimport);
        TEST_convert(test, "synop-wmo");
    }),
    Test("new_synop13", [](Fixture& f) {
        // Test import/export of GTS synop with temperature change information
        dballe::tests::TestCodec test("bufr/temp-timesig18.bufr");
        test.expected_min_vars = 50;

        wruntest(test.run_reimport);
        TEST_convert(test, "synop-wmo");
    }),
    // Re-export test for old style temps
    Test("old_temp1", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs2-101.16.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp-ecmwf");
    }),
    Test("old_temp2", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs2-102.1.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp-ecmwf");
    }),
    Test("old_temp3", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/obs2-91.2.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "pilot-ecmwf");
    }),
    Test("old_temp4", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-bad3.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp");
    }),
    Test("old_temp5", [](Fixture& f) {
        // This has some sounding groups with undefined VSS
        dballe::tests::TestCodec test("bufr/temp-bad5.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp-ecmwf");
    }),
    Test("old_temp6", [](Fixture& f) {
        // This has some sounding groups with undefined VSS, and an unusual template
        dballe::tests::TestCodec test("bufr/test-temp1.bufr");
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp-ecmwf");
    }),
    Test("old_temp7", [](Fixture& f) {
        // This has an unusual template
        dballe::tests::TestCodec test("bufr/C23000.bufr");
        test.after_reimport_import.add(new StripSubstituteAttrs());
        test.after_convert_import.add(new StripSubstituteAttrs());
        wruntest(test.run_reimport);
        wruntest(test.run_convert, "temp-ecmwf");
    }),
    Test("old_temp8", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-bad1.bufr");
        test.after_reimport_reimport.add(new StripQCAttrs());
        test.after_reimport_reimport.add(new StripSubstituteAttrs());
        test.after_reimport_reimport.add(new StripDatetimeVars());
        test.after_convert_import.add(new StripQCAttrs());
        test.after_convert_import.add(new StripSubstituteAttrs());
        test.after_convert_import.add(new StripDatetimeVars());
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 255;
        test.expected_localsubtype = 101;
        TEST_reimport(test);
        TEST_convert(test, "temp-ecmwf");
    }),
    Test("old_temp9", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-bad2.bufr");
        test.after_reimport_reimport.add(new StripQCAttrs());
        test.after_reimport_reimport.add(new StripSubstituteAttrs());
        test.after_reimport_reimport.add(new StripDatetimeVars());
        test.after_convert_import.add(new StripQCAttrs());
        test.after_convert_import.add(new StripSubstituteAttrs());
        test.after_convert_import.add(new StripDatetimeVars());
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 255;
        test.expected_localsubtype = 101;
        TEST_reimport(test);
        TEST_convert(test, "temp-ecmwf");
    }),
    // Re-export test for new style temps
    Test("new_temp1", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-gts1.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 4;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    Test("new_temp2", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-gts2.bufr");
        test.expected_subsets = 6;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 4;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    Test("new_temp3", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-gts3.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 4;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    Test("new_temp4", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-gtscosmo.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 4;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    Test("new_temp5", [](Fixture& f) {
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
    }),
    Test("new_temp7", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/temp-bad4.bufr");
        test.after_reimport_reimport.add(new RoundLegacyVars());
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 2;
        test.expected_subtype = 255;
        test.expected_localsubtype = 101;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    Test("new_temp8", [](Fixture& f) {
#warning There is no template that can export these forecast TEMPs except generic
#if 0
        // generic temp with forecast info and hybrid levels (?)
        BufrReimportTest test("bufr/tempforecast.bufr");
        run_test(test, do_test, "temp-wmo");
        test.clear_tweaks();
        test.tweaks.push_back(new StripQCAttrs());
        run_test(test, do_test, "temp-ecmwf", "temp-wmo");
#endif
    }),
    Test("new_temp9", [](Fixture& f) {
        // Test that temp vad subtype is set correctly
        dballe::tests::TestCodec test("bufr/vad.bufr");
        test.expected_subsets = 1;
        test.expected_min_vars = 10;
        test.expected_type = 6;
        test.expected_subtype = 1;
        test.expected_localsubtype = 255;
        TEST_reimport(test);
        TEST_convert(test, "temp");
    }),
    Test("new_temp10", [](Fixture& f) {
        // Test correct import/export of a temp with thousands of levels
        dballe::tests::TestCodec test("bufr/temp-huge.bufr");
        test.expected_min_vars = 30000;
        TEST_reimport(test);
        TEST_convert(test, "temp-wmo");
    }),
    // New style ACARS
    Test("new_acars1", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
        //BufrReimportTest test("bufr/gts-acars1.bufr");
        //run_test(test, do_test, "acars-wmo");
    }),
    Test("new_acars2", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
        //BufrReimportTest test("bufr/gts-acars2.bufr");
        //run_test(test, do_test, "acars-wmo");
    }),
    Test("new_acars3", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
        //BufrReimportTest test("bufr/gts-acars-uk1.bufr");
        //run_test(test, do_test, "acars-wmo");
    }),
    Test("new_acars4", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
        //BufrReimportTest test("bufr/gts-acars-us1.bufr");
        //run_test(test, do_test, "acars-wmo");
    }),
    // New style AMDAR
    Test("new_amdar1", [](Fixture& f) {
        BufrReimportTest test("bufr/gts-amdar1.bufr");
        run_test(test, do_test, "amdar-wmo");
    }),
    Test("new_amdar2", [](Fixture& f) {
        BufrReimportTest test("bufr/gts-amdar2.bufr");
        run_test(test, do_test, "amdar-wmo");
    }),
    // Re-export to BUFR (simplified, full template autodetect) and see the differences
    Test("all_simplified", [](Fixture& f) {
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
        std::unique_ptr<msg::Exporter> exporter;
        exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
        std::unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR/*, opts*/);

        for (i = 0; files[i] != NULL; i++)
        {
            if (blacklist.find(files[i]) != blacklist.end()) continue;
            try {
                // Import
                Messages msgs = read_msgs(files[i], File::BUFR);
                ensure(msgs.size() > 0);

                // Export
                unique_ptr<Bulletin> bbulletin(BufrBulletin::create());
                exporter->to_bulletin(msgs, *bbulletin);

                // Import again
                Messages msgs1 = importer->from_bulletin(*bbulletin);

                // Compare
                notes::Collect c(cerr);
                int diffs = msgs.diff(msgs1);
                if (diffs)
                {
                    FILE* out1 = fopen("/tmp/msg1.txt", "w");
                    FILE* out2 = fopen("/tmp/msg2.txt", "w");
                    msgs.print(out1);
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
    }),
    Test("all_precise", [](Fixture& f) {
        // Re-export to BUFR (not simplified, full template autodetect) and see the differences
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
        std::unique_ptr<msg::Exporter> exporter;
        exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
        msg::Importer::Options import_opts;
        import_opts.simplified = false;
        std::unique_ptr<msg::Importer> importer = msg::Importer::create(File::BUFR, import_opts);

        for (i = 0; files[i] != NULL; i++)
        {
            if (blacklist.find(files[i]) != blacklist.end()) continue;
            try {
                // Import
                Messages msgs = read_msgs_opts(files[i], File::BUFR, import_opts);
                ensure(msgs.size() > 0);

                // Export
                unique_ptr<Bulletin> bbulletin(BufrBulletin::create());
                exporter->to_bulletin(msgs, *bbulletin);

                // Import again
                Messages msgs1 = importer->from_bulletin(*bbulletin);

                // Compare
                stringstream str;
                notes::Collect c(str);
                int diffs = msgs.diff(msgs1);
                if (diffs)
                {
                    string tag = str::basename(files[i]);
                    dballe::tests::dump("dballe-orig-" + tag, msgs, "original message");
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
    }),
    // Old PILOT
    Test("old_pilot1", [](Fixture& f) {
        // Test that pilot subtype is set correctly
        Messages msgs = read_msgs("bufr/obs2-91.2.bufr", File::BUFR);
        unique_ptr<Bulletin> bulletin(BufrBulletin::create());
        msg::Exporter::Options opts;
        opts.template_name = "pilot-wmo";
        test_export_msgs(msgs, *bulletin, "pilotwmo", opts);
        ensure_equals(bulletin->type, 2);
        ensure_equals(bulletin->subtype, 1);
        ensure_equals(bulletin->localsubtype, 255);
    }),
    Test("old_pilot2", [](Fixture& f) {
        // Test import/export of ECMWF pilot with pressure levels
        dballe::tests::TestCodec test("bufr/pilot-gts3.bufr");
        test.expected_min_vars = 50;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "pilot-wmo");
    }),
    Test("old_pilot3", [](Fixture& f) {
        // Test import/export of ECMWF pilot with geopotential levels
        dballe::tests::TestCodec test("bufr/pilot-gts4.bufr");
        test.expected_min_vars = 50;
        test.configure_ecmwf_to_wmo_tweaks();
        test.after_convert_reimport.add(new dballe::tests::tweaks::HeightToGeopotential);
        test.after_convert_reimport.add(new dballe::tests::tweaks::RemoveContext(
                    Level(100, 70000), Trange::instant()));
        test.after_convert_reimport.add(new dballe::tests::tweaks::RemoveContext(
                    Level(100, 85000), Trange::instant()));

        TEST_reimport(test);
        TEST_convert(test, "pilot-wmo");
    }),
    // New style PILOT
    Test("new_pilot1", [](Fixture& f) {
        BufrReimportTest test("bufr/pilot-gts1.bufr");
        run_test(test, do_test, "pilot-wmo");

        ensure_equals(test.exported->type, 2);
        ensure_equals(test.exported->subtype, 1);
        ensure_equals(test.exported->localsubtype, 255);
    }),
    Test("new_pilot2", [](Fixture& f) {
        BufrReimportTest test("bufr/pilot-gts1.bufr");
        run_test(test, do_test, "pilot-wmo");

        ensure_equals(test.exported->type, 2);
        ensure_equals(test.exported->subtype, 1);
        ensure_equals(test.exported->localsubtype, 255);
    }),
    Test("new_pilot3", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/pilot-ecmwf-geopotential.bufr");
        test.configure_ecmwf_to_wmo_tweaks();
        test.after_reimport_reimport.add(new StripVars({ WR_VAR(0, 10, 8) }));
        test.after_convert_reimport.add(new StripVars({ WR_VAR(0, 10, 8) }));
        test.expected_min_vars = 30;
        TEST_reimport(test);
        TEST_convert(test, "pilot-wmo");
    }),
    Test("new_pilot4", [](Fixture& f) {
        // Test for a bug where geopotential levels became pressure levels
        Messages msgs1 = read_msgs("bufr/pilot-ecmwf-geopotential.bufr", File::BUFR);
        ensure_equals(msgs1.size(), 1);
        Msg& msg1 = Msg::downcast(msgs1[0]);

        // Geopotential levels are converted to height above msl
        const msg::Context* c = msg1.find_context(Level(102, 900), Trange(254, 0, 0));
        ensure(c != NULL);

        // Convert to WMO template
        msg::Exporter::Options output_opts;
        output_opts.template_name = "pilot-wmo";
        //if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
        std::unique_ptr<BufrBulletin> bulletin = BufrBulletin::create();
        test_export_msgs(msgs1, *bulletin, "towmo", output_opts);

        // Import again
        std::unique_ptr<msg::Importer> imp = msg::Importer::create(File::BUFR);
        Messages msgs2 = imp->from_bulletin(*bulletin);
        ensure_equals(msgs2.size(), 1);
        Msg& msg2 = Msg::downcast(msgs2[0]);

        // Ensure we didn't get pressure levels
        ensure(msg2.find_context(Level(100, 900), Trange(254, 0, 0)) == NULL);
    }),
    Test("new_pilot5", [](Fixture& f) {
        // Test for a range error in one specific BUFR
        Messages msgs1 = read_msgs("bufr/temp-2-255.bufr", File::BUFR);
        ensure_equals(msgs1.size(), 1);
        Msg& msg1 = Msg::downcast(msgs1[0]);

        // Convert to CREX
        msg::Exporter::Options output_opts;
        output_opts.template_name = "temp-wmo";
        std::unique_ptr<CrexBulletin> bulletin = CrexBulletin::create();
        test_export_msgs(msgs1, *bulletin, "tocrex", output_opts);

        // Import again
        std::unique_ptr<msg::Importer> imp = msg::Importer::create(File::BUFR);
        Messages msgs2 = wcallchecked(imp->from_bulletin(*bulletin));
        ensure_equals(msgs2.size(), 1);
        Msg& msg2 = Msg::downcast(msgs2[0]);
    }),
    // Old SHIP
    Test("old_ship1", [](Fixture& f) {
        // Test that temp ship subtype is set correctly
        Messages msgs = read_msgs("bufr/obs2-102.1.bufr", File::BUFR);
        unique_ptr<Bulletin> bulletin(BufrBulletin::create());
        msg::Exporter::Options opts;
        opts.template_name = "temp-wmo";
        test_export_msgs(msgs, *bulletin, "tempship", opts);
        ensure_equals(bulletin->type, 2);
        ensure_equals(bulletin->subtype, 5);
        ensure_equals(bulletin->localsubtype, 255);
    }),
    Test("old_ship2", [](Fixture& f) {
        // Test import/export of ECMWF synop ship
        dballe::tests::TestCodec test("bufr/ecmwf-ship-1-11.bufr");
        test.expected_min_vars = 34;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "ship-wmo");
    }),
    Test("old_ship3", [](Fixture& f) {
        // Test import/export of ECMWF synop ship record 2
        dballe::tests::TestCodec test("bufr/ecmwf-ship-1-12.bufr");
        test.expected_min_vars = 21;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "ship-wmo");
    }),
    Test("old_ship4", [](Fixture& f) {
        // Test import/export of ECMWF synop ship (auto)
        dballe::tests::TestCodec test("bufr/ecmwf-ship-1-13.bufr");
        test.expected_min_vars = 30;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "ship-wmo");
    }),
    Test("old_ship5", [](Fixture& f) {
        // Test import/export of ECMWF synop ship (auto) record 2
        dballe::tests::TestCodec test("bufr/ecmwf-ship-1-14.bufr");
        test.expected_min_vars = 28;
        test.configure_ecmwf_to_wmo_tweaks();

        TEST_reimport(test);
        TEST_convert(test, "ship-wmo");
    }),
    // New SHIP
    Test("new_ship1", [](Fixture& f) {
        // Test import/export of WMO synop ship
        dballe::tests::TestCodec test("bufr/wmo-ship-1.bufr");
        test.expected_min_vars = 50;

        TEST_reimport(test);
        TEST_convert(test, "ship-wmo");
    }),
    // ECMWF <-> WMO
    Test("ecmwf_wmo1", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/ecmwf-amdar1.bufr");
        test.expected_min_vars = 21;

        TEST_reimport(test);
        TEST_convert(test, "amdar");
    }),
    Test("ecmwf_wmo2", [](Fixture& f) {
        dballe::tests::TestCodec test("bufr/ecmwf-acars1.bufr");
        test.expected_min_vars = 21;

        TEST_reimport(test);
        TEST_convert(test, "acars");
    }),
};

test_group tg("msg_wr_export", tests);

}

