#include "config.h"
#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/trace.h"
#include "dballe/db/v7/transaction.h"
#include "dbapi.h"
#include "msgapi.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <wreport/utils/sys.h>

using namespace std;
using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;

namespace {

struct TempStdin
{
    TempStdin(const char* fname, const char* mode)
    {
        wassert_true(freopen(fname, mode, stdin));
    }

    ~TempStdin() { wassert_true(freopen("/dev/stdin", "r", stdin)); }
};

#if 0
struct TempStdout
{
    TempStdout(const char* fname, const char* mode)
    {
        wassert_true(freopen(fname, mode, stdout));
    }

    ~TempStdout()
    {
        wassert_true(freopen("/dev/stdout", "w", stdout));
    }
};
#endif

struct TempBind
{
    int fd;
    int old_val;

    TempBind(int fd, int new_fd) : fd(fd)
    {
        // Connect stdin to an input file
        old_val = dup(fd);
        wassert(actual(old_val) != -1);
        wassert(actual(dup2(new_fd, fd)) != -1);
    }

    ~TempBind() { wassert(actual(dup2(old_val, fd)) != -1); }
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
    api.insert_data();

    // Instant wind speed, 10 meters above ground
    api.unsetb();
    api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
    api.setd("B11002", 2.4);
    api.insert_data();

