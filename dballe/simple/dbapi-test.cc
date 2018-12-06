#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/trace.h"
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

template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

template<typename DB>
class CommitTests : public FixtureTestCase<DBFixture<DB>>
{
    typedef DBFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2("simple_dbapi_tr_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("simple_dbapi_tr_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("simple_dbapi_tr_v7_mysql", "MYSQL");
#endif

CommitTests<V7DB> ct2("simple_dbapi_db_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
CommitTests<V7DB> ct4("simple_dbapi_db_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
CommitTests<V7DB> ct6("simple_dbapi_db_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests() {

this->add_method("query_basic", [](Fixture& f) {
    // Test vars
    fortran::DbAPI api(f.tr, "write", "write", "write");
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
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    wassert(actual(api.enqd("B12101")) == 21.5);
    wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
    wassert(actual(api.enqd("lat")) == 44.5);
    wassert(actual(api.enqd("lon")) == 11.5);
    wassert(actual(api.enqd("B11002")) == 2.4);

    // Delete variables
    api.unsetall();
    api.setc("var", "B12101");
    api.dimenticami();
    wassert(actual(api.voglioquesto()) == 0);
});

this->add_method("query_attrs", [](Fixture& f) {
    // Test attrs
    fortran::DbAPI api(f.tr, "write", "write", "write");
    populate_variables(api);

    for (const char* query: { "", "attrs", "best", "attrs,best" })
    {
        WREPORT_TEST_INFO(locinfo);
        locinfo() << "Query: " << query;

        int reference_id;

        // Query a variable
        api.setc("var", "B12101");
        api.setc("query", query);
        wassert(actual(api.voglioquesto()) == 1);
        wassert(api.dammelo());

        wassert(actual(api.test_get_operation()).istrue());

        // Store its context info to access attributes of this variable later
        reference_id = api.enqi("context_id");

        // It has no attributes
        wassert(actual(api.voglioancora()) == 0);
        wassert(actual(api.test_get_operation()).istrue());

        // Set one attribute after a dammelo
        api.seti("*B33007", 50);
        wassert(api.critica());
        wassert(actual(api.test_get_operation()).istrue());

        // It now has one attribute
        wassert(actual(api.voglioancora()) == 1);
        wassert(actual(api.enqi("*B33007")) == 50);
        wassert(actual(api.ancora()) == "*B33007");
        wassert(actual(api.enqi("*B33007")) == 50);

        // Query it back, it has attributes
        api.setc("var", "B12101");
        api.setc("query", query);
        wassert(actual(api.voglioquesto()) == 1);
        wassert(api.dammelo());
        wassert(actual(api.voglioancora()) == 1);
        wassert(actual(api.enqi("*B33007")) == 50);

        // Query a different variable, it has no attributes
        api.setc("var", "B11002");
        api.setc("query", query);
        wassert(actual(api.voglioquesto()) == 1);
        wassert(api.dammelo());
        wassert(actual(api.voglioancora()) == 0);

        // Query the first variable using its stored reference id
        api.seti("*context_id", reference_id);
        api.setc("*var_related", "B12101");
        wassert(actual(api.test_get_operation()).istrue());
        wassert(actual(api.voglioancora()) == 1);
        wassert(actual(api.enqi("*B33007")) == 50);

        // Delete all attributes
        wassert(api.scusa());
        wassert(actual(api.voglioancora()) == 0);
    }
});

this->add_method("insert_attrs_prendilo", [](Fixture& f) {
    // Test attrs prendilo
    fortran::DbAPI api(f.tr, "write", "write", "write");

    // Set one attribute after a prendilo
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
    api.settimerange(254, 0, 0);
    api.setd("B10004", 100000.0);
    api.prendilo(); // Pressure at ground level
    wassert(actual(api.enqi("ana_id")) != 0);
    api.seti("*B33007", 60);
    api.critica();

    // Query it again
    api.unsetall();
    api.setc("var", "B10004");
    wassert(actual(api.voglioquesto()) == 1);
    wassert(actual(api.dammelo()) == WR_VAR(0, 10, 4));
    wassert(actual(api.voglioancora()) == 1);
    wassert(actual(api.enqi("*B33007")) == 60);
    wassert(actual(api.enqd("*B33007")) == 60.0);
    string res;
    wassert_true(api.enqc("*B33007", res));
    wassert(actual(res) == "60");
});

this->add_method("insert_attrs_prendilo_anaid", [](Fixture& f) {
    // Test prendilo anaid
    fortran::DbAPI api(f.tr, "write", "write", "write");
    populate_variables(api);

    // Run a prendilo
    api.setd("lat", 44.6);
    api.setd("lon", 11.6);
    api.setc("rep_memo", "synop");
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
    api.settimerange(254, 0, 0);
    api.setd("B10004", 100000.0);
    api.prendilo(); // Pressure at ground level

    int anaid = api.enqi("ana_id");
    wassert(actual(anaid) != MISSING_INT);

    // Query it back
    api.unsetall();
    api.seti("ana_id", anaid);
    api.setc("var", "B10004");
    wassert(actual(api.voglioquesto()) == 1);
    wassert(actual(api.dammelo()) == WR_VAR(0, 10, 4));

    // Querying the variable of the other station with the same ana_id has no
    // results
    api.unsetall();
    api.seti("ana_id", anaid);
    api.setc("var", "B12101");
    wassert(actual(api.voglioquesto()) == 0);
    wassert(actual(api.dammelo()) == 0);
});

this->add_method("insert_auto_repmemo", [](Fixture& f) {
    // Check that an unknown rep_memo is correctly handled on insert
    fortran::DbAPI api(f.tr, "write", "write", "write");

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
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
});

this->add_method("undefined_level2", [](Fixture& f) {
    // Test handling of values with undefined leveltype2 and l2
    fortran::DbAPI api(f.tr, "write", "write", "write");
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

    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    wassert(actual(api.enqi("leveltype1")) == 103);
    wassert(actual(api.enqi("l1")) == 2000);
    wassert(actual(api.enqi("leveltype2")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("l2")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("pindicator")) == 254);
    wassert(actual(api.enqi("p1")) == fortran::DbAPI::missing_int);
    wassert(actual(api.enqi("p2")) == fortran::DbAPI::missing_int);
});

this->add_method("delete_attrs_dammelo", [](Fixture& f) {
    // Test deleting attributes after a dammelo
    fortran::DbAPI api(f.tr, "write", "write", "write");
    populate_variables(api);

    // Query all variables and add attributes
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    api.seti("*B33007", 50);
    api.critica();
    wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
    api.seti("*B33007", 60);
    api.critica();

    // Query all variables again and check that attributes are there
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    wassert(actual(api.voglioancora()) == 1);
    wassert(actual(api.enqi("*B33007")) == 50);
    wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
    wassert(actual(api.voglioancora()) == 1);
    wassert(actual(api.enqi("*B33007")) == 60);

    // Query all variables and delete all attributes
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    api.scusa();
    wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
    api.scusa();

    // Query again and check that the attributes are gone
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 2);
    wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
    wassert(actual(api.voglioancora()) == 0);
    wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
    wassert(actual(api.voglioancora()) == 0);

    // The QC attrs record should be cleaned
    wassert(actual(api.enqi("*B33007")) == MISSING_INT);
});

this->add_method("perms_consistency", [](Fixture& f) {
    {
        fortran::DbAPI api(f.tr, "read", "read", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(f.tr, "write", "read", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(f.tr, "read", "add", "read");
        try {
            api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
        } catch (std::exception& e) {
            wassert(actual(e.what()).contains("must be called on a session with writable"));
        }
    }
    {
        fortran::DbAPI api(f.tr, "write", "add", "read");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
    }
    {
        fortran::DbAPI api(f.tr, "write", "write", "read");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
    }
    {
        fortran::DbAPI api(f.tr, "write", "add", "write");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
    }
    {
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);
    }
});

this->add_method("messages_read_messages", [](Fixture& f) {
    // 2 messages, 1 subset each
    fortran::DbAPI api(f.tr, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r", Encoding::BUFR);

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
this->add_method("messages_read_messages_stdin", [](Fixture& f) {
    fortran::DbAPI api(f.tr, "write", "write", "write");

    // Connect stdin to an input file
    wreport::sys::File in(tests::datafile("bufr/synotemp.bufr"), O_RDONLY);
    TempBind tb(0, in);

    // 2 messages, 1 subset each
    api.messages_open_input("", "r", Encoding::BUFR);

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
this->add_method("messages_read_subsets", [](Fixture& f) {
    // 1 message, 6 subsets
    fortran::DbAPI api(f.tr, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/temp-gts2.bufr").c_str(), "r", Encoding::BUFR);

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
this->add_method("messages_read_messages_subsets", [](Fixture& f) {
    // 2 messages, 2 subsets each
    fortran::DbAPI api(f.tr, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/db-messages1.bufr").c_str(), "r", Encoding::BUFR);

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
this->add_method("messages_write", [](Fixture& f) {
    // Write one message
    {
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_output("test.bufr", "wb", Encoding::BUFR);

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
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input("test.bufr", "rb", Encoding::BUFR);

        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.voglioquesto()) == 2);
        wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
        wassert(actual(api.enqd("B12101")) == 21.5);
        wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
        wassert(actual(api.enqd("B11002")) == 2.4);

        wassert(actual(api.messages_read_next()).isfalse());
    }
});
this->add_method("messages_write_stdout", [](Fixture& f) {
    // Write one message
    {
        // Connect stdout to an output file
        wreport::sys::File out("test.bufr", O_WRONLY | O_CREAT | O_TRUNC);
        TempBind tb(1, out);

        {
            fortran::DbAPI api(f.tr, "write", "write", "write");
            api.messages_open_output("", "wb", Encoding::BUFR);

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
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input("test.bufr", "rb", Encoding::BUFR);

        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.voglioquesto()) == 2);
        wassert(actual(api.dammelo()) == WR_VAR(0, 12, 101));
        wassert(actual(api.enqd("B12101")) == 21.5);
        wassert(actual(api.dammelo()) == WR_VAR(0, 11,   2));
        wassert(actual(api.enqd("B11002")) == 2.4);

        wassert(actual(api.messages_read_next()).isfalse());
    }
});
this->add_method("messages_bug1", [](Fixture& f) {
    // Reproduce an issue reported by Paolo
    // 2 messages, 2 subsets each
    fortran::DbAPI api(f.tr, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140312.bufr").c_str(), "r", Encoding::BUFR);
    wassert(actual(api.messages_read_next()) == 1);
    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 3);
    // bug with mem DB: message: "enqi: B00000 (Context ID of the variable) is not defined"
    wassert(actual(api.dammelo()) == WR_VAR(0, 1, 194));
    wassert(actual(api.dammelo()) == WR_VAR(0, 5,   1));
    wassert(actual(api.dammelo()) == WR_VAR(0, 6,   1));
});
this->add_method("messages_bug2", [](Fixture& f) {
    // Reproduce an issue reported by Paolo
    // 2 messages, 2 subsets each
    fortran::DbAPI api(f.tr, "write", "write", "write");
    api.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140326.bufr").c_str(), "r", Encoding::BUFR);
    wassert(actual(api.messages_read_next()) == 1);
    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 5);
    wassert(actual(api.dammelo()) == WR_VAR(0, 1, 19));
    // Bug: missing variable 000000 in table dballe
    wassert(actual(api.voglioancora()) == 0);
});

this->add_method("attr_reference_id", [](Fixture& f) {
    // Test attr_reference_id behaviour
    fortran::DbAPI api(f.tr, "write", "write", "write");

    // Try setting a context with a missing context_id
    {
        auto e = wassert_throws(wreport::error_consistency, api.setc("*var_related", "B07030"));
        wassert(actual(e.what()).matches("\\*var_related set without context_id, or before any dammelo or prendilo"));
    }

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
    int id_B07030 = wcallchecked(api.enqi("context_id"));
    wassert(actual(id_B07030) != MISSING_INT);
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
    wassert(api.dammelo());
    // Read its attrs (ok)
    wassert(actual(api.voglioancora()) == 1);
    // Cannot manipulate station attrs setting *context_id

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

this->add_method("attrs_bug1", [](Fixture& f) {
    // Reproduce a bug when setting attributes
    using namespace dballe::fortran;
    fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
    dbapi0.scopa();

    // Add a value
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1000000);
    dbapi0.unset("ident");
    dbapi0.seti("mobile", 0);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
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
    wassert(actual(dbapi0.dammelo()) == WR_VAR(0, 13, 3));
    wassert(actual(dbapi0.voglioancora()) == 3);
});

this->add_method("stationdata_bug1", [](Fixture& f) {
    // Reproduce a bug handling station data
    {
        fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
        dbapi0.scopa();
        // Copy a message using the API
        dbapi0.messages_open_input(dballe::tests::datafile("bufr/generic-bug20140403.bufr").c_str(), "r", Encoding::BUFR);
        dbapi0.messages_open_output("test.bufr", "w", Encoding::BUFR);
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
    // std::unique_ptr<Msgs> msgs1 = read_msgs("bufr/generic-bug20140403.bufr", Encoding::BUFR);
    // std::unique_ptr<Msgs> msgs2 = read_msgs("./test.bufr", Encoding::BUFR);
    // unsigned diffs = msgs1->diff(*msgs2);
    // if (diffs) dballe::tests::track_different_msgs(*msgs1, *msgs2, "apicopy");
    // wassert(actual(diffs) == 0);
});
this->add_method("segfault1", [](Fixture& f) {
    // Reproduce a segfault with mem:
    fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1300000);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setcontextana();
    dbapi0.setc("B12102", "26312");
    wassert(dbapi0.prendilo());
    dbapi0.setc("*B33194", "50");
    wassert(dbapi0.critica());
});

this->add_method("issue45", [](Fixture& f) {
    using namespace dballe::fortran;
    fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
    dbapi0.scopa();
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1000000);
    dbapi0.unset("ident");
    dbapi0.unset("mobile");
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
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

this->add_method("issue52", [](Fixture& f) {
    using namespace wreport;

    std::string fname = dballe::tests::datafile("bufr/issue52.bufr");
    fortran::DbAPI dbapi(f.tr, "write", "write", "write");
    wassert(dbapi.messages_open_input(fname.c_str(), "r", Encoding::BUFR, true));
    wassert(actual(dbapi.messages_read_next()).istrue());
    wassert(actual(dbapi.messages_read_next()).istrue());

    // error: no year information found in message to import
});

this->add_method("perf_data", [](Fixture& f) {
    // Test prendilo anaid
    fortran::DbAPI api(f.tr, "write", "write", "write");

    // Run a prendilo
    f.tr->trc->clear();
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setc("rep_memo", "synop");
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
    api.settimerange(254, 0, 0);
    api.setd("B10004", 100000.0);
    api.prendilo(); // Pressure at ground level
    v7::trace::Aggregate stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 2);
    wassert(actual(stats.rows) == 0);
    stats = f.tr->trc->aggregate("insert");
    wassert(actual(stats.count) == 3);
    wassert(actual(stats.rows) == 3);

    // Query it back
    f.tr->trc->clear();
    api.unsetall();
    api.setd("lat", 44.5);
    api.setd("lon", 11.5);
    api.setdate(2013, 4, 25, 12, 0, 0);
    api.setc("var", "B10004");
    wassert(actual(api.voglioquesto()) == 1);
    stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 2);
    wassert(actual(stats.rows) == 2);

    // Query stations only
    f.tr->trc->clear();
    api.unsetall();
    wassert(actual(api.quantesono()) == 1);
    stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 1);
    wassert(actual(stats.rows) == 1);
});

this->add_method("perf_read_attrs", [](Fixture& f) {
    auto msgs = read_msgs("bufr/temp-gts1.bufr", Encoding::BUFR, impl::ImporterOptions("accurate"));
    wassert(f.tr->import_messages(msgs, DBImportOptions::defaults));
    wassert(f.tr->clear_cached_state());

    // Readonly test session
    fortran::DbAPI api(f.tr, "read", "read", "read");

    // Read all station data and their attributes
    f.tr->trc->clear();
    api.unsetall();
    api.setcontextana();
    wassert(actual(api.voglioquesto()) == 7);
    while (api.dammelo())
        wassert(api.voglioancora());

    // Check number of queries
    v7::trace::Aggregate stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 8);
    wassert(actual(stats.rows) == 14);

    // Read all measured data and their attributes
    f.tr->trc->clear();
    api.unsetall();
    wassert(actual(api.voglioquesto()) == 550);
    while (api.dammelo())
        wassert(api.voglioancora());

    // Check number of queries
    stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 552);
    wassert(actual(stats.rows) == 1156);

    // Repeat with query=attrs

    // Read all station data and their attributes
    f.tr->trc->clear();
    api.unsetall();
    api.setcontextana();
    api.setc("query", "attrs");
    wassert(actual(api.voglioquesto()) == 7);
    while (api.dammelo())
        wassert(api.voglioancora());

    // Check number of queries
    stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 1);
    wassert(actual(stats.rows) == 7);

    // Read all measured data and their attributes
    f.tr->trc->clear();
    api.unsetall();
    api.setc("query", "attrs");
    wassert(actual(api.voglioquesto()) == 550);
    while (api.dammelo())
        wassert(api.voglioancora());

    // Check number of queries
    stats = f.tr->trc->aggregate("select");
    wassert(actual(stats.count) == 2);
    wassert(actual(stats.rows) == 606);
});

}

template<typename DB>
void CommitTests<DB>::register_tests() {

this->add_method("attr_insert", [](Fixture& f) {
    // Reproduce a problem with attribute insert when inserting a variable
    // that already exists in the database
    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI pre(tr, "write", "write", "write");
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
        pre.commit();
    }

    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi0(tr, "write", "write", "write");
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
    }
});

this->add_method("issue73", [](Fixture& f) {
    using namespace wreport;

    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi(tr, "write", "write", "write");
        wassert(populate_variables(dbapi));
        wassert(dbapi.commit());
    }

    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi(tr, "read", "read", "read");
        dbapi.setc("varlist", "B10004,B12101,B12103");
        wassert(actual(dbapi.voglioquesto()) == 1);
    }
});

this->add_method("issue75", [](Fixture& f) {
    using namespace wreport;

    std::string fname = dballe::tests::datafile("bufr/issue75.bufr");
    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi(tr, "write", "write", "write");
        wassert(dbapi.messages_open_input(fname.c_str(), "r", Encoding::BUFR, true));
        wassert(actual(dbapi.messages_read_next()).istrue());
        wassert(actual(dbapi.messages_read_next()).istrue());
        wassert(actual(dbapi.messages_read_next()).istrue());
        wassert(actual(dbapi.messages_read_next()).istrue());
        wassert(dbapi.commit());
    }

    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi(tr, "read", "read", "read");
        dbapi.setc("rep_memo", "temp");
        dbapi.setcontextana();
        dbapi.setc("varlist", "B07001");
        wassert(actual(dbapi.voglioquesto()) == 0);
        dbapi.setc("varlist", "B07030");
        wassert(actual(dbapi.voglioquesto()) == 1);
    }
});

this->add_method("transactions", [](Fixture& f) {
    // FIXME: move check to fortran and python bindings
    // Write, do not commit
    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI api(tr, "write", "write", "write");
        wassert(actual(api.voglioquesto()) == 0);
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.prendilo();
        wassert(actual(api.voglioquesto()) == 1);
    }

    // Write, commit
    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI api(tr, "write", "write", "write");
        wassert(actual(api.voglioquesto()) == 0);
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.prendilo();
        wassert(actual(api.voglioquesto()) == 1);
        wassert(api.commit());
    }

    // This time, data was written
    {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI api(tr, "write", "write", "write");
        wassert(actual(api.voglioquesto()) == 1);
    }
});

