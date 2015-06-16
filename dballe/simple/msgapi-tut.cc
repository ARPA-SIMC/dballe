/*
 * Copyright (C) 2010--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/core/tests.h>
#include "msgapi.h"

using namespace std;
using namespace dballe;
using namespace wibble::tests;

namespace tut {

struct msgapi_shar
{
    msgapi_shar()
    {
    }

    ~msgapi_shar()
    {
    }
};
TESTGRP(msgapi);

template<> template<>
void to::test<1>()
{
    // Open test file
    std::string fname = tests::datafile("bufr/simple-generic-group.bufr");
    fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

    ensure_equals(api.voglioquesto(), 4);
    ensure_equals(api.voglioquesto(), 4);
    ensure_equals(api.voglioquesto(), 4);
    ensure_equals(api.quantesono(), 1);
}

// Test resuming after a broken BUFR
template<> template<>
void to::test<2>()
{
    // Concatenate a broken BUFR with a good one
    BinaryMessage rm1(read_rawmsg("bufr/interpreted-range.bufr", File::BUFR));
    BinaryMessage rm2(read_rawmsg("bufr/temp-gts1.bufr", File::BUFR));

    // Broken + good
    {
        string concat = rm1.data + rm2.data;
        FILE* out = fopen("test-simple-concat.bufr", "w");
        fwrite(concat.data(), concat.size(), 1, out);
        fclose(out);

        fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

        // The first one fails
        try {
            api.voglioquesto();
            ensure(false);
        } catch (std::exception) {
        }

        // The second one should be read
        ensure_equals(api.voglioquesto(), 555);
    }

    // Good + broken + good
    {
        string concat = rm2.data + rm1.data + rm2.data;
        FILE* out = fopen("test-simple-concat.bufr", "w");
        fwrite(concat.data(), concat.size(), 1, out);
        fclose(out);

        fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

        ensure_equals(api.voglioquesto(), 555);

        try {
            api.voglioquesto();
            ensure(false);
        } catch (std::exception) {
        }

        ensure_equals(api.voglioquesto(), 555);
    }

    // Good + broken + broken + good
    {
        string concat = rm2.data + rm1.data + rm1.data + rm2.data;
        FILE* out = fopen("test-simple-concat.bufr", "w");
        fwrite(concat.data(), concat.size(), 1, out);
        fclose(out);

        fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

        ensure_equals(api.voglioquesto(), 555);

        try {
            api.voglioquesto();
            ensure(false);
        } catch (std::exception) {
        }

        try {
            api.voglioquesto();
            ensure(false);
        } catch (std::exception) {
        }

        ensure_equals(api.voglioquesto(), 555);
    }
}

template<> template<>
void to::test<3>()
{
    // Try reading a file
    std::string fname = tests::datafile("bufr/dbapi-emptymsg.bufr");
    fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

    ensure_equals(api.voglioquesto(), 99);
    ensure_equals(api.voglioquesto(), 0);
    ensure_equals(api.voglioquesto(), 90);
    ensure_equals(api.voglioquesto(), api.missing_int);
}

template<> template<>
void to::test<4>()
{
    // Try reading a file
    std::string fname = tests::datafile("bufr/synop-bug20140401.bufr");
    fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 25);
    wassert(actual(api.dammelo()) == "B13011");
    wassert(actual(api.dammelo()) == "B10060");
    wassert(actual(api.dammelo()) == "B10063");
    wassert(actual(api.dammelo()) == "B20004");
    wassert(actual(api.dammelo()) == "B20005");
    wassert(actual(api.dammelo()) == "B10004");
    wassert(actual(api.dammelo()) == "B20001");
    wassert(actual(api.dammelo()) == "B20003");
    wassert(actual(api.dammelo()) == "B10051");
    wassert(actual(api.dammelo()) == "B12101"); // 10
    wassert(actual(api.dammelo()) == "B12103");
    wassert(actual(api.dammelo()) == "B11001");
    wassert(actual(api.dammelo()) == "B11002");
    wassert(actual(api.dammelo()) == "B08002");
    wassert(actual(api.dammelo()) == "B20012");
    wassert(actual(api.dammelo()) == "B20012");
    wassert(actual(api.dammelo()) == "B20012");
    wassert(actual(api.dammelo()) == "B08002");
    wassert(actual(api.dammelo()) == "B08002");
    wassert(actual(api.dammelo()) == "B08002"); // 20
    wassert(actual(api.dammelo()) == "B20010");
    wassert(actual(api.dammelo()) == "B01001");
    wassert(actual(api.dammelo()) == "B01002");
    wassert(actual(api.dammelo()) == "B02001");
//    wassert(actual(api.dammelo()) == "B04001");
//    wassert(actual(api.dammelo()) == "B04002");
//    wassert(actual(api.dammelo()) == "B04003");
//    wassert(actual(api.dammelo()) == "B04004");
//    wassert(actual(api.dammelo()) == "B04005");
//    wassert(actual(api.dammelo()) == "B05001"); // 30
//    wassert(actual(api.dammelo()) == "B06001");
    wassert(actual(api.dammelo()) == "B07030");
    wassert(actual(api.voglioquesto()) == MISSING_INT);
}

}

/* vim:set ts=4 sw=4: */
