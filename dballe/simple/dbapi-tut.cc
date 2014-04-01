/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"
#include "db/test-utils-db.h"
#include "dbapi.h"

using namespace std;
using namespace dballe;
using namespace dballe::db;
using namespace wibble::tests;

namespace {

struct dbapi_tests : public dballe::tests::db_test
{
    void populate_variables(fortran::DbAPI& api)
    {
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);

        // Instant temperature, 2 meters above ground
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.prendilo();

        // Instant wind speed, 10 meters above ground
        api.unsetb();
        api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
        api.setd("B11002", 2.4);
        api.prendilo();

        api.unsetall();
    }
};

}

namespace tut {

using namespace dballe::tests;
typedef db_tg<dbapi_tests> tg;
typedef tg::object to;

// Test vars
template<> template<> void to::test<1>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    // Query stations
    api.unsetall();
    ensure_equals(api.quantesono(), 1);
    api.elencamele();
    ensure_equals(api.enqd("lat"), 44.5);
    ensure_equals(api.enqd("lon"), 11.5);

    // Query variables
    api.unsetall();
    ensure_equals(api.voglioquesto(), 2);
    ensure_equals(string(api.dammelo()), "B12101");
    ensure_equals(api.enqd("B12101"), 21.5);
    ensure_equals(string(api.dammelo()), "B11002");
    ensure_equals(api.enqd("lat"), 44.5);
    ensure_equals(api.enqd("lon"), 11.5);
    ensure_equals(api.enqd("B11002"), 2.4);

    // Delete variables
    api.unsetall();
    api.setc("var", "B12101");
    api.dimenticami();
    ensure_equals(api.voglioquesto(), 0);
}

// Check that the context ID we get is correct
template<> template<> void to::test<2>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    int reference_id;

    // Query a variable
    api.setc("var", "B12101");
    ensure_equals(api.voglioquesto(), 1);
    api.dammelo();

    // Store its context info to access attributes of this variable later
    reference_id = api.enqi("context_id");

    // Check that the context id that we get points to the right variable
    api.unsetall();
    api.seti("context_id", reference_id);
    wassert(actual(api.voglioquesto()) == 1);
    wassert(actual(api.dammelo()) == "B12101");
    ensure_equals(api.enqi("l1"), 2000);
}

// Test attrs
template<> template<> void to::test<3>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    int reference_id;

    // Query a variable
    api.setc("var", "B12101");
    ensure_equals(api.voglioquesto(), 1);
    api.dammelo();

    wassert(actual(api.test_get_attr_state()) == fortran::DbAPI::ATTR_DAMMELO);

    // Store its context info to access attributes of this variable later
    reference_id = api.enqi("context_id");

    // It has no attributes
    wassert(actual(api.voglioancora()) ==  0);
    wassert(actual(api.test_get_attr_state()) == fortran::DbAPI::ATTR_DAMMELO);

    // Set one attribute after a dammelo
    api.seti("*B33007", 50);
    api.critica();
    wassert(actual(api.test_get_attr_state()) == fortran::DbAPI::ATTR_DAMMELO);

    // It now has one attribute
    wassert(actual(api.voglioancora()) ==  1);


    // Query a different variable, it has no attributes
    api.setc("var", "B11002");
    ensure_equals(api.voglioquesto(), 1);
    api.dammelo();
    ensure_equals(api.voglioancora(), 0);


    // Query the first variable using its stored reference id
    api.seti("*context_id", reference_id);
    api.setc("*var_related", "B12101");
    wassert(actual(api.test_get_attr_state()) == fortran::DbAPI::ATTR_REFERENCE);
    ensure_equals(api.voglioancora(), 1);
    ensure_equals(api.enqi("*B33007"), 50);

    // Delete all attributes
    api.scusa();
    ensure_equals(api.voglioancora(), 0);
}

// Test attrs prendilo
template<> template<> void to::test<4>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    // Set one attribute after a prendilo
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
    api.settimerange(254, 0, 0);
    api.setd("B10004", 100000.0);
    api.prendilo(); // Pressure at ground level
    api.seti("*B33007", 60);
    api.critica();

    // Query it again
    api.unsetall();
    api.setc("var", "B10004");
    ensure_equals(api.voglioquesto(), 1);
    api.dammelo();
    ensure_equals(api.voglioancora(), 1);
    ensure_equals(api.enqi("*B33007"), 60);
}

// Test prendilo anaid
template<> template<> void to::test<5>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    // Run a prendilo
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
    api.settimerange(254, 0, 0);
    api.setd("B10004", 100000.0);
    api.prendilo(); // Pressure at ground level

    int anaid = api.enqi("*ana_id");
    wassert(actual(anaid) != MISSING_INT);

    // Query it back
    api.unsetall();
    api.seti("ana_id", anaid);
    api.setc("var", "B12101");
    wassert(actual(api.voglioquesto()) == 1);
}

