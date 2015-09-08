#include "db/tests.h"
#include "db/odbc/internals.h"
#include <sql.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct ConnectorFixture : public Fixture
{
    ODBCConnection conn;

    ConnectorFixture()
    {
        conn.connect_test();
    }

    void test_setup() override
    {
        Fixture::test_setup();
        conn.drop_table_if_exists("dballe_test");
        conn.exec("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};

class Tests : public FixtureTestCase<ConnectorFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("query_int", [](Fixture& f) {
            // Test querying int values
            auto& c = f.conn;
            c.exec("INSERT INTO dballe_test VALUES (42)");

            auto s = c.odbcstatement("SELECT val FROM dballe_test");
            int val = 0;
            s->bind_out(1, val);
            s->execute();
            unsigned count = 0;
            while (s->fetch())
                ++count;
            wassert(actual(val) == 42);
            wassert(actual(count) == 1);
        });
        add_method("query_int_null", [](Fixture& f) {
            // Test querying int values, with indicators
            auto& c = f.conn;
            c.exec("INSERT INTO dballe_test VALUES (42)");

            auto s = c.odbcstatement("SELECT val FROM dballe_test");
            int val = 0;
            SQLLEN ind = 0;
            s->bind_out(1, val, ind);
            s->execute();
            unsigned count = 0;
            while (s->fetch())
                ++count;
            wassert(actual(val) == 42);
            wassert(actual(ind) != SQL_NULL_DATA);
            wassert(actual(count) == 1);
        });
        add_method("query_unsigned", [](Fixture& f) {
            // Test querying unsigned values
            auto& c = f.conn;
            c.exec("INSERT INTO dballe_test VALUES (42)");

            auto s = c.odbcstatement("SELECT val FROM dballe_test");
            unsigned val = 0;
            s->bind_out(1, val);
            s->execute();
            unsigned count = 0;
            while (s->fetch())
                ++count;
            wassert(actual(val) == 42);
            wassert(actual(count) == 1);
        });
        add_method("query_unsigned_null", [](Fixture& f) {
            // Test querying unsigned values, with indicators
            auto& c = f.conn;
            c.exec("INSERT INTO dballe_test VALUES (42)");

            auto s = c.odbcstatement("SELECT val FROM dballe_test");
            unsigned val = 0;
            SQLLEN ind = 0;
            s->bind_out(1, val, ind);
            s->execute();
            unsigned count = 0;
            while (s->fetch())
                ++count;
            wassert(actual(val) == 42);
            wassert(actual(ind) != SQL_NULL_DATA);
            wassert(actual(count) == 1);
        });
        add_method("query_unsigned_short", [](Fixture& f) {
            // Test querying unsigned short values
            auto& c = f.conn;
            c.exec("INSERT INTO dballe_test VALUES (42)");
            auto s = c.odbcstatement("SELECT val FROM dballe_test");
            unsigned short val = 0;
            s->bind_out(1, val);
            s->execute();
            unsigned count = 0;
            while (s->fetch())
                ++count;
            wassert(actual(val) == 42);
            wassert(actual(count) == 1);
        });
        add_method("has_tables", [](Fixture& f) {
            // Test has_tables
            auto& c = f.conn;
            wassert(actual(c.has_table("this_should_not_exist")).isfalse());
            wassert(actual(c.has_table("dballe_test")).istrue());
        });
        add_method("settings", [](Fixture& f) {
            // Test settings
            auto& c = f.conn;
            c.drop_table_if_exists("dballe_settings");
            wassert(actual(c.has_table("dballe_settings")).isfalse());

            wassert(actual(c.get_setting("test_key")) == "");

            c.set_setting("test_key", "42");
            wassert(actual(c.has_table("dballe_settings")).istrue());

            wassert(actual(c.get_setting("test_key")) == "42");
        });
    }
} test("db_internals_odbc");

}
