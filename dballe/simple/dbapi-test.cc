#include "config.h"
#include "db/tests.h"
#include "dbapi.h"
#include "config.h"
#include "msgapi.h"
#include <wreport/utils/sys.h>
#include <sys/fcntl.h>
#include <unistd.h>

using namespace std;
using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;

namespace {

struct TempBind
{
    int fd;
    int old_val;

    TempBind(int fd, int new_fd)
        : fd(fd)
    {
        // Connect stdin to an input file
        old_val = dup(fd);
        wassert(actual(old_val) != -1);
        wassert(actual(dup2(new_fd, fd)) != -1);
    }

    ~TempBind()
    {
        wassert(actual(dup2(old_val, fd)) != -1);
    }
};


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

class Tests : public FixtureTestCase<DBFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("query_basic", [](Fixture& f) {
            // Test vars
            fortran::DbAPI api(*f.db, "write", "write", "write");
            populate_variables(api);

            // Query stations
            api.unsetall();
            wassert(actual(api.quantesono()) == 1);
            api.elencamele();
            wassert(actual(api.enqd("lat")) == 44.5);
            wassert(actual(api.enqd("lon")) == 11.5);

            // Query variables
            api.unsetall();
            wassert(actual(api.voglioquesto()) == 2);
            wassert(actual(string(api.dammelo())) == "B12101");
            wassert(actual(api.enqd("B12101")) == 21.5);
            wassert(actual(string(api.dammelo())) == "B11002");
            wassert(actual(api.enqd("lat")) == 44.5);
            wassert(actual(api.enqd("lon")) == 11.5);
            wassert(actual(api.enqd("B11002")) == 2.4);

            // Delete variables
            api.unsetall();
            api.setc("var", "B12101");
            api.dimenticami();
            wassert(actual(api.voglioquesto()) == 0);
        });
        add_method("query_attrs", [](Fixture& f) {
            // Test attrs
            fortran::DbAPI api(*f.db, "write", "write", "write");
            populate_variables(api);

            int reference_id;

            // Query a variable
            api.setc("var", "B12101");
            wassert(actual(api.voglioquesto()) == 1);
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
            wassert(actual(api.voglioquesto()) == 1);
            api.dammelo();
            wassert(actual(api.voglioancora()) == 0);


            // Query the first variable using its stored reference id
            api.seti("*context_id", reference_id);
            api.setc("*var_related", "B12101");
            wassert(actual(api.test_get_attr_state()) == fortran::DbAPI::ATTR_REFERENCE);
            wassert(actual(api.voglioancora()) == 1);
            wassert(actual(api.enqi("*B33007")) == 50);

            // Delete all attributes
            api.scusa();
            wassert(actual(api.voglioancora()) == 0);
        });
        add_method("insert_attrs_prendilo", [](Fixture& f) {
            // Test attrs prendilo
            fortran::DbAPI api(*f.db, "write", "write", "write");
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
            wassert(actual(api.voglioquesto()) == 1);
            api.dammelo();
            wassert(actual(api.voglioancora()) == 1);
            wassert(actual(api.enqi("*B33007")) == 60);
        });
        add_method("insert_attrs_prendilo_anaid", [](Fixture& f) {
            // Test prendilo anaid
            fortran::DbAPI api(*f.db, "write", "write", "write");
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
        });
        add_method("insert_auto_repmemo", [](Fixture& f) {
            // Check that an unknown rep_memo is correctly handled on insert
            fortran::DbAPI api(*f.db, "write", "write", "write");

            // Insert a record with a rep_memo that is not in the database
            api.setc("rep_memo", "insert_auto_repmemo");
            api.setd("lat", 45.6);
            api.setd("lon", 11.2);
            api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
            api.settimerange(254, MISSING_INT, MISSING_INT);
            api.setdate(2015, 4, 25, 12, 30, 45);
            api.setd("B12101", 286.4);
            api.prendilo();

            // Query it back
            api.unsetall();
            api.setc("rep_memo", "insert_auto_repmemo");
            wassert(actual(api.voglioquesto()) == 1);
            wassert(actual(api.dammelo()) == "B12101");
        });
        add_method("undefined_level2", [](Fixture& f) {
            // Test handling of values with undefined leveltype2 and l2
            fortran::DbAPI api(*f.db, "write", "write", "write");
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
        });
        add_method("delete_attrs_dammelo", [](Fixture& f) {
            // Test deleting attributes after a dammelo
            fortran::DbAPI api(*f.db, "write", "write", "write");
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
        });
        add_method("perms_consistency", [](Fixture& f) {
            {
                fortran::DbAPI api(*f.db, "read", "read", "read");
                try {
                    api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
                } catch (std::exception& e) {
                    wassert(actual(e.what()).contains("must be called on a session with writable"));
                }
            }
            {
                fortran::DbAPI api(*f.db, "write", "read", "read");
                try {
                    api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
                } catch (std::exception& e) {
                    wassert(actual(e.what()).contains("must be called on a session with writable"));
                }
            }
            {
                fortran::DbAPI api(*f.db, "read", "add", "read");
                try {
                    api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
                } catch (std::exception& e) {
                    wassert(actual(e.what()).contains("must be called on a session with writable"));
                }
            }
            {
                fortran::DbAPI api(*f.db, "write", "add", "read");
                api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
            }
            {
                fortran::DbAPI api(*f.db, "write", "write", "read");
                api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
            }
            {
                fortran::DbAPI api(*f.db, "write", "add", "write");
                api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
            }
            {
                fortran::DbAPI api(*f.db, "write", "write", "write");
                api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);
            }
        });
        add_method("messages_read_messages", [](Fixture& f) {
            // 2 messages, 1 subset each
            fortran::DbAPI api(*f.db, "write", "write", "write");
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", File::BUFR);

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
        });
        add_method("messages_read_messages_stdin", [](Fixture& f) {
            fortran::DbAPI api(*f.db, "write", "write", "write");

            // Connect stdin to an input file
            wreport::sys::File in(tests::datafile("bufr/synotemp.bufr"), O_RDONLY);
            TempBind tb(0, in);

            // 2 messages, 1 subset each
            api.messages_open_input("", "r", File::BUFR);

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
        });
        add_method("messages_read_subsets", [](Fixture& f) {
            // 1 message, 6 subsets
            fortran::DbAPI api(*f.db, "write", "write", "write");
            api.messages_open_input(dballe::tests::datafile("bufr/temp-gts2.bufr").c_str(), "r", File::BUFR);

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
        });
        add_method("messages_read_messages_subsets", [](Fixture& f) {
            // 2 messages, 2 subsets each
            fortran::DbAPI api(*f.db, "write", "write", "write");
            api.messages_open_input(dballe::tests::datafile("bufr/db-messages1.bufr").c_str(), "r", File::BUFR);

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
        });
        add_method("messages_write", [](Fixture& f) {
            // Write one message
            {
                fortran::DbAPI api(*f.db, "write", "write", "write");
                api.messages_open_output("test.bufr", "wb", File::BUFR);

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
                fortran::DbAPI api(*f.db, "write", "write", "write");
                api.messages_open_input("test.bufr", "rb", File::BUFR);

                wassert(actual(api.messages_read_next()).istrue());
                wassert(actual(api.voglioquesto()) == 2);
                wassert(actual(api.dammelo()) == "B12101");
                wassert(actual(api.enqd("B12101")) == 21.5);
                wassert(actual(api.dammelo()) == "B11002");
                wassert(actual(api.enqd("B11002")) == 2.4);

                wassert(actual(api.messages_read_next()).isfalse());
            }
        });
        add_method("messages_write_stdout", [](Fixture& f) {
            // Write one message
            {
                // Connect stdout to an output file
                wreport::sys::File out("test.bufr", O_WRONLY | O_CREAT | O_TRUNC);
                TempBind tb(1, out);

                {
                    fortran::DbAPI api(*f.db, "write", "write", "write");
                    api.messages_open_output("", "wb", File::BUFR);

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

                    fflush(stdout);
                }
            }

            // Read it back
            {
                fortran::DbAPI api(*f.db, "write", "write", "write");
                api.messages_open_input("test.bufr", "rb", File::BUFR);

                wassert(actual(api.messages_read_next()).istrue());
                wassert(actual(api.voglioquesto()) == 2);
                wassert(actual(api.dammelo()) == "B12101");
                wassert(actual(api.enqd("B12101")) == 21.5);
                wassert(actual(api.dammelo()) == "B11002");
                wassert(actual(api.enqd("B11002")) == 2.4);

                wassert(actual(api.messages_read_next()).isfalse());
            }
        });
        add_method("messages_bug1", [](Fixture& f) {
            // Reproduce an issue reported by Paolo
            // 2 messages, 2 subsets each
            fortran::DbAPI api(*f.db, "write", "write", "write");
            api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140312.bufr").c_str(), "r", File::BUFR);
            wassert(actual(api.messages_read_next()) == 1);
            api.unsetall();
            api.setcontextana();
            wassert(actual(api.voglioquesto()) == 3);
            // bug with mem DB: message: "enqi: B00000 (Context ID of the variable) is not defined"
            wassert(actual(api.dammelo()) == "B01194");
            wassert(actual(api.dammelo()) == "B05001");
            wassert(actual(api.dammelo()) == "B06001");
        });
        add_method("messages_bug2", [](Fixture& f) {
            // Reproduce an issue reported by Paolo
            // 2 messages, 2 subsets each
            fortran::DbAPI api(*f.db, "write", "write", "write");
            api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140326.bufr").c_str(), "r", File::BUFR);
            wassert(actual(api.messages_read_next()) == 1);
            api.unsetall();
            api.setcontextana();
            wassert(actual(api.voglioquesto()) == 5);
            wassert(actual(api.dammelo()) == "B01019");
            // Bug: missing variable 000000 in table dballe
            wassert(actual(api.voglioancora()) == 0);
        });
        add_method("attr_reference_id", [](Fixture& f) {
            // Test attr_reference_id behaviour
            fortran::DbAPI api(*f.db, "write", "write", "write");
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
            wassert(actual(api.enqi("context_id")) != MISSING_INT);
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
        });
        add_method("attrs_bug1", [](Fixture& f) {
            // Reproduce a bug when setting attributes
            fortran::DbAPI dbapi0(*f.db, "write", "write", "write");
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
        });
        add_method("stationdata_bug1", [](Fixture& f) {
            // Reproduce a bug handling station data
            {
                fortran::DbAPI dbapi0(*f.db, "write", "write", "write");
                dbapi0.scopa();
                // Copy a message using the API
                dbapi0.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140403.bufr").c_str(), "r", File::BUFR);
                dbapi0.messages_open_output("test.bufr", "w", File::BUFR);
                wassert(actual(dbapi0.messages_read_next()) == 1);
                //dbapi0.setcontextana();
                dbapi0.messages_write_next("generic");
                dbapi0.remove_all();
                wassert(actual(dbapi0.messages_read_next()) == 1);
                //dbapi0.setcontextana();
                dbapi0.messages_write_next("generic");
                dbapi0.remove_all();
                wassert(actual(dbapi0.messages_read_next()) == 0);
            }

            // TODO: decide what is the expected behaviour for exporting only station
            // values

            // // Compare the two messages
            // std::unique_ptr<Msgs> msgs1 = read_msgs("bufr/generic-bug20140403.bufr", File::BUFR);
            // std::unique_ptr<Msgs> msgs2 = read_msgs("./test.bufr", File::BUFR);
            // unsigned diffs = msgs1->diff(*msgs2);
            // if (diffs) dballe::tests::track_different_msgs(*msgs1, *msgs2, "apicopy");
            // wassert(actual(diffs) == 0);
        });
        add_method("segfault1", [](Fixture& f) {
            // Reproduce a segfault with mem:
            fortran::DbAPI dbapi0(*f.db, "write", "write", "write");
            dbapi0.seti("lat", 4500000);
            dbapi0.seti("lon", 1300000);
            dbapi0.setc("rep_memo", "generic");
            dbapi0.setcontextana();
            dbapi0.setc("B12102", "26312");
            wassert(dbapi0.prendilo());
            dbapi0.setc("*B33194", "50");
            wassert(dbapi0.critica());
        });
        add_method("attr_insert", [](Fixture& f) {
            // Reproduce a problem with attribute insert when inserting a variable
            // that already exists in the database
            fortran::DbAPI pre(*f.db, "write", "write", "write");
            pre.unsetall();
            pre.seti("lat", 4452128);
            pre.seti("lon", 1199127);
            pre.unset("ident");
            pre.unset("mobile");
            pre.setc("rep_memo", "locali");
            pre.setdate(2014, 8, 1, 0, 0, 0);
            pre.setlevel(103, 2000, 2147483647, 2147483647);
            pre.settimerange(254, 0, 0);
            pre.setd("B12101", 273.149994);
            pre.prendilo();
            pre.fatto();

            fortran::DbAPI dbapi0(*f.db, "write", "write", "write");
            dbapi0.unsetall();
            dbapi0.seti("lat", 4452128);
            dbapi0.seti("lon", 1199127);
            dbapi0.unset("ident");
            dbapi0.unset("mobile");
            dbapi0.setc("rep_memo", "locali");
            dbapi0.setdate(2014, 8, 1, 0, 0, 0);
            dbapi0.setlevel(103, 2000, 2147483647, 2147483647);
            dbapi0.settimerange(254, 0, 0);
            dbapi0.setd("B12101", 273.149994);
            dbapi0.prendilo();
            dbapi0.seti("*B33192", 0);
            dbapi0.setc("*var_related", "B12101");
            dbapi0.critica();
        });
        add_method("bug45", [](Fixture& f) {
            // ** Execution begins **
            fortran::DbAPI dbapi0(*f.db, "write", "write", "write");
            dbapi0.scopa();
            dbapi0.unsetall();
            dbapi0.seti("lat", 4500000);
            dbapi0.seti("lon", 1000000);
            dbapi0.unset("ident");
            dbapi0.unset("mobile");
            dbapi0.setc("rep_memo", "generic");
            dbapi0.setdate(2014, 1, 6, 18, 0, 0);
            dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
            dbapi0.settimerange(4, 3600, 7200);
            dbapi0.seti("B13003", 85);
            dbapi0.prendilo();
            dbapi0.setd("*B33192", 30.000000);
            dbapi0.seti("*B33193", 50);
            dbapi0.setd("*B33194", 70.000000);
            dbapi0.critica();
            dbapi0.seti("B12101", 27315);
            dbapi0.prendilo();
            dbapi0.setd("*B33192", 30.000000);
            dbapi0.seti("*B33193", 50);
            dbapi0.critica();
            // error: cannot insert attributes for variable 000000: no data id given or found from last prendilo()
        });
    }
};

Tests tg1("dbapi_mem", nullptr, db::MEM);
Tests tg2("dbapi_v6_sqlite", "SQLITE", db::V6);
Tests tg3("dbapi_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_ODBC
Tests tg4("dbapi_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg5("dbapi_v6_postgresql", "POSTGRESQL", db::V6);
Tests tg6("dbapi_v7_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg7("dbapi_v6_mysql", "MYSQL", db::V6);
#endif

}