// Test handling of values with undefined leveltype2 and l2
template<> template<> void to::test<6>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
    api.settimerange(254, MISSING_INT, MISSING_INT);
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setd("B12101", 21.5);
    api.prendilo();
    api.unsetall();

    // Query it back
    api.seti("leveltype1", 103);
    wassert(actual(api.voglioquesto()) == 1);

    wassert(actual(api.dammelo()) == "B12101");
    wassert(actual(api.enqi("leveltype1")) == 103);
    wassert(actual(api.enqi("l1")) == 2000);
    wassert(actual(api.enqi("leveltype2")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("l2")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("pindicator")) == 254);
    wassert(actual(api.enqi("p1")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("p2")) == fortran::DbAPI::missing_int);
}

// Test deleting attributes after a dammelo
template<> template<> void to::test<7>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    populate_variables(api);

    // Query all variables and add attributes
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == "B12101");
    api.seti("*B33007", 50);
    api.critica();
    wassert(actual(api.dammelo()) == "B11002");
    api.seti("*B33007", 60);
    api.critica();

    // Query all variables again and check that attributes are there
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == "B12101");
    wassert(actual(api.voglioancora()) == 1);
    wassert(actual(api.enqi("*B33007")) == 50);
    wassert(actual(api.dammelo()) == "B11002");
    wassert(actual(api.voglioancora()) == 1);
    wassert(actual(api.enqi("*B33007")) == 60);

    // Query all variables and delete all attributes
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == "B12101");
    api.scusa();
    wassert(actual(api.dammelo()) == "B11002");
    api.scusa();

    // Query again and check that the attributes are gone
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == "B12101");
    wassert(actual(api.voglioancora()) == 0);
    wassert(actual(api.dammelo()) == "B11002");
    wassert(actual(api.voglioancora()) == 0);

    // The QC attrs record should be cleaned
    wassert(actual(api.enqi("*B33007")) == MISSING_INT);
}

template<> template<> void to::test<8>()
{
    {
        fortran::DbAPI api(*db, "read", "read", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(*db, "write", "read", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(*db, "read", "add", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(*db, "write", "add", "read");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
    }
    {
        fortran::DbAPI api(*db, "write", "write", "read");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
    }
    {
        fortran::DbAPI api(*db, "write", "add", "write");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
    }
    {
        fortran::DbAPI api(*db, "write", "write", "write");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);
    }
}

template<> template<> void to::test<9>()
{
    // 2 messages, 1 subset each
    fortran::DbAPI api(*db, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", BUFR);

    // At the beginning, the DB is empty
    wassert(actual(api.voglioquesto()) == 0);

    // First message
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 88);

    // Second message
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 9);

    // End of messages
    api.remove_all();
    wassert(actual(api.messages_read_next()).isfalse());
    wassert(actual(api.voglioquesto()) == 0);
}

template<> template<> void to::test<10>()
{
    // 1 message, 6 subsets
    fortran::DbAPI api(*db, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/temp-gts2.bufr").c_str(), "r", BUFR);

    // At the beginning, the DB is empty
    wassert(actual(api.voglioquesto()) == 0);

    // 6 subsets
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 193);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 182);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 170);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 184);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 256);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 213);

    // End of messages
    api.remove_all();
    wassert(actual(api.messages_read_next()).isfalse());
    wassert(actual(api.voglioquesto()) == 0);
}

template<> template<> void to::test<11>()
{
    // 2 messages, 2 subsets each
    fortran::DbAPI api(*db, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/db-messages1.bufr").c_str(), "r", BUFR);

    // At the beginning, the DB is empty
    wassert(actual(api.voglioquesto()) == 0);

    // 6 subsets
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 88);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 9);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 193);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 182);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 170);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 184);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 256);
    api.remove_all();
    wassert(actual(api.messages_read_next()).istrue());
    wassert(actual(api.voglioquesto()) == 213);

    // End of messages
    api.remove_all();
    wassert(actual(api.messages_read_next()).isfalse());
    wassert(actual(api.voglioquesto()) == 0);
}

template<> template<> void to::test<12>()
{
    // Write one message
    {
        fortran::DbAPI api(*db, "write", "write", "write");
        api.messages_open_output("test.bufr", "wb", BUFR);

        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);
        // Instant temperature, 2 meters above ground
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.prendilo();
        // Instant wind speed, 10 meters above ground
        api.unsetb();
        api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
        api.setd("B11002", 2.4);
        api.prendilo();

        api.unsetall();
        api.messages_write_next("wmo");
    }

    // Read it back
    {
        fortran::DbAPI api(*db, "write", "write", "write");
        api.messages_open_input("test.bufr", "rb", BUFR);

        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.voglioquesto()) == 2);
        wassert(actual(api.dammelo()) == "B12101");
        wassert(actual(api.enqd("B12101")) == 21.5);
        wassert(actual(api.dammelo()) == "B11002");
        wassert(actual(api.enqd("B11002")) == 2.4);

        wassert(actual(api.messages_read_next()).isfalse());
    }
}