    api.unsetall();
}

template <typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

template <typename DB> class CommitTests : public FixtureTestCase<DBFixture<DB>>
{
    typedef DBFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2("fortran_dbapi_tr_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("fortran_dbapi_tr_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("fortran_dbapi_tr_v7_mysql", "MYSQL");
#endif

CommitTests<V7DB> ct2("fortran_dbapi_db_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
CommitTests<V7DB> ct4("fortran_dbapi_db_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
CommitTests<V7DB> ct6("fortran_dbapi_db_v7_mysql", "MYSQL");
#endif

template <typename DB> void Tests<DB>::register_tests()
{

    this->add_method("query_basic", [](Fixture& f) {
        // Test vars
        fortran::DbAPI api(f.tr, "write", "write", "write");
        populate_variables(api);

        // Query stations
        api.unsetall();
        wassert(actual(api.query_stations()) == 1);
        api.next_station();
        wassert(actual(api.enqd("lat")) == 44.5);
        wassert(actual(api.enqd("lon")) == 11.5);

        // Query variables
        api.unsetall();
        wassert(actual(api.query_data()) == 2);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(api.enqd("B12101")) == 21.5);
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        wassert(actual(api.enqd("lat")) == 44.5);
        wassert(actual(api.enqd("lon")) == 11.5);
        wassert(actual(api.enqd("B11002")) == 2.4);

        // Delete variables
        api.unsetall();
        api.setc("var", "B12101");
        api.remove_data();
        wassert(actual(api.query_data()) == 0);
    });

    this->add_method("query_attrs", [](Fixture& f) {
        // Test attrs
        fortran::DbAPI api(f.tr, "write", "write", "write");
        populate_variables(api);

        for (const char* query : {"", "attrs", "best", "attrs,best"})
        {
            WREPORT_TEST_INFO(locinfo);
            locinfo() << "Query: " << query;

            int reference_id;

            // Query a variable
            api.setc("var", "B12101");
            api.setc("query", query);
            wassert(actual(api.query_data()) == 1);
            wassert(api.next_data());

            wassert(actual(api.test_get_operation()).istrue());

            // Store its context info to access attributes of this variable
            // later
            reference_id = api.enqi("context_id");

            // It has no attributes
            wassert(actual(api.query_attributes()) == 0);
            wassert(actual(api.test_get_operation()).istrue());

            // Set one attribute after a next_data
            api.seti("*B33007", 50);
            wassert(api.insert_attributes());
            wassert(actual(api.test_get_operation()).istrue());

            // It now has one attribute
            wassert(actual(api.query_attributes()) == 1);
            wassert(actual(api.enqi("*B33007")) == 50);
            wassert(actual(api.next_attribute()) == "*B33007");
            wassert(actual(api.enqi("*B33007")) == 50);

            // Query it back, it has attributes
            api.setc("var", "B12101");
            api.setc("query", query);
            wassert(actual(api.query_data()) == 1);
            wassert(api.next_data());
            wassert(actual(api.query_attributes()) == 1);
            wassert(actual(api.enqi("*B33007")) == 50);

            // Query a different variable, it has no attributes
            api.setc("var", "B11002");
            api.setc("query", query);
            wassert(actual(api.query_data()) == 1);
            wassert(api.next_data());
            wassert(actual(api.query_attributes()) == 0);

            // Query the first variable using its stored reference id
            api.seti("*context_id", reference_id);
            api.setc("*var_related", "B12101");
            wassert(actual(api.test_get_operation()).istrue());
            wassert(actual(api.query_attributes()) == 1);
            wassert(actual(api.enqi("*B33007")) == 50);

            // Delete all attributes
            wassert(api.remove_attributes());
            wassert(actual(api.query_attributes()) == 0);
        }
    });

    this->add_method("insert_attrs_insert_data", [](Fixture& f) {
        // Test attrs insert_data
        fortran::DbAPI api(f.tr, "write", "write", "write");

        // Set one attribute after a insert_data
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
        api.settimerange(254, 0, 0);
        api.setd("B10004", 100000.0);
        api.insert_data(); // Pressure at ground level
        wassert(actual(api.enqi("ana_id")) != 0);
        api.seti("*B33007", 60);
        api.insert_attributes();

        // Query it again
        api.unsetall();
        api.setc("var", "B10004");
        wassert(actual(api.query_data()) == 1);
        wassert(actual(api.next_data()) == WR_VAR(0, 10, 4));
        wassert(actual(api.query_attributes()) == 1);
        wassert(actual(api.enqi("*B33007")) == 60);
        wassert(actual(api.enqd("*B33007")) == 60.0);
        char res[4];
        wassert_true(api.enqc("*B33007", res, 4));
        wassert(actual(string(res, 4)) == "60  ");
    });

    this->add_method("insert_attrs_insert_data", [](Fixture& f) {
        // Test insert_data anaid
        fortran::DbAPI api(f.tr, "write", "write", "write");
        populate_variables(api);

        // Run a insert_data
        api.setd("lat", 44.6);
        api.setd("lon", 11.6);
        api.setc("rep_memo", "synop");
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
        api.settimerange(254, 0, 0);
        api.setd("B10004", 100000.0);
        api.insert_data(); // Pressure at ground level

        int anaid = api.enqi("ana_id");
        wassert(actual(anaid) != MISSING_INT);

        // Query it back
        api.unsetall();
        api.seti("ana_id", anaid);
        api.setc("var", "B10004");
        wassert(actual(api.query_data()) == 1);
        wassert(actual(api.next_data()) == WR_VAR(0, 10, 4));

        // Querying the variable of the other station with the same ana_id has
        // no results
        api.unsetall();
        api.seti("ana_id", anaid);
        api.setc("var", "B12101");
        wassert(actual(api.query_data()) == 0);
        wassert(actual(api.next_data()) == 0);
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
        api.insert_data();

        // Query it back
        api.unsetall();
        api.setc("rep_memo", "insert_auto_repmemo");
        wassert(actual(api.query_data()) == 1);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
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
        api.insert_data();
        api.unsetall();

        // Query it back
        api.seti("leveltype1", 103);
        wassert(actual(api.query_data()) == 1);

        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(api.enqi("leveltype1")) == 103);
        wassert(actual(api.enqi("l1")) == 2000);
        wassert(actual(api.enqi("leveltype2")) == fortran::DbAPI::missing_int);
        wassert(actual(api.enqi("l2")) == fortran::DbAPI::missing_int);
        wassert(actual(api.enqi("pindicator")) == 254);
        wassert(actual(api.enqi("p1")) == fortran::DbAPI::missing_int);
        wassert(actual(api.enqi("p2")) == fortran::DbAPI::missing_int);
    });

    this->add_method("delete_attrs_next_data", [](Fixture& f) {
        // Test deleting attributes after a next_data
        fortran::DbAPI api(f.tr, "write", "write", "write");
        populate_variables(api);

        // Query all variables and add attributes
        api.unsetall();
        wassert(actual(api.query_data()) == 2);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        api.seti("*B33007", 50);
        api.insert_attributes();
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        api.seti("*B33007", 60);
        api.insert_attributes();

        // Query all variables again and check that attributes are there
        api.unsetall();
        wassert(actual(api.query_data()) == 2);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(api.query_attributes()) == 1);
        wassert(actual(api.enqi("*B33007")) == 50);
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        wassert(actual(api.query_attributes()) == 1);
        wassert(actual(api.enqi("*B33007")) == 60);

        // Query all variables and delete all attributes
        api.unsetall();
        wassert(actual(api.query_data()) == 2);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        api.remove_attributes();
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        api.remove_attributes();

        // Query again and check that the attributes are gone
        api.unsetall();
        wassert(actual(api.query_data()) == 2);
        wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(api.query_attributes()) == 0);
        wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
        wassert(actual(api.query_attributes()) == 0);

        // The QC attrs record should be cleaned
        wassert(actual(api.enqi("*B33007")) == MISSING_INT);
    });

    this->add_method("perms_consistency", [](Fixture& f) {
        {
            fortran::DbAPI api(f.tr, "read", "read", "read");
            try
            {
                api.messages_open_input(
                    dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                    Encoding::BUFR);
            }
            catch (std::exception& e)
            {
                wassert(actual(e.what()).contains(
                    "must be called on a session with writable"));
            }
        }
        {
            fortran::DbAPI api(f.tr, "write", "read", "read");
            try
            {
                api.messages_open_input(
                    dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                    Encoding::BUFR);
            }
            catch (std::exception& e)
            {
                wassert(actual(e.what()).contains(
                    "must be called on a session with writable"));
            }
        }
        {
            fortran::DbAPI api(f.tr, "read", "add", "read");
            try
            {
                api.messages_open_input(
                    dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                    Encoding::BUFR);
            }
            catch (std::exception& e)
            {
                wassert(actual(e.what()).contains(
                    "must be called on a session with writable"));
            }
        }
        {
            fortran::DbAPI api(f.tr, "write", "add", "read");
            api.messages_open_input(
                dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                Encoding::BUFR);
        }
        {
            fortran::DbAPI api(f.tr, "write", "write", "read");
            api.messages_open_input(
                dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                Encoding::BUFR);
        }
        {
            fortran::DbAPI api(f.tr, "write", "add", "write");
            api.messages_open_input(
                dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                Encoding::BUFR);
        }
        {
            fortran::DbAPI api(f.tr, "write", "write", "write");
            api.messages_open_input(
                dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
                Encoding::BUFR);
        }
    });

    this->add_method("messages_read_messages", [](Fixture& f) {
        // 2 messages, 1 subset each
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(
            dballe::tests::datafile("bufr/synotemp.bufr").c_str(), "r",
            Encoding::BUFR);

        // At the beginning, the DB is empty
        wassert(actual(api.query_data()) == 0);

        // First message
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 88);

        // Second message
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 9);

        // End of messages
        api.remove_all();
        wassert(actual(api.messages_read_next()).isfalse());
        wassert(actual(api.query_data()) == 0);
    });
    this->add_method("messages_read_messages_stdin", [](Fixture& f) {
        fortran::DbAPI api(f.tr, "write", "write", "write");

        // Connect stdin to an input file
        TempStdin ts(tests::datafile("bufr/synotemp.bufr").c_str(), "rb");

        // 2 messages, 1 subset each
        api.messages_open_input("", "r", Encoding::BUFR);

        // At the beginning, the DB is empty
        wassert(actual(api.query_data()) == 0);

        // First message
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 88);

        // Second message
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 9);

        // End of messages
        api.remove_all();
        wassert(actual(api.messages_read_next()).isfalse());
        wassert(actual(api.query_data()) == 0);
    });
    this->add_method("messages_read_subsets", [](Fixture& f) {
        // 1 message, 6 subsets
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(
            dballe::tests::datafile("bufr/temp-gts2.bufr").c_str(), "r",
            Encoding::BUFR);

        // At the beginning, the DB is empty
        wassert(actual(api.query_data()) == 0);

        // 6 subsets
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 193);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 182);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 170);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 184);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 256);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 213);

        // End of messages
        api.remove_all();
        wassert(actual(api.messages_read_next()).isfalse());
        wassert(actual(api.query_data()) == 0);
    });
    this->add_method("messages_read_messages_subsets", [](Fixture& f) {
        // 2 messages, 2 subsets each
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(
            dballe::tests::datafile("bufr/db-messages1.bufr").c_str(), "r",
            Encoding::BUFR);

        // At the beginning, the DB is empty
        wassert(actual(api.query_data()) == 0);

        // 6 subsets
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 88);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 9);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 193);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 182);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 170);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 184);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 256);
        api.remove_all();
        wassert(actual(api.messages_read_next()).istrue());
        wassert(actual(api.query_data()) == 213);

        // End of messages
        api.remove_all();
        wassert(actual(api.messages_read_next()).isfalse());
        wassert(actual(api.query_data()) == 0);
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
            api.insert_data();
            // Instant wind speed, 10 meters above ground
            api.unsetb();
            api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
            api.setd("B11002", 2.4);
            api.insert_data();

            api.unsetall();
            api.messages_write_next("wmo");
        }

        // Read it back
        {
            fortran::DbAPI api(f.tr, "write", "write", "write");
            api.messages_open_input("test.bufr", "rb", Encoding::BUFR);

            wassert(actual(api.messages_read_next()).istrue());
            wassert(actual(api.query_data()) == 2);
            wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(api.enqd("B12101")) == 21.5);
            wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
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
                api.insert_data();
                // Instant wind speed, 10 meters above ground
                api.unsetb();
                api.setlevel(103, 10000, MISSING_INT, MISSING_INT);
                api.setd("B11002", 2.4);
                api.insert_data();

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
            wassert(actual(api.query_data()) == 2);
            wassert(actual(api.next_data()) == WR_VAR(0, 12, 101));
            wassert(actual(api.enqd("B12101")) == 21.5);
            wassert(actual(api.next_data()) == WR_VAR(0, 11, 2));
            wassert(actual(api.enqd("B11002")) == 2.4);

            wassert(actual(api.messages_read_next()).isfalse());
        }
    });
    this->add_method("messages_bug1", [](Fixture& f) {
        // Reproduce an issue reported by Paolo
        // 2 messages, 2 subsets each
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(
            dballe::tests::datafile("bufr/generic-bug20140312.bufr").c_str(),
            "r", Encoding::BUFR);
        wassert(actual(api.messages_read_next()) == 1);
        api.unsetall();
        api.set_station_context();
        wassert(actual(api.query_data()) == 3);
        // bug with mem DB: message: "enqi: B00000 (Context ID of the variable)
        // is not defined"
        wassert(actual(api.next_data()) == WR_VAR(0, 1, 194));
        wassert(actual(api.next_data()) == WR_VAR(0, 5, 1));
        wassert(actual(api.next_data()) == WR_VAR(0, 6, 1));
    });
    this->add_method("messages_bug2", [](Fixture& f) {
        // Reproduce an issue reported by Paolo
        // 2 messages, 2 subsets each
        fortran::DbAPI api(f.tr, "write", "write", "write");
        api.messages_open_input(
            dballe::tests::datafile("bufr/generic-bug20140326.bufr").c_str(),
            "r", Encoding::BUFR);
        wassert(actual(api.messages_read_next()) == 1);
        api.unsetall();
        api.set_station_context();
        wassert(actual(api.query_data()) == 5);
        wassert(actual(api.next_data()) == WR_VAR(0, 1, 19));
        // Bug: missing variable 000000 in table dballe
        wassert(actual(api.query_attributes()) == 0);
    });

    this->add_method("attr_reference_id", [](Fixture& f) {
        // Test attr_reference_id behaviour
        fortran::DbAPI api(f.tr, "write", "write", "write");

        // Try setting a context with a missing context_id
        {
            auto e = wassert_throws(wreport::error_consistency,
                                    api.setc("*var_related", "B07030"));
            wassert(actual(e.what()).matches(
                "\\*var_related set without context_id, or before any "
                "next_data or insert_data"));
        }

        // Initial data
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        // One station variable
        api.set_station_context();
        api.setd("B07030", 100.0);
        api.insert_data();
        // One data variable
        api.settimerange(254, 0, 0);
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
        api.setd("B12101", 21.5);
        api.insert_data();
        api.unsetall();

        // Query the station variable
        api.set_station_context();
        api.setc("var", "B07030");
        wassert(actual(api.query_data()) == 1);
        api.next_data();
        // Get its reference id
        int id_B07030 = wcallchecked(api.enqi("context_id"));
        wassert(actual(id_B07030) != MISSING_INT);
        // Get its attrs (none)
        wassert(actual(api.query_attributes()) == 0);
        // Set an attr
        api.seti("*B33007", 50);
        api.insert_attributes();
        // Query the variable again
        api.unsetall();
        api.set_station_context();
        api.setc("var", "B07030");
        wassert(actual(api.query_data()) == 1);
        wassert(api.next_data());
        // Read its attrs (ok)
        wassert(actual(api.query_attributes()) == 1);
        // Cannot manipulate station attrs setting *context_id

        // Query the variable
        api.unsetall();
        api.setc("var", "B12101");
        wassert(actual(api.query_data()) == 1);
        api.next_data();
        int ref_id = api.enqi("context_id");
        // Save its ref id (ok)
        wassert(actual(ref_id) != MISSING_INT);
        // Get its attrs (none)
        wassert(actual(api.query_attributes()) == 0);
        // Set an attr
        api.seti("*B33007", 50);
        api.insert_attributes();
        // Query the variable again
        api.unsetall();
        api.setc("var", "B12101");
        wassert(actual(api.query_data()) == 1);
        api.next_data();
        // Read its attrs (ok)
        wassert(actual(api.query_attributes()) == 1);
        // Set *context_id and *varid
        // Read attrs (ok)
        api.seti("*context_id", ref_id);
        api.setc("*var_related", "B12101");
        wassert(actual(api.query_attributes()) == 1);
        // Query the variable again, to delete all attributes
        api.unsetall();
        api.setc("var", "B12101");
        wassert(actual(api.query_data()) == 1);
        api.next_data();
        api.remove_attributes();
        wassert(actual(api.query_attributes()) == 0);
        // Try to delete by *context_id and *varid: it works
        api.seti("*context_id", ref_id);
        api.setc("*var_related", "B12101");
        api.remove_attributes();
    });

    this->add_method("attrs_bug1", [](Fixture& f) {
        // Reproduce a bug when setting attributes
        using namespace dballe::fortran;
        fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
        dbapi0.reinit_db();

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
        dbapi0.insert_data();

        // Add attributes
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.setc("*var_related", "B13003");
        dbapi0.insert_attributes();

        // Read them back
        dbapi0.unsetall();
        dbapi0.setc("var", "B13003");
        wassert(actual(dbapi0.query_data()) == 1);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 13, 3));
        wassert(actual(dbapi0.query_attributes()) == 3);
    });

    this->add_method("stationdata_bug1", [](Fixture& f) {
        // Reproduce a bug handling station data
        {
            fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
            dbapi0.reinit_db();
            // Copy a message using the API
            dbapi0.messages_open_input(
                dballe::tests::datafile("bufr/generic-bug20140403.bufr")
                    .c_str(),
                "r", Encoding::BUFR);
            dbapi0.messages_open_output("test.bufr", "w", Encoding::BUFR);
            wassert(actual(dbapi0.messages_read_next()) == 1);
            // dbapi0.set_station_context();
            dbapi0.messages_write_next("generic");
            dbapi0.remove_all();
            wassert(actual(dbapi0.messages_read_next()) == 1);
            // dbapi0.set_station_context();
            dbapi0.messages_write_next("generic");
            dbapi0.remove_all();
            wassert(actual(dbapi0.messages_read_next()) == 0);
        }

        // TODO: decide what is the expected behaviour for exporting only
        // station values

        // // Compare the two messages
        // std::unique_ptr<Msgs> msgs1 =
        // read_msgs("bufr/generic-bug20140403.bufr", Encoding::BUFR);
        // std::unique_ptr<Msgs> msgs2 = read_msgs("./test.bufr",
        // Encoding::BUFR); unsigned diffs = msgs1->diff(*msgs2); if (diffs)
        // dballe::tests::track_different_msgs(*msgs1, *msgs2, "apicopy");
        // wassert(actual(diffs) == 0);
    });
    this->add_method("segfault1", [](Fixture& f) {
        // Reproduce a segfault with mem:
        fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
        dbapi0.seti("lat", 4500000);
        dbapi0.seti("lon", 1300000);
        dbapi0.setc("rep_memo", "generic");
        dbapi0.set_station_context();
        dbapi0.setc("B12102", "26312");
        wassert(dbapi0.insert_data());
        dbapi0.setc("*B33194", "50");
        wassert(dbapi0.insert_attributes());
    });

    this->add_method("issue45", [](Fixture& f) {
        using namespace dballe::fortran;
        fortran::DbAPI dbapi0(f.tr, "write", "write", "write");
        dbapi0.reinit_db();
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
        dbapi0.insert_data();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.insert_attributes();
        dbapi0.seti("B12101", 27315);
        dbapi0.insert_data();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.insert_attributes();
        // error: cannot insert attributes for variable 000000: no data id given
        // or found from last insert_data()
    });

    this->add_method("issue52", [](Fixture& f) {
        using namespace wreport;

        std::string fname = dballe::tests::datafile("bufr/issue52.bufr");
        fortran::DbAPI dbapi(f.tr, "write", "write", "write");
        wassert(dbapi.messages_open_input(fname.c_str(), "r", Encoding::BUFR,
                                          true));
        wassert(actual(dbapi.messages_read_next()).istrue());
        wassert(actual(dbapi.messages_read_next()).istrue());

        // error: no year information found in message to import
    });

    this->add_method("perf_data", [](Fixture& f) {
        // Test insert_data anaid
        fortran::DbAPI api(f.tr, "write", "write", "write");

        // Run a insert_data
        f.tr->trc->clear();
        api.setd("lat", 44.5);
        api.setd("lon", 11.5);
        api.setc("rep_memo", "synop");
        api.setdate(2013, 4, 25, 12, 0, 0);
        api.setlevel(1, MISSING_INT, MISSING_INT, MISSING_INT);
        api.settimerange(254, 0, 0);
        api.setd("B10004", 100000.0);
        api.insert_data(); // Pressure at ground level
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
        wassert(actual(api.query_data()) == 1);
        stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 2);
        wassert(actual(stats.rows) == 2);

        // Query stations only
        f.tr->trc->clear();
        api.unsetall();
        wassert(actual(api.query_stations()) == 1);
        stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 1);
        wassert(actual(stats.rows) == 1);
    });

    this->add_method("perf_read_attrs", [](Fixture& f) {
        auto msgs = read_msgs("bufr/temp-gts1.bufr", Encoding::BUFR,
                              impl::ImporterOptions("accurate"));
        wassert(f.tr->import_messages(msgs, DBImportOptions::defaults));
        wassert(f.tr->clear_cached_state());

        // Readonly test session
        fortran::DbAPI api(f.tr, "read", "read", "read");

        // Read all station data and their attributes
        f.tr->trc->clear();
        api.unsetall();
        api.set_station_context();
        wassert(actual(api.query_data()) == 7);
        while (api.next_data())
            wassert(api.query_attributes());

        // Check number of queries
        v7::trace::Aggregate stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 8);
        wassert(actual(stats.rows) == 14);

        // Read all measured data and their attributes
        f.tr->trc->clear();
        api.unsetall();
        wassert(actual(api.query_data()) == 550);
        while (api.next_data())
            wassert(api.query_attributes());

        // Check number of queries
        stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 552);
        wassert(actual(stats.rows) == 1156);

        // Repeat with query=attrs

        // Read all station data and their attributes
        f.tr->trc->clear();
        api.unsetall();
        api.set_station_context();
        api.setc("query", "attrs");
        wassert(actual(api.query_data()) == 7);
        while (api.next_data())
            wassert(api.query_attributes());

        // Check number of queries
        stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 1);
        wassert(actual(stats.rows) == 7);

        // Read all measured data and their attributes
        f.tr->trc->clear();
        api.unsetall();
        api.setc("query", "attrs");
        wassert(actual(api.query_data()) == 550);
        while (api.next_data())
            wassert(api.query_attributes());

        // Check number of queries
        stats = f.tr->trc->aggregate("select");
        wassert(actual(stats.count) == 2);
        wassert(actual(stats.rows) == 606);
    });

    this->add_method("negative_values", [](Fixture& f) {
        // See issue #144
        {
            core::Data vals;
            vals.station.report = "synop";
            vals.station.coords = Coords(44.10, 11.50);
            vals.station.ident  = "foo";
            vals.level          = Level(1);
            vals.trange         = Trange::instant();
            vals.datetime       = Datetime(2015, 4, 25, 12, 30, 45);
            vals.values.set("B07030", -5.2);
            vals.values.set("B07031", 295.1);
            f.tr->insert_data(vals);
            f.tr->insert_station_data(vals);
        }

        {
            fortran::DbAPI db(f.tr, "read", "read", "read");
            db.set_station_context();
            wassert(actual(db.query_data()) == 2u);
            wassert(actual(db.next_data()) == WR_VAR(0, 7, 30));
            wassert(actual(db.enqi("B07030")) == -52);
            wassert(actual(db.enqd("B07030")) == -5.2);
            wassert(actual(db.test_enqc("B07030", 10)) == "-52");
            wassert(actual(db.next_data()) == WR_VAR(0, 7, 31));
            wassert(actual(db.enqi("B07031")) == 2951);
            wassert(actual(db.enqd("B07031")) == 295.1);
            wassert(actual(db.test_enqc("B07031", 10)) == "2951");
        }

        {
            fortran::DbAPI db(f.tr, "read", "read", "read");
            wassert(actual(db.query_data()) == 2u);
            wassert(actual(db.next_data()) == WR_VAR(0, 7, 30));
            wassert(actual(db.enqi("B07030")) == -52);
            wassert(actual(db.enqd("B07030")) == -5.2);
            wassert(actual(db.test_enqc("B07030", 10)) == "-52");
            wassert(actual(db.next_data()) == WR_VAR(0, 7, 31));
            wassert(actual(db.enqi("B07031")) == 2951);
            wassert(actual(db.enqd("B07031")) == 295.1);
            wassert(actual(db.test_enqc("B07031", 10)) == "2951");
        }
    });
}

