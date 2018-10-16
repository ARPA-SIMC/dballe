#include "tests.h"
#include "wr_codec.h"
#include "msg.h"
#include "context.h"
#include <wreport/bulletin.h>
#include <wreport/options.h>
#include <cstring>

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

#define IS(field, val) do { \
        WREPORT_TEST_INFO(locinfo); \
        locinfo() << #field; \
        const Var* var = msg.get_##field##_var(); \
        wassert(actual(var).istrue()); \
        wassert(actual(*var) == val); \
    } while (0)
#define IS2(code, lev, tr, val) do { \
        WREPORT_TEST_INFO(locinfo); \
        locinfo() << #code #lev #tr; \
        const Var* var = msg.get(code, lev, tr); \
        wassert(actual(var).istrue()); \
        wassert(actual(*var) == val); \
    } while (0)
#define UN(field) do { \
        const Var* var = msg.get_##field##_var(); \
        if (var != 0) \
            wassert(actual(*var).isunset()); \
    } while (0)


class Tests : public TestCase
{
    using TestCase::TestCase;

    void add_bufr_method(const std::string& fname, std::function<void(const Messages& msgs)> m)
    {
        add_method("prec_" + fname, [=]() {
            std::string pathname = "bufr/" + fname;
            ImporterOptions opts;
            opts.simplified = false;
            Messages msgs = wcallchecked(read_msgs(pathname.c_str(), Encoding::BUFR, opts));
            m(msgs);
        });
    }

    void add_bufr_simplified_method(const std::string& fname, std::function<void(const Messages& msgs)> m)
    {
        add_method("simp_" + fname, [=]() {
            std::string pathname = "bufr/" + fname;
            ImporterOptions opts;
            opts.simplified = true;
            Messages msgs = read_msgs(pathname.c_str(), Encoding::BUFR, opts);
            m(msgs);
        });
    }

    void add_crex_method(const std::string& fname, std::function<void(const Messages& msgs)> m)
    {
        add_method(fname, [=]() {
            std::string pathname = "crex/" + fname;
            Messages msgs = read_msgs(pathname.c_str(), Encoding::CREX);
            m(msgs);
        });
    }

    void register_tests() override
    {
        // Test plain import of all our BUFR test files
        add_method("all_bufr", []() {
            // note: These were blacklisted:
            //      "bufr/obs3-3.1.bufr",
            //      "bufr/obs3-56.2.bufr",
            //      "bufr/test-buoy1.bufr", 
            //      "bufr/test-soil1.bufr", 
            const char** files = dballe::tests::bufr_files;

            for (int i = 0; files[i] != NULL; i++)
            {
                try {
                    Messages msgs = read_msgs(files[i], Encoding::BUFR);
                    wassert(actual(msgs.size()) > 0);
                } catch (std::exception& e) {
                    cerr << "Failing bulletin:";
                    try {
                        BinaryMessage raw = read_rawmsg(files[i], Encoding::BUFR);
                        unique_ptr<Bulletin> bulletin(BufrBulletin::decode(raw.data));
                        bulletin->print(stderr);
                    } catch (std::exception& e1) {
                        cerr << "Cannot display failing bulletin: " << e1.what() << endl;
                    }
                    throw TestFailed(string("[") + files[i] + "] " + e.what());
                }
            }
        });

        // Test plain import of all our CREX test files
        add_method("all_crex", []() {
            const char** files = dballe::tests::crex_files;

            for (int i = 0; files[i] != NULL; i++)
            {
                try {
                    Messages msgs = read_msgs(files[i], Encoding::CREX);
                    wassert(actual(msgs.size()) > 0);
                } catch (std::exception& e) {
                    cerr << "Failing bulletin:";
                    try {
                        BinaryMessage raw = read_rawmsg(files[i], Encoding::CREX);
                        unique_ptr<Bulletin> bulletin(CrexBulletin::decode(raw.data));
                        bulletin->print(stderr);
                    } catch (std::exception& e1) {
                        cerr << "Cannot display failing bulletin: " << e1.what() << endl;
                    }
                    throw TestFailed(string("[") + files[i] + "] " + e.what());
                }
            }
        });

        add_crex_method("test-synop0.crex", [](const Messages& msgs) {
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::SYNOP);

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
        });