// Reproduce an issue reported by Paolo
template<> template<> void to::test<13>()
{
    // 2 messages, 2 subsets each
    fortran::DbAPI api(*db, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140312.bufr").c_str(), "r", BUFR);
    wassert(actual(api.messages_read_next()) == 1);
    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 3);
    // bug with mem DB: message: "enqi: B00000 (Context ID of the variable) is not defined"
    wassert(actual(api.dammelo()) == "B01194");
    wassert(actual(api.dammelo()) == "B05001");
    wassert(actual(api.dammelo()) == "B06001");
}

// Reproduce an issue reported by Paolo
template<> template<> void to::test<14>()
{
    // 2 messages, 2 subsets each
    fortran::DbAPI api(*db, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140426.bufr").c_str(), "r", BUFR);
    wassert(actual(api.messages_read_next()) == 1);
    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 5);
    wassert(actual(api.dammelo()) == "B01019");
    // Bug: missing variable 000000 in table dballe
    wassert(actual(api.voglioancora()) == 0);
}

// Test attr_reference_id behaviour
template<> template<> void to::test<15>()
{
    fortran::DbAPI api(*db, "write", "write", "write");
    // Initial data
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    // One station variable
    api.setcontextana();
    api.setd("B07030", 100.0);
    api.prendilo();
    // One data variable
    api.settimerange(254, 0, 0);
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
    api.setd("B12101", 21.5);
    api.prendilo();
    api.unsetall();

    // Query the station variable
    api.setcontextana();
    api.setc("var", "B07030");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    // Get its reference id
    wassert(actual(api.enqi("context_id")) == MISSING_INT);
    // Get its attrs (none)
    wassert(actual(api.voglioancora()) == 0);
    // Set an attr
    api.seti("*B33007", 50);
    api.critica();
    // Query the variable again
    api.unsetall();
    api.setcontextana();
    api.setc("var", "B07030");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    // Read its attrs (ok)
    wassert(actual(api.voglioancora()) == 1);
    // Read attrs setting *context_id and *varid: it fails
    api.seti("*context_id", MISSING_INT);
    api.setc("*var_related", "B07030");
    try {
        api.voglioancora();
    } catch (std::exception& e) {
        wassert(actual(e.what()).matches("invalid \\*context_id"));
    }
    // Query the variable again, to delete all attributes
    api.unsetall();
    api.setcontextana();
    api.setc("var", "B07030");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    api.scusa();
    wassert(actual(api.voglioancora()) == 0);
    // Try to delete by *context_id and *varid: it fails
    api.seti("*context_id", MISSING_INT);
    api.setc("*var_related", "B07030");
    try {
        api.scusa();
    } catch (std::exception& e) {
        wassert(actual(e.what()).matches("invalid \\*context_id"));
    }

    // Query the variable
    api.unsetall();
    api.setc("var", "B12101");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    int ref_id = api.enqi("context_id");
    // Save its ref id (ok)
    wassert(actual(ref_id) != MISSING_INT);
    // Get its attrs (none)
    wassert(actual(api.voglioancora()) == 0);
    // Set an attr
    api.seti("*B33007", 50);
    api.critica();
    // Query the variable again
    api.unsetall();
    api.setc("var", "B12101");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    // Read its attrs (ok)
    wassert(actual(api.voglioancora()) == 1);
    // Set *context_id and *varid
    // Read attrs (ok)
    api.seti("*context_id", ref_id);
    api.setc("*var_related", "B12101");
    wassert(actual(api.voglioancora()) == 1);
    // Query the variable again, to delete all attributes
    api.unsetall();
    api.setc("var", "B12101");
    wassert(actual(api.voglioquesto()) == 1);
    api.dammelo();
    api.scusa();
    wassert(actual(api.voglioancora()) == 0);
    // Try to delete by *context_id and *varid: it works
    api.seti("*context_id", ref_id);
    api.setc("*var_related", "B12101");
    api.scusa();
}

// Reproduce a bug when setting attributes
template<> template<> void to::test<16>()
{
    fortran::DbAPI dbapi0(*db, "write", "write", "write");
    dbapi0.scopa();

    // Add a value
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1000000);
    dbapi0.unset("ident");
    dbapi0.seti("mobile", 0);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
    dbapi0.settimerange(4, 3600, 7200);
    dbapi0.seti("B13003", 85);
    dbapi0.prendilo();

    // Add attributes
    dbapi0.setd("*B33192", 30.000000);
    dbapi0.seti("*B33193", 50);
    dbapi0.setd("*B33194", 70.000000);
    dbapi0.setc("*var_related", "B13003");
    dbapi0.critica();

    // Read them back
    dbapi0.unsetall();
    dbapi0.setc("var", "B13003");
    wassert(actual(dbapi0.voglioquesto()) == 1);
    wassert(actual(dbapi0.dammelo()) == "B13003");
    wassert(actual(dbapi0.voglioancora()) == 3);
}

}

namespace {

tut::tg db_tests_dbapi_mem_tg("dbapi_mem", MEM);
#ifdef HAVE_ODBC
tut::tg db_tests_dbapi_v5_tg("dbapi_v5", V5);
tut::tg db_tests_dbapi_v6_tg("dbapi_v6", V6);
#endif

}