this->add_method("insert_block", [](Fixture& f) {
    auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
    fortran::DbAPI dbapi0(tr, "write", "write", "write");
    dbapi0.scopa();
    dbapi0.setd("lat", -45.678902);
    dbapi0.setd("lon", -12.345600);
    dbapi0.setc("rep_memo", "synop");
    dbapi0.seti("block", 1);
    dbapi0.setcontextana();
    dbapi0.prendilo();
    wassert(actual(dbapi0.quantesono()) == 1);
});

this->add_method("query_attr_values", [](Fixture& f) {
    auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
    fortran::DbAPI dbapi0(tr, "write", "write", "write");
    dbapi0.scopa();
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1000000);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
    dbapi0.settimerange(4, 3600, 7200);
    dbapi0.seti("B13003", 85);
    dbapi0.prendilo();
    dbapi0.unsetb();
    dbapi0.setd("*B33192", 30.000000);
    dbapi0.seti("*B33193", 50);
    dbapi0.setd("*B33194", 70.000000);
    dbapi0.critica();
    dbapi0.seti("B12101", 27315);
    dbapi0.prendilo();
    dbapi0.unsetb();
    dbapi0.setd("*B33192", 30.000000);
    dbapi0.seti("*B33193", 50);
    dbapi0.setd("*B33194", 70.000000);
    dbapi0.critica();
    dbapi0.unsetall();
    dbapi0.setc("varlist", "B12101");
    wassert(actual(dbapi0.voglioquesto()) == 1);
    wassert(actual(dbapi0.dammelo()) == WR_VAR(0, 12, 101));
    dbapi0.setc("*varlist", "*B33193,*B33194");
    wassert(actual(dbapi0.voglioancora()) == 2);
    wassert(actual(dbapi0.ancora()) == "*B33193");
    wassert(dbapi0.enqi("*B33193"));
    wassert(dbapi0.enqb("*B33193"));
    wassert(dbapi0.enqr("*B33193"));
    wassert(dbapi0.enqd("*B33193"));
    string sres;
    wassert(dbapi0.enqc("*B33193", sres));
    // error: cannot parse a Varcode out of '*B33193'
});