        add_bufr_simplified_method("obs0-1.22.bufr", [](const Messages& msgs) {
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::SYNOP);

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
        });

        add_bufr_simplified_method("synop-cloudbelow.bufr", [](const Messages& msgs) {
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::SYNOP);

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
        });

        add_bufr_method("synop-cloudbelow.bufr", [](const Messages& msgs) {
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::SYNOP);

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
        });

        add_bufr_simplified_method("temp-2-255.bufr", [](const Messages& msgs) {
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);

            // No negative pressure layers please
            wassert(actual(msg.find_context(Level(100, -1), Trange::instant())).isfalse());
        });

        add_bufr_simplified_method("synop-longname.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 7u);
            const Msg& msg = Msg::downcast(*msgs[2]);
            wassert(actual(msg.type) == MessageType::SYNOP);

            // Check that the long station name has been correctly truncated on import
            const Var* var = msg.get_st_name_var();
            wassert(actual(var).istrue());
            wassert(actual(string(var->enqc())) == "Budapest Pestszentlorinc-kulteru");
        });

        add_bufr_simplified_method("temp-bad1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        add_bufr_simplified_method("temp-bad2.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        add_bufr_simplified_method("temp-bad3.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        add_bufr_simplified_method("temp-bad4.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        // ECWMF AIREP
        add_bufr_simplified_method("obs4-142.1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::AIREP);
            IS(ident, "ACA872");
        });

        // ECWMF AMDAR
        add_bufr_simplified_method("obs4-144.4.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::AMDAR);
            IS(ident, "EU4444");
        });

        // ECWMF ACARS
        add_bufr_simplified_method("obs4-145.4.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::ACARS);
            IS(ident, "JBNYR3RA");
        });

        // WMO ACARS
        add_bufr_simplified_method("gts-acars1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::ACARS);
            IS(ident, "EU5331");
        });

        // WMO ACARS
        add_bufr_simplified_method("gts-acars2.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::ACARS);
            IS(ident, "FJCYR4RA");
        });

        // WMO ACARS UK
        add_bufr_simplified_method("gts-acars-uk1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            // This contains the same data as an AMDAR and has undefined subtype and
            // localsubtype, so it gets identified as an AMDAR
            wassert(actual(msg.type) == MessageType::AMDAR);
            IS(ident, "EU3375");
        });

        // WMO ACARS US
        add_bufr_simplified_method("gts-acars-us1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::ACARS);
            IS(ident, "FJCYR4RA");
        });

        // BUFR that has a variable that goes out of range when converted to local B
        // table
        add_method("outofrange", []() {
            try
            {
                // Read and interpretate the message
                BinaryMessage raw = read_rawmsg("bufr/interpreted-range.bufr", Encoding::BUFR);
                std::unique_ptr<Importer> importer = Importer::create(Encoding::BUFR);
                Messages msgs = importer->from_binary(raw);
                throw TestFailed("error_domain was not thrown");
            } catch (wreport::error_domain& e) {
                //cerr << e.code() << "--" << e.what() << endl;
            }

            {
                wreport::options::LocalOverride<bool> o(wreport::options::var_silent_domain_errors, true);
                Messages msgs = read_msgs("bufr/interpreted-range.bufr", Encoding::BUFR);
                wassert(actual(msgs.size()) == 1u);
                const Msg& msg = Msg::downcast(*msgs[0]);
                wassert(actual(msg.type) == MessageType::SHIP);
                IS(ident, "DBBC");
            }
        });

        // WMO PILOT, with pressure levels
        add_bufr_simplified_method("pilot-gts2.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::PILOT);
        });

        // WMO PILOT, with pressure levels
        add_bufr_simplified_method("temp-tsig-2.bufr", [](const Messages& msgs) {
            // FIXME: this still fails
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        // WMO pilot pressure
        add_bufr_simplified_method("pilot-gts3.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::PILOT);
        });

        // WMO pilot geopotential
        add_bufr_simplified_method("pilot-gts4.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::PILOT);
        });

        add_bufr_simplified_method("vad.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        // Wind profiler
        add_bufr_simplified_method("temp-windprof1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::TEMP);
        });

        // Precise import
        add_bufr_method("gts-synop-linate.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            const Var* v = msg.get(WR_VAR(0, 12, 101), Level(103, 2000), Trange(3, 0, 43200));
            wassert(actual(v).istrue());
            wassert(actual(v->enqd()) == 284.75);
        });

        // Soil temperature (see https://github.com/ARPA-SIMC/dballe/issues/41 )
        add_bufr_simplified_method("test-soil1.bufr", [](const Messages& msgs) {
            wassert(actual(msgs.size()) == 1u);
            const Msg& msg = Msg::downcast(*msgs[0]);
            wassert(actual(msg.type) == MessageType::SYNOP);
            IS2(WR_VAR(0, 12, 30), Level(106,   50), Trange::instant(), 288.5);
            IS2(WR_VAR(0, 12, 30), Level(106,  100), Trange::instant(), 289.4);
            IS2(WR_VAR(0, 12, 30), Level(106,  200), Trange::instant(), 288.6);
            IS2(WR_VAR(0, 12, 30), Level(106,  500), Trange::instant(), 288.8);
            IS2(WR_VAR(0, 12, 30), Level(106, 1000), Trange::instant(), 288.4);
        });

    }
} test("msg_wr_import");

}