template <typename DB> void CommitTests<DB>::register_tests()
{

    this->add_method("attr_insert", [](Fixture& f) {
        // Reproduce a problem with attribute insert when inserting a variable
        // that already exists in the database
        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
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
            pre.insert_data();
            pre.commit();
        }

        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
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
            dbapi0.insert_data();
            dbapi0.seti("*B33192", 0);
            dbapi0.setc("*var_related", "B12101");
            dbapi0.insert_attributes();
        }
    });

    this->add_method("issue73", [](Fixture& f) {
        using namespace wreport;

        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI dbapi(tr, "write", "write", "write");
            wassert(populate_variables(dbapi));
            wassert(dbapi.commit());
        }

        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI dbapi(tr, "read", "read", "read");
            dbapi.setc("varlist", "B10004,B12101,B12103");
            wassert(actual(dbapi.query_data()) == 1);
        }
    });

    this->add_method("issue75", [](Fixture& f) {
        using namespace wreport;

        std::string fname = dballe::tests::datafile("bufr/issue75.bufr");
        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI dbapi(tr, "write", "write", "write");
            wassert(dbapi.messages_open_input(fname.c_str(), "r",
                                              Encoding::BUFR, true));
            wassert(actual(dbapi.messages_read_next()).istrue());
            wassert(actual(dbapi.messages_read_next()).istrue());
            wassert(actual(dbapi.messages_read_next()).istrue());
            wassert(actual(dbapi.messages_read_next()).istrue());
            wassert(dbapi.commit());
        }

        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI dbapi(tr, "read", "read", "read");
            dbapi.setc("rep_memo", "temp");
            dbapi.set_station_context();
            dbapi.setc("varlist", "B07001");
            wassert(actual(dbapi.query_data()) == 0);
            dbapi.setc("varlist", "B07030");
            wassert(actual(dbapi.query_data()) == 1);
        }
    });

    this->add_method("transactions", [](Fixture& f) {
        // FIXME: move check to fortran and python bindings
        // Write, do not commit
        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI api(tr, "write", "write", "write");
            wassert(actual(api.query_data()) == 0);
            api.setd("lat", 44.5);
            api.setd("lon", 11.5);
            api.setc("rep_memo", "synop");
            api.settimerange(254, 0, 0);
            api.setdate(2013, 4, 25, 12, 0, 0);
            api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
            api.setd("B12101", 21.5);
            api.insert_data();
            wassert(actual(api.query_data()) == 1);
        }

        // Write, commit
        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI api(tr, "write", "write", "write");
            wassert(actual(api.query_data()) == 0);
            api.setd("lat", 44.5);
            api.setd("lon", 11.5);
            api.setc("rep_memo", "synop");
            api.settimerange(254, 0, 0);
            api.setdate(2013, 4, 25, 12, 0, 0);
            api.setlevel(103, 2000, MISSING_INT, MISSING_INT);
            api.setd("B12101", 21.5);
            api.insert_data();
            wassert(actual(api.query_data()) == 1);
            wassert(api.commit());
        }

        // This time, data was written
        {
            auto tr =
                dynamic_pointer_cast<db::Transaction>(f.db->transaction());
            fortran::DbAPI api(tr, "write", "write", "write");
            wassert(actual(api.query_data()) == 1);
        }
    });

    this->add_method("insert_block", [](Fixture& f) {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi0(tr, "write", "write", "write");
        dbapi0.reinit_db();
        dbapi0.setd("lat", -45.678902);
        dbapi0.setd("lon", -12.345600);
        dbapi0.setc("rep_memo", "synop");
        dbapi0.seti("block", 1);
        dbapi0.set_station_context();
        dbapi0.insert_data();
        wassert(actual(dbapi0.query_stations()) == 1);
    });

    this->add_method("query_attr_values", [](Fixture& f) {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi0(tr, "write", "write", "write");
        dbapi0.reinit_db();
        dbapi0.unsetall();
        dbapi0.seti("lat", 4500000);
        dbapi0.seti("lon", 1000000);
        dbapi0.setc("rep_memo", "generic");
        dbapi0.setdate(2014, 1, 6, 18, 0, 0);
        dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
        dbapi0.settimerange(4, 3600, 7200);
        dbapi0.seti("B13003", 85);
        dbapi0.insert_data();
        dbapi0.unsetb();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.insert_attributes();
        dbapi0.seti("B12101", 27315);
        dbapi0.insert_data();
        dbapi0.unsetb();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.insert_attributes();
        dbapi0.unsetall();
        dbapi0.setc("varlist", "B12101");
        wassert(actual(dbapi0.query_data()) == 1);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 12, 101));
        dbapi0.setc("*varlist", "*B33193,*B33194");
        wassert(actual(dbapi0.query_attributes()) == 2);
        wassert(actual(dbapi0.next_attribute()) == "*B33193");
        wassert(dbapi0.enqi("*B33193"));
        wassert(dbapi0.enqb("*B33193"));
        wassert(dbapi0.enqr("*B33193"));
        wassert(dbapi0.enqd("*B33193"));
        char sres[1];
        wassert(dbapi0.enqc("*B33193", sres, 1));
        // error: cannot parse a Varcode out of '*B33193'
    });

    this->add_method("query_attr_filtered", [](Fixture& f) {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi0(tr, "write", "write", "write");
        dbapi0.reinit_db();
        dbapi0.unsetall();
        dbapi0.seti("lat", 4500000);
        dbapi0.seti("lon", 1000000);
        dbapi0.setc("rep_memo", "generic");
        dbapi0.setdate(2014, 1, 6, 18, 0, 0);
        dbapi0.setlevel(105, 2000, 2147483647, 2147483647);
        dbapi0.settimerange(4, 3600, 7200);
        dbapi0.seti("B13003", 85);
        dbapi0.insert_data();
        dbapi0.unsetb();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.insert_attributes();
        dbapi0.seti("B12101", 27315);
        dbapi0.insert_data();
        dbapi0.unsetb();
        dbapi0.setd("*B33192", 30.000000);
        dbapi0.seti("*B33193", 50);
        dbapi0.setd("*B33194", 70.000000);
        dbapi0.insert_attributes();
        dbapi0.unsetall();
        dbapi0.setc("varlist", "B12101");
        dbapi0.setc("*varlist", "*B33193,*B33194");
        wassert(actual(dbapi0.query_data()) == 1);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 12, 101));
        wassert(actual(dbapi0.query_attributes()) == 2);
        wassert(actual(dbapi0.next_attribute()) == "*B33193");
        wassert(actual(dbapi0.next_attribute()) == "*B33194");
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
        dbapi0.reinit_db();

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
        dbapi0.insert_data();

        // Remove one. In a previous implementation, var and varlist both edited
        // Query::varcodes, and unsetting one had the effect of unsetting the
        // other
        dbapi0.unsetall();
        dbapi0.setc("var", "B12101");
        dbapi0.unset("varlist");
        dbapi0.remove_data();

        dbapi0.unsetall();
        dbapi0.setc("var", "B12102");
        dbapi0.unset("varlist");
        dbapi0.seti("lat", 4500000);
        dbapi0.seti("lon", 1100000);
        dbapi0.setc("rep_memo", "generic");
        dbapi0.setdate(2014, 1, 6, 18, 0, 0);
        dbapi0.setlevel(105, 2000, API::missing_int, API::missing_int);
        dbapi0.settimerange(4, 3600, 7200);
        dbapi0.remove_data();

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
        wassert(actual(dbapi0.query_data()) == 1);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 12, 102));
        ;
    });

    this->add_method("issue144", [](Fixture& f) {
        auto tr = dynamic_pointer_cast<db::Transaction>(f.db->transaction());
        fortran::DbAPI dbapi0(tr, "write", "write", "write");
        wassert(dbapi0.messages_open_input(
            dballe::tests::datafile("bufr/camse-rad1havg.bufr").c_str(), "r",
            Encoding::BUFR, true));
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == true);
        wassert(actual(dbapi0.messages_read_next()) == false);

        // Query station data
        wassert(dbapi0.unsetall());
        wassert(dbapi0.set_station_context());
        wassert(actual(dbapi0.query_data()) == 6);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 1, 19));
        wassert(actual(dbapi0.test_enqc("B01019", 255)) == "Camse");
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 1, 194));
        wassert(actual(dbapi0.test_enqc("B01194", 255)) == "locali");
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 25));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 5, 1));
        wassert(actual(dbapi0.test_enqc("B05001", 255)) == "4460016");
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 6, 1));
        wassert(actual(dbapi0.test_enqc("B06001", 255)) == "1207738");
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 7, 30));
        wassert(actual(dbapi0.test_enqc("B07030", 255)) == "-10"); // New: 10
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 7, 31));
        wassert(actual(dbapi0.test_enqc("B07031", 255)) == "0");
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        // Query data for B14198
        int year, month, day, hour, min, sec;
        int ltype1, l1, ltype2, l2;
        int pind, p1, p2;

        wassert(dbapi0.unsetall());
        wassert(dbapi0.setc("varlist", "B14198"));
        wassert(dbapi0.unset("*varlist"));
        wassert(actual(dbapi0.query_data()) == 24);
        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.enqi("lat")) == 4460016);
        wassert(actual(dbapi0.enqi("lon")) == 1207738);
        wassert_false(dbapi0.test_enqc("ident", 255));
        wassert(actual(dbapi0.test_enqc("rep_memo", 255)) == "locali");
        wassert(actual(dbapi0.enqi("priority")) == 1001);

        wassert(dbapi0.enqdate(year, month, day, hour, min, sec));
        wassert(actual(year) == 2018);
        wassert(actual(month) == 12);
        wassert(actual(day) == 8);
        wassert(actual(hour) == 1);
        wassert(actual(min) == 0);
        wassert(actual(sec) == 0);

        wassert(dbapi0.enqlevel(ltype1, l1, ltype2, l2));
        wassert(actual(ltype1) == 1);
        wassert(actual(l1) == fortran::API::missing_int);
        wassert(actual(ltype2) == fortran::API::missing_int);
        wassert(actual(l2) == fortran::API::missing_int);

        wassert(dbapi0.enqtimerange(pind, p1, p2));
        wassert(actual(pind) == 0);
        wassert(actual(p1) == 0);
        wassert(actual(p2) == 3600);

        wassert(actual(dbapi0.test_enqc("var", 255)) == "B14198");
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "1");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "5");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "44");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "141");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "238");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "339");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "294");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "202");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "91");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "9");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-1");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-2");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-1");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "0");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-2");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-2");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-3");

        wassert(actual(dbapi0.next_data()) == WR_VAR(0, 14, 198));
        wassert(actual(dbapi0.test_enqc("B14198", 255)) == "-4");
    });
}

} // namespace