this->add_method("query_attr_filtered", [](Fixture& f) {
    auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
    fortran::DbAPI dbapi0(tr, "write", "write", "write");
    dbapi0.scopa();
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1000000);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
    dbapi0.settimerange(4, 3600, 7200);
    dbapi0.seti("B13003", 85);
    dbapi0.prendilo();
    dbapi0.unsetb();
    dbapi0.setd("*B33192", 30.000000);
    dbapi0.seti("*B33193", 50);
    dbapi0.setd("*B33194", 70.000000);
    dbapi0.critica();
    dbapi0.seti("B12101", 27315);
    dbapi0.prendilo();
    dbapi0.unsetb();
    dbapi0.setd("*B33192", 30.000000);
    dbapi0.seti("*B33193", 50);
    dbapi0.setd("*B33194", 70.000000);
    dbapi0.critica();
    dbapi0.unsetall();
    dbapi0.setc("varlist", "B12101");
    dbapi0.setc("*varlist", "*B33193,*B33194");
    wassert(actual(dbapi0.voglioquesto()) == 1);
    wassert(actual(dbapi0.dammelo()) == WR_VAR(0, 12, 101));
    wassert(actual(dbapi0.voglioancora()) == 2);
    wassert(actual(dbapi0.ancora()) == "*B33193");
    wassert(actual(dbapi0.ancora()) == "*B33194");
});

