#include "dballe/core/tests.h"
#include "msgapi.h"
#include <wreport/utils/sys.h>

using namespace std;
using namespace dballe;
using namespace dballe::fortran;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} newtg("fortran_msgapi");

void Tests::register_tests()
{
    add_method("open", []() {
        // Open test file
        std::string fname = tests::datafile("bufr/simple-generic-group.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        wassert(actual(api.query_data()) == 4);
        wassert(actual(api.query_data()) == 4);
        wassert(actual(api.query_data()) == 4);
        wassert(actual(api.query_stations()) == 1);
    });
    add_method("resume", []() {
        // Test resuming after a broken BUFR

        // Concatenate a broken BUFR with a good one
        BinaryMessage rm1(
            read_rawmsg("bufr/interpreted-range.bufr", Encoding::BUFR));
        BinaryMessage rm2(read_rawmsg("bufr/temp-gts1.bufr", Encoding::BUFR));

        // Broken + good
        {
            string concat = rm1.data + rm2.data;
            FILE* out     = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            // The first one fails
            wassert(actual_function([&]() { api.query_data(); }).throws(""));

            // The second one should be read
            wassert(actual(api.query_data()) == 555);
        }

        // Good + broken + good
        {
            string concat = rm2.data + rm1.data + rm2.data;
            FILE* out     = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            wassert(actual(api.query_data()) == 555);
            wassert(actual_function([&]() { api.query_data(); }).throws(""));
            wassert(actual(api.query_data()) == 555);
        }

        // Good + broken + broken + good
        {
            string concat = rm2.data + rm1.data + rm1.data + rm2.data;
            FILE* out     = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            wassert(actual(api.query_data()) == 555);

            wassert(actual_function([&]() { api.query_data(); }).throws(""));
            wassert(actual_function([&]() { api.query_data(); }).throws(""));
            wassert(actual(api.query_data()) == 555);
        }
    });
    add_method("read", []() {
        // Try reading a file
        std::string fname = tests::datafile("bufr/dbapi-emptymsg.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        wassert(actual(api.query_data()) == 99);
        wassert(actual(api.query_data()) == 0);
        wassert(actual(api.query_data()) == 90);
        wassert(actual(api.query_data()) == api.missing_int);
    });
    add_method("missing", []() {
        // Try reading 'missing' values
        std::string fname = tests::datafile("bufr/temp-bad5.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        wassert(actual(api.enqb("latmin")) == API::missing_byte);
        // wassert(actual(api.enqc("latmin")) == API::missing_byte);
        wassert(actual(api.enqi("latmin")) == API::missing_int);
        wassert(actual(api.enqr("latmin")) == API::missing_float);
        wassert(actual(api.enqd("latmin")) == API::missing_double);

        wassert(actual(api.enqb("B05002")) == API::missing_byte);
        // wassert(actual(api.enqc("B05002")) == API::missing_byte);
        wassert(actual(api.enqi("B05002")) == API::missing_int);
        wassert(actual(api.enqr("B05002")) == API::missing_float);
        wassert(actual(api.enqd("B05002")) == API::missing_double);

        api.unsetall();

        for (unsigned msgi = 0;; ++msgi)
        {
            WREPORT_TEST_INFO(msgloop);
            msgloop() << "Message " << msgi;

            int count = wcallchecked(api.query_data());
            if (count == API::missing_int)
                break;
            wassert(actual(count) > 0);

            for (unsigned i = 0; i < (unsigned)count; ++i)
            {
                msgloop() << "Message " << msgi << " var " << i;
                wassert(api.next_data());
            }
        }
    });
    add_method("create", []() {
        using namespace wreport;
        std::filesystem::remove("tmp.bufr");
        {
            MsgAPI msgapi0("tmp.bufr", "w", "BUFR");
            msgapi0.set_station_context();
            msgapi0.setc("rep_memo", "temp");
            msgapi0.setd("lat", 45.027700);
            msgapi0.setd("lon", 9.666700);
            msgapi0.seti("mobile", 0);
            msgapi0.seti("block", 0);
            msgapi0.seti("station", 101);
            wassert(msgapi0.insert_data());

            msgapi0.setlevel(1, 0, API::missing_int, API::missing_int);
            msgapi0.settimerange(254, 0, 0);
            msgapi0.setdate(2018, 6, 1, 0, 0, 0);
            msgapi0.setd("B12101", 25.5);
            wassert(msgapi0.insert_data());

            msgapi0.setc("query", "message");
            wassert(msgapi0.insert_data());
        }

        wassert(actual_file("tmp.bufr").contents_startwith("BUFR"));
        // error: no year information found in message to import
    });
    add_method("issue46", []() {
        using namespace wreport;
        std::filesystem::remove("tmp.bufr");
        {
            MsgAPI msgapi0("tmp.bufr", "w", "BUFR");
            msgapi0.set_station_context();
            msgapi0.setc("rep_memo", "temp");
            msgapi0.setd("lat", 45.027700);
            msgapi0.setd("lon", 9.666700);
            msgapi0.seti("mobile", 0);
            msgapi0.seti("block", 0);
            msgapi0.seti("station", 101);
            wassert(msgapi0.insert_data());
        }
        // error: no year information found in message to import
    });

    add_method("iterate", []() {
        std::string fname = tests::datafile("bufr/gts-acars-uk1.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        wassert(actual(api.query_data()) == 11);
        // wassert(actual(api.next_data()) == WR_VAR(0, 4, 1));
        // 004001 YEAR(YEAR): 2009
        // 004002 MONTH(MONTH): 2
        // 004003 DAY(DAY): 24
        // 004004 HOUR(HOUR): 11
        // 004005 MINUTE(MINUTE): 31
        // 005001 LATITUDE (HIGH ACCURACY)(DEGREE): 48.90500
        // 006001 LONGITUDE (HIGH ACCURACY)(DEGREE): 10.63667
        wassert(actual(api.next_data()) == WR_VAR(0, 1, 6));
        wassert(actual(api.next_data()) == WR_VAR(0, 2, 61));
        wassert(actual(api.next_data()) == WR_VAR(0, 2, 62));
        wassert(actual(api.next_data()) == WR_VAR(0, 2, 64));
        wassert(actual(api.next_data()) == WR_VAR(0, 7, 30));
        wassert(actual(api.next_data()) == WR_VAR(0, 8, 4));
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 1));
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(api.next_data()) == WR_VAR(0, 13, 2));
        // 001011 SHIP OR MOBILE LAND STATION IDENTIFIER(CCITTIA5): EU3375
        wassert(actual(api.next_data()) == WR_VAR(0, 1, 11));
        wassert(actual(api.next_data()) == 0);
        // Level 102,6260000,-,-, tr 254,0,0
        // 001006 AIRCRAFT FLIGHT NUMBER(CCITTIA5): LH968
        // 002061 AIRCRAFT NAVIGATIONAL SYSTEM(CODE TABLE): 0
        // 002062 TYPE OF AIRCRAFT DATA RELAY SYSTEM(CODE TABLE): 3
        // 002064 AIRCRAFT ROLL ANGLE QUALITY(CODE TABLE): 0
        // 007030 HEIGHT OF STATION GROUND ABOVE MEAN SEA LEVEL (SEE NOTE 3)(M):
        // 6260.0 008004 PHASE OF AIRCRAFT FLIGHT(CODE TABLE): 3 011001 WIND
        // DIRECTION(DEGREE TRUE): 33 011002 WIND SPEED(M/S): 33.4 012101
        // TEMPERATURE/DRY-BULB TEMPERATURE(K): 240.00 013002 MIXING
        // RATIO(KG/KG): 0.00000

        wassert(actual(api.query_data()) == api.missing_int);
    });

    add_method("message_ordering", [] {
        using namespace wreport;
        using namespace dballe::fortran;
        std::filesystem::remove("dballe_test.bufr");

        // Check the ordering of results when iterating message contents

        {
            MsgAPI msgapi1("dballe_test.bufr", "w", "bufr");
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1000000);
            msgapi1.setc("rep_memo", "generic");
            msgapi1.setdate(2014, 1, 6, 18, 0, 0);
            msgapi1.setlevel(105, 2000, 2147483647, 2147483647);
            msgapi1.settimerange(4, 3600, 7200);
            msgapi1.seti("B13003", 85);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setd("*B33192", 30.000000);
            msgapi1.seti("*B33193", 50);
            msgapi1.setd("*B33194", 70.000000);
            msgapi1.insert_attributes();
            msgapi1.seti("B12101", 27315);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setd("*B33192", 30.000000);
            msgapi1.seti("*B33193", 50);
            msgapi1.insert_attributes();
            msgapi1.setc("query", "message");
            msgapi1.unsetb();
            msgapi1.insert_data();

            msgapi1.unsetall();
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1000000);
            msgapi1.unset("ident");
            msgapi1.unset("mobile");
            msgapi1.setc("rep_memo", "generic");
            msgapi1.set_station_context();
            msgapi1.seti("B07030", 223);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setc("B01019", "My beautifull station");
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setc("query", "message");
            msgapi1.unsetb();
            msgapi1.insert_data();

            msgapi1.unsetall();
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1000000);
            msgapi1.unset("ident");
            msgapi1.unset("mobile");
            msgapi1.setc("rep_memo", "generic");
            msgapi1.set_station_context();
            msgapi1.seti("B07030", 223);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setc("B01019", "My beautifull station");
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.setc("query", "message");
            msgapi1.unsetb();
            msgapi1.insert_data();

            msgapi1.unsetall();
            msgapi1.setlevel(105, 2000, 2147483647, 2147483647);
            msgapi1.settimerange(4, 3600, 7200);
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1100000);
            msgapi1.setc("rep_memo", "generic");
            msgapi1.setdate(2014, 1, 6, 18, 0, 0);
            msgapi1.setd("B12102", 265.329987);
            msgapi1.setd("B12101", 273.149994);
            msgapi1.insert_data();
            msgapi1.setc("query", "message");
            msgapi1.unsetb();
            msgapi1.insert_data();

            msgapi1.unsetall();
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1200000);
            msgapi1.setc("rep_memo", "generic");
            msgapi1.setdate(2014, 1, 6, 18, 0, 0);
            msgapi1.setlevel(105, 2000, 2147483647, 2147483647);
            msgapi1.settimerange(4, 3600, 7200);
            msgapi1.seti("B12102", 26312);
            msgapi1.setd("B12101", 273.149994);
            msgapi1.insert_data();
            msgapi1.setc("query", "message");
            msgapi1.unsetb();
            msgapi1.insert_data();

            msgapi1.unsetall();
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1300000);
            msgapi1.setc("rep_memo", "generic");
            msgapi1.setdate(2014, 1, 6, 18, 0, 0);
            msgapi1.setlevel(105, 2000, 2147483647, 2147483647);
            msgapi1.settimerange(4, 3600, 7200);
            msgapi1.seti("B12102", 26312);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.seti("*B33192", 30);
            msgapi1.setc("*B33193", "70");
            msgapi1.setd("*B33194", 50.000000);
            msgapi1.insert_attributes();
            msgapi1.setd("B12101", 273.149994);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.seti("lat", 4500000);
            msgapi1.seti("lon", 1300000);
            msgapi1.setc("rep_memo", "generic");
            msgapi1.set_station_context();
            msgapi1.seti("B12102", 26312);
            msgapi1.insert_data();
            msgapi1.unsetb();
            msgapi1.seti("*B33192", 30);
            msgapi1.setc("*B33193", "70");
            msgapi1.setd("*B33194", 50.000000);
            msgapi1.insert_attributes();
            msgapi1.setd("B12101", 273.149994);
            msgapi1.insert_data();
            msgapi1.unsetb();
            // msgapi1 not used anymore
        }

        {
            MsgAPI msgapi4("dballe_test.bufr", "r", "bufr");
            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 3);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(msgapi4.query_attributes()) == 2);
            wassert(actual(msgapi4.next_attribute()) == "*B33192");
            wassert(actual(msgapi4.next_attribute()) == "*B33193");
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 13, 3));
            wassert(actual(msgapi4.query_attributes()) == 3);
            wassert(actual(msgapi4.next_attribute()) == "*B33192");
            wassert(actual(msgapi4.next_attribute()) == "*B33193");
            wassert(actual(msgapi4.next_attribute()) == "*B33194");
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 3);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 19));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 7, 30));
            wassert(actual(msgapi4.query_attributes()) == 0);

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 3);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 19));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 7, 30));
            wassert(actual(msgapi4.query_attributes()) == 0);

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 3);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 102));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 3);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 102));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == 5);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 102));
            wassert(actual(msgapi4.query_attributes()) == 3);
            wassert(actual(msgapi4.next_attribute()) == "*B33192");
            wassert(actual(msgapi4.next_attribute()) == "*B33193");
            wassert(actual(msgapi4.next_attribute()) == "*B33194");
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 1, 194));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(msgapi4.query_attributes()) == 0);
            wassert(actual(msgapi4.next_data()) == WR_VAR(0, 12, 102));
            wassert(actual(msgapi4.query_attributes()) == 3);
            wassert(actual(msgapi4.next_attribute()) == "*B33192");
            wassert(actual(msgapi4.next_attribute()) == "*B33193");
            wassert(actual(msgapi4.next_attribute()) == "*B33194");

            msgapi4.unsetall();
            wassert(actual(msgapi4.query_data()) == API::missing_int);
        }
    });
}

} // namespace
