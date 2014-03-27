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

#include <dballe/core/test-utils-core.h>
#include "msgapi.h"

using namespace std;
using namespace dballe;

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

    ensure_equals(api.voglioquesto(), 0);
    ensure_equals(api.voglioquesto(), 0);
    ensure_equals(api.voglioquesto(), 0);
    ensure_equals(api.quantesono(), 1);
}

// Test resuming after a broken BUFR
template<> template<>
void to::test<2>()
{
    // Concatenate a broken BUFR with a good one
    std::auto_ptr<Rawmsg> rm1(read_rawmsg("bufr/interpreted-range.bufr", BUFR));
    std::auto_ptr<Rawmsg> rm2(read_rawmsg("bufr/temp-gts1.bufr", BUFR));

    // Broken + good
    {
        string concat = *rm1 + *rm2;
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
        ensure_equals(api.voglioquesto(), 550);
    }

    // Good + broken + good
    {
        string concat = *rm2 + *rm1 + *rm2;
        FILE* out = fopen("test-simple-concat.bufr", "w");
        fwrite(concat.data(), concat.size(), 1, out);
        fclose(out);

        fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

        ensure_equals(api.voglioquesto(), 550);

        try {
            api.voglioquesto();
            ensure(false);
        } catch (std::exception) {
        }

        ensure_equals(api.voglioquesto(), 550);
    }

    // Good + broken + broken + good
    {
        string concat = *rm2 + *rm1 + *rm1 + *rm2;
        FILE* out = fopen("test-simple-concat.bufr", "w");
        fwrite(concat.data(), concat.size(), 1, out);
        fclose(out);

        fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

        ensure_equals(api.voglioquesto(), 550);

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

        ensure_equals(api.voglioquesto(), 550);
    }
}

template<> template<>
void to::test<3>()
{
    // Try reading a file
    std::string fname = tests::datafile("bufr/dbapi-emptymsg.bufr");
    fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

    ensure_equals(api.voglioquesto(), 96);
    // FIXME: this is indistinguishable from an EOF
    ensure_equals(api.voglioquesto(), 0);
    ensure_equals(api.voglioquesto(), 86);
    ensure_equals(api.voglioquesto(), api.missing_int);
}

}

/* vim:set ts=4 sw=4: */