this->add_method("set_varlist", [](Fixture& f) {
    auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
    fortran::DbAPI dbapi0(tr, "write", "write", "write");
    wassert(dbapi0.setc("*varlist", "*B33193,*B33194"));
});

this->add_method("issue137", [](Fixture& f) {
    using namespace dballe::fortran;
    auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
    fortran::DbAPI dbapi0(tr, "write", "write", "write");
    dbapi0.scopa();

    // Insert two variables
    dbapi0.unsetall();
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1300000);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
    dbapi0.settimerange(4, 3600, 7200);
    dbapi0.seti("B12102", 26312);
    dbapi0.setd("B12101", 273.149994);
    dbapi0.prendilo();

    // Remove one. In a previous implementation, var and varlist both edited
    // Query::varcodes, and unsetting one had the effect of unsetting the other
    dbapi0.unsetall();
    dbapi0.setc("var", "B12101");
    dbapi0.unset("varlist");
    dbapi0.dimenticami();

    dbapi0.unsetall();
    dbapi0.setc("var", "B12102");
    dbapi0.unset("varlist");
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1100000);
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
    dbapi0.settimerange(4, 3600, 7200);
    dbapi0.dimenticami();

    dbapi0.unsetall();
    dbapi0.setc("var", "B12102");
    dbapi0.unset("varlist");
    dbapi0.seti("lat", 4500000);
    dbapi0.seti("lon", 1300000);
    dbapi0.unset("ident");
    dbapi0.unset("mobile");
    dbapi0.setc("rep_memo", "generic");
    dbapi0.setdate(2014, 1, 6, 18, 0, 0);
    dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
    dbapi0.settimerange(4, 3600, 7200);
    wassert(actual(dbapi0.voglioquesto()) == 1);
    wassert(actual(dbapi0.dammelo()) == WR_VAR(0, 12, 102));;
});

}

}
