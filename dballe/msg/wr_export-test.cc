#include "tests.h"
#include "wr_codec.h"
#include "msg.h"
#include "context.h"
#include <wreport/bulletin.h>
#include <wreport/conv.h>
#include <wreport/notes.h>
#include <wreport/utils/string.h>
#include <set>
#include <cstring>

using namespace wreport;
using namespace std;
using namespace dballe;
using namespace dballe::tests;
using namespace dballe::tests::tweaks;

namespace {

using dballe::tests::TestCodec;

class Tests : public TestCase
{
    using TestCase::TestCase;

    void add_testcodec(const char* fname, std::function<void(TestCodec&)> method)
    {
        add_method(fname, [=]() {
            string pathname = "bufr/";
            pathname += fname;
            dballe::tests::TestCodec test(pathname);
            method(test);
        });
    }

    void register_tests() override
    {
        add_method("all_bufr", []() {
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
                    wassert(actual(msgs.size()) > 0);

                    std::unique_ptr<msg::Exporter> exporter;

                    exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
                    unique_ptr<Bulletin> bbulletin = exporter->to_bulletin(msgs);

                    if (bl_crex.find(files[i]) == bl_crex.end())
                    {
                        exporter = msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/);
                        unique_ptr<Bulletin> cbulletin = exporter->to_bulletin(msgs);
                    }
                } catch (std::exception& e) {
                    fails.push_back(string(files[i]) + ": " + e.what());
                }
            }
            if (!fails.empty())
            {
                std::stringstream ss;
                ss << fails.size() << "/" << i << " errors:" << endl;
                ss << str::join("\n", fails.begin(), fails.end());
                throw TestFailed(ss.str());
            }
        });
        add_method("all_crex", []() {
            // Test that plain re-export of all our CREX test files is possible
            const char** files = dballe::tests::crex_files;

            vector<string> fails;
            int i;
            for (i = 0; files[i] != NULL; i++)
            {
                try {
                    Messages msgs = read_msgs(files[i], File::CREX);
                    wassert(actual(msgs.size()) > 0);

                    std::unique_ptr<msg::Exporter> exporter = msg::Exporter::create(File::BUFR/*, const Options& opts=Options()*/);
                    unique_ptr<Bulletin> bbulletin = exporter->to_bulletin(msgs);

                    exporter = msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/);
                    unique_ptr<Bulletin> cbulletin = exporter->to_bulletin(msgs);
                } catch (std::exception& e) {
                    fails.push_back(string(files[i]) + ": " + e.what());
                }
            }
            if (!fails.empty())
            {
                std::stringstream ss;
                ss << fails.size() << "/" << i << " errors:" << endl;
                ss << str::join("\n", fails.begin(), fails.end());
                throw TestFailed(ss.str());
            }
        });
        add_method("reproduce_temp", []() {
            // Export a well known TEMP which used to fail
            Messages msgs = wcallchecked(read_msgs_csv("csv/temp1.csv"));
            wassert(actual(msgs.size()) > 0);

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
            unique_ptr<Bulletin> bbulletin = bufr_exporter->to_bulletin(msgs);

            // Import and check the differences
            {
                std::unique_ptr<msg::Importer> bufr_importer(msg::Importer::create(File::BUFR/*, const Options& opts=Options()*/));
                Messages msgs1 = wcallchecked(bufr_importer->from_bulletin(*bbulletin));
                notes::Collect c(cerr);
                wassert(actual(msgs.diff(msgs1)) == 0);
            }

            // Export to CREX
            std::unique_ptr<msg::Exporter> crex_exporter(msg::Exporter::create(File::CREX/*, const Options& opts=Options()*/));
            unique_ptr<Bulletin> cbulletin = crex_exporter->to_bulletin(msgs);

            // Import and check the differences
            {
                std::unique_ptr<msg::Importer> crex_importer(msg::Importer::create(File::CREX/*, const Options& opts=Options()*/));
                Messages msgs1 = wcallchecked(crex_importer->from_bulletin(*cbulletin));
                notes::Collect c(cerr);
                wassert(actual(msgs.diff(msgs1)) == 0);
            }
        });

        // Re-export tests for old style synops
        add_testcodec("obs0-1.22.bufr", [](TestCodec& test) {
            test.expected_min_vars = 34;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));

            test.after_convert_reimport.add(new StripDatetimeVars());
            wassert(test.run_convert("synop"));
        });
        add_testcodec("obs0-1.11188.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-ecmwf"));
        });
        add_testcodec("obs0-3.504.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-ecmwf"));
        });
        // Re-export test for new style synops
        add_testcodec("synop-cloudbelow.bufr", [](TestCodec& test) {
            test.expected_subsets = 22;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-evapo.bufr", [](TestCodec& test) {
            test.expected_subsets = 14;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 2;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-groundtemp.bufr", [](TestCodec& test) {
            test.expected_subsets = 26;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-longname.bufr", [](TestCodec& test) {
            test.after_reimport_reimport.add(new TruncStName());
            test.after_convert_reimport.add(new TruncStName());
            test.expected_subsets = 7;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-oddgust.bufr", [](TestCodec& test) {
            test.expected_subsets = 26;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 0;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-oddprec.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 0;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-strayvs.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-sunshine.bufr", [](TestCodec& test) {
            test.expected_subsets = 26;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-gtscosmo.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 0;
            test.expected_data_subcategory = 2;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-rad1.bufr", [](TestCodec& test) {
            // Test import/export of GTS synop with radiation information
            test.expected_subsets = 25;
            test.expected_min_vars = 50;

            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-rad2.bufr", [](TestCodec& test) {
            // Test import/export of GTS synop without pressure of standard level
            // FIXME: currently fails
            test.expected_template = "synop-wmo";
            test.expected_min_vars = 50;
            //test.verbose = true;

            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("synop-tchange.bufr", [](TestCodec& test) {
            // Test import/export of GTS synop with temperature change information
            test.expected_min_vars = 50;

            wassert(test.run_reimport());
            wassert(test.run_convert("synop-wmo"));
        });
        add_testcodec("temp-timesig18.bufr", [](TestCodec& test) {
            // FIXME: temp message with data category 0: what do we do with it?
            // FIXME: currently fails
            test.expected_min_vars = 50;
            test.expected_template = "temp-wmo";

            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
        add_testcodec("obs2-101.16.bufr", [](TestCodec& test) {
            // Re-export test for old style temps
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("obs2-102.1.bufr", [](TestCodec& test) {
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("obs2-91.2.bufr", [](TestCodec& test) {
            wassert(test.run_reimport());
            wassert(test.run_convert("pilot-ecmwf"));
        });
        add_testcodec("temp-bad3.bufr", [](TestCodec& test) {
            wassert(test.run_reimport());
            wassert(test.run_convert("temp"));
        });
        add_testcodec("temp-bad5.bufr", [](TestCodec& test) {
            // This has some sounding groups with undefined VSS
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("test-temp1.bufr", [](TestCodec& test) {
            // This has some sounding groups with undefined VSS, and an unusual template
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("C23000.bufr", [](TestCodec& test) {
            // This has an unusual template
            test.after_reimport_import.add(new StripSubstituteAttrs());
            test.after_convert_import.add(new StripSubstituteAttrs());
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("temp-bad1.bufr", [](TestCodec& test) {
            test.after_reimport_reimport.add(new StripQCAttrs());
            test.after_reimport_reimport.add(new StripSubstituteAttrs());
            test.after_reimport_reimport.add(new StripDatetimeVars());
            test.after_convert_import.add(new StripQCAttrs());
            test.after_convert_import.add(new StripSubstituteAttrs());
            test.after_convert_import.add(new StripDatetimeVars());
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 255;
            test.expected_data_subcategory_local = 101;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("temp-bad2.bufr", [](TestCodec& test) {
            test.after_reimport_reimport.add(new StripQCAttrs());
            test.after_reimport_reimport.add(new StripSubstituteAttrs());
            test.after_reimport_reimport.add(new StripDatetimeVars());
            test.after_convert_import.add(new StripQCAttrs());
            test.after_convert_import.add(new StripSubstituteAttrs());
            test.after_convert_import.add(new StripDatetimeVars());
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 255;
            test.expected_data_subcategory_local = 101;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-ecmwf"));
        });
        add_testcodec("temp-gts1.bufr", [](TestCodec& test) {
            // Re-export test for new style temps
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 4;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
        add_testcodec("temp-gts2.bufr", [](TestCodec& test) {
            test.expected_subsets = 6;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 4;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
        add_testcodec("temp-gts3.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 4;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
        add_testcodec("temp-gtscosmo.bufr", [](TestCodec& test) {
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 4;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
#if 0
        Test("new_temp5", [](Fixture& f) {
#warning This is importer with height above ground levels, but exported with pressure levels
            // Another weird template
            BufrReimportTest test("bufr/temp-2-255.bufr");
            run_test(test, do_wmo, "temp");
            //test.output_opts.template_name = "temp-wmo";
            //run_test(test, do_test, "auto");
            //test.output_opts.template_name = "temp-ecmwf";
            //test.clear_tweaks();
            //run_test(test, do_test, "old");
        });
#endif
        add_testcodec("temp-bad4.bufr", [](TestCodec& test) {
            test.after_reimport_reimport.add(new RoundLegacyVars());
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 2;
            test.expected_data_subcategory = 255;
            test.expected_data_subcategory_local = 101;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
#if 0
        Test("new_temp8", [](Fixture& f) {
#warning There is no template that can export these forecast TEMPs except generic
            // generic temp with forecast info and hybrid levels (?)
            BufrReimportTest test("bufr/tempforecast.bufr");
            run_test(test, do_test, "temp-wmo");
            test.clear_tweaks();
            test.tweaks.push_back(new StripQCAttrs());
            run_test(test, do_test, "temp-ecmwf", "temp-wmo");
        });
#endif
        add_testcodec("vad.bufr", [](TestCodec& test) {
            // Test that temp vad subtype is set correctly
            test.expected_subsets = 1;
            test.expected_min_vars = 10;
            test.expected_data_category = 6;
            test.expected_data_subcategory = 1;
            test.expected_data_subcategory_local = 255;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp"));
        });
        add_testcodec("temp-huge.bufr", [](TestCodec& test) {
            // Test correct import/export of a temp with thousands of levels
            test.expected_min_vars = 30000;
            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
        // New style ACARS
#if 0
        Test("new_acars1", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
            //BufrReimportTest test("bufr/gts-acars1.bufr");
            //run_test(test, do_test, "acars-wmo");
        });
        Test("new_acars2", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
            //BufrReimportTest test("bufr/gts-acars2.bufr");
            //run_test(test, do_test, "acars-wmo");
        });
        Test("new_acars3", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
            //BufrReimportTest test("bufr/gts-acars-uk1.bufr");
            //run_test(test, do_test, "acars-wmo");
        });
        Test("new_acars4", [](Fixture& f) {
#warning no documentation on new stile ACARS available yet
            //BufrReimportTest test("bufr/gts-acars-us1.bufr");
            //run_test(test, do_test, "acars-wmo");
        });
#endif
        // New style AMDAR
        add_testcodec("gts-amdar1.bufr", [](TestCodec& test) {
            // FIXME: currently fails
            wassert(test.run_reimport());
            wassert(test.run_convert("amdar-wmo"));
        });
        add_testcodec("gts-amdar2.bufr", [](TestCodec& test) {
            // FIXME: currently fails
            wassert(test.run_reimport());
            wassert(test.run_convert("amdar-wmo"));
        });
        // Re-export to BUFR (simplified, full template autodetect) and see the differences
        add_method("all_simplified", []() {
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
                    wassert(actual(msgs.size()) > 0);

                    // Export
                    unique_ptr<Bulletin> bbulletin = exporter->to_bulletin(msgs);

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
                    wassert(actual(diffs) == 0);
                } catch (std::exception& e) {
                    fails.push_back(string(files[i]) + ": " + e.what());
                }
            }
            if (!fails.empty())
            {
                std::stringstream ss;
                ss << fails.size() << "/" << i << " errors:" << endl;
                ss << str::join("\n", fails.begin(), fails.end());
                throw TestFailed(ss.str());
            }
        });
        add_method("all_precise", []() {
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
                    Messages msgs = read_msgs(files[i], File::BUFR, import_opts);
                    wassert(actual(msgs.size()) > 0);

                    // Export
                    unique_ptr<Bulletin> bbulletin = exporter->to_bulletin(msgs);

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
                    wassert(actual(diffs) == 0);
                } catch (std::exception& e) {
                    fails.push_back(string(files[i]) + ": " + e.what());
                }
            }
            if (!fails.empty())
            {
                std::stringstream ss;
                ss << fails.size() << "/" << i << " errors:" << endl;
                ss << str::join("\n", fails.begin(), fails.end());
                throw TestFailed(ss.str());
            }
        });
        // Old PILOT
        add_method("old_pilot1", []() {
            // Test that pilot subtype is set correctly
            Messages msgs = read_msgs("bufr/obs2-91.2.bufr", File::BUFR);
            msg::Exporter::Options opts;
            opts.template_name = "pilot-wmo";
            unique_ptr<Bulletin> bulletin = test_export_msgs(File::BUFR, msgs, "pilotwmo", opts);
            wassert(actual(bulletin->data_category) == 2);
            wassert(actual(bulletin->data_subcategory) == 1);
            wassert(actual(bulletin->data_subcategory_local) == 255);
        });
        add_testcodec("pilot-gts3.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF pilot with pressure levels
            test.expected_min_vars = 50;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("pilot-wmo"));
        });
        add_testcodec("pilot-gts4.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF pilot with geopotential levels
            test.expected_min_vars = 50;
            test.configure_ecmwf_to_wmo_tweaks();
            test.after_convert_reimport.add(new dballe::tests::tweaks::HeightToGeopotential);
            test.after_convert_reimport.add(new dballe::tests::tweaks::RemoveContext(
                        Level(100, 70000), Trange::instant()));
            test.after_convert_reimport.add(new dballe::tests::tweaks::RemoveContext(
                        Level(100, 85000), Trange::instant()));

            wassert(test.run_reimport());
            wassert(test.run_convert("pilot-wmo"));
        });
        // New style PILOT
        add_testcodec("pilot-ecmwf-geopotential.bufr", [](TestCodec& test) {
            test.configure_ecmwf_to_wmo_tweaks();
            test.after_reimport_reimport.add(new StripVars({ WR_VAR(0, 10, 8) }));
            test.after_convert_reimport.add(new StripVars({ WR_VAR(0, 10, 8) }));
            test.expected_min_vars = 30;
            wassert(test.run_reimport());
            wassert(test.run_convert("pilot-wmo"));
        });
        add_method("new_pilot4", []() {
            // Test for a bug where geopotential levels became pressure levels
            Messages msgs1 = read_msgs("bufr/pilot-ecmwf-geopotential.bufr", File::BUFR);
            wassert(actual(msgs1.size()) == 1);
            Msg& msg1 = Msg::downcast(msgs1[0]);

            // Geopotential levels are converted to height above msl
            const msg::Context* c = msg1.find_context(Level(102, 900), Trange(254, 0, 0));
            wassert(actual(c).istrue());

            // Convert to WMO template
            msg::Exporter::Options output_opts;
            output_opts.template_name = "pilot-wmo";
            //if (verbose) cerr << "Exporting " << output_opts.to_string() << endl;
            std::unique_ptr<Bulletin> bulletin = test_export_msgs(File::BUFR, msgs1, "towmo", output_opts);

            // Import again
            std::unique_ptr<msg::Importer> imp = msg::Importer::create(File::BUFR);
            Messages msgs2 = imp->from_bulletin(*bulletin);
            wassert(actual(msgs2.size()) == 1);
            Msg& msg2 = Msg::downcast(msgs2[0]);

            // Ensure we didn't get pressure levels
            wassert(actual(msg2.find_context(Level(100, 900), Trange(254, 0, 0))).isfalse());
        });
        add_method("new_pilot5", []() {
            // Test for a range error in one specific BUFR
            Messages msgs1 = read_msgs("bufr/temp-2-255.bufr", File::BUFR);
            wassert(actual(msgs1.size()) == 1);

            // Convert to CREX
            msg::Exporter::Options output_opts;
            output_opts.template_name = "temp-wmo";
            std::unique_ptr<Bulletin> bulletin = test_export_msgs(File::CREX, msgs1, "tocrex", output_opts);

            // Import again
            std::unique_ptr<msg::Importer> imp = msg::Importer::create(File::BUFR);
            Messages msgs2 = wcallchecked(imp->from_bulletin(*bulletin));
            wassert(actual(msgs2.size()) == 1);
        });
        // Old SHIP
        add_method("old_ship1", []() {
            // Test that temp ship subtype is set correctly
            Messages msgs = read_msgs("bufr/obs2-102.1.bufr", File::BUFR);
            msg::Exporter::Options opts;
            opts.template_name = "temp-wmo";
            unique_ptr<Bulletin> bulletin = test_export_msgs(File::BUFR, msgs, "tempship", opts);
            wassert(actual(bulletin->data_category) == 2);
            wassert(actual(bulletin->data_subcategory) == 5);
            wassert(actual(bulletin->data_subcategory_local) == 255);
        });
        add_testcodec("ecmwf-ship-1-11.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF synop ship
            test.expected_min_vars = 34;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("ship-wmo"));
        });
        add_testcodec("ecmwf-ship-1-12.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF synop ship record 2
            test.expected_min_vars = 21;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("ship-wmo"));
        });
        add_testcodec("ecmwf-ship-1-13.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF synop ship (auto)
            test.expected_min_vars = 30;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("ship-wmo"));
        });
        add_testcodec("ecmwf-ship-1-14.bufr", [](TestCodec& test) {
            // Test import/export of ECMWF synop ship (auto) record 2
            test.expected_min_vars = 28;
            test.configure_ecmwf_to_wmo_tweaks();

            wassert(test.run_reimport());
            wassert(test.run_convert("ship-wmo"));
        });
        // New SHIP
        add_testcodec("wmo-ship-1.bufr", [](TestCodec& test) {
            // Test import/export of WMO synop ship
            test.expected_min_vars = 50;

            wassert(test.run_reimport());
            wassert(test.run_convert("ship-wmo"));
        });
        // ECMWF <-> WMO
        add_testcodec("ecmwf-amdar1.bufr", [](TestCodec& test) {
            test.expected_min_vars = 21;

            wassert(test.run_reimport());
            wassert(test.run_convert("amdar"));
        });
        add_testcodec("ecmwf-acars1.bufr", [](TestCodec& test) {
            test.expected_min_vars = 21;

            wassert(test.run_reimport());
            wassert(test.run_convert("acars"));
        });
        // Specific issues
        add_testcodec("issue62.bufr", [](TestCodec& test) {
            test.expected_min_vars = 21;

            wassert(test.run_reimport());
            wassert(test.run_convert("temp-wmo"));
        });
    }
} test("msg_wr_export");

}

