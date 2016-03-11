#include "db/tests.h"
#include "sql/postgresql.h"

using namespace dballe;
using namespace dballe::sql;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct ConnectorFixture : public Fixture
{
    PostgreSQLConnection conn;

    ConnectorFixture()
    {
        conn.open_test();
    }

    void test_setup()
    {
        Fixture::test_setup();
        conn.drop_table_if_exists("dballe_test");
        conn.exec_no_data("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};

class Tests : public FixtureTestCase<ConnectorFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("int", [](Fixture& f) {
            // Test querying int values
            auto& conn = f.conn;
            conn.exec_no_data("INSERT INTO dballe_test VALUES (1)");
            conn.exec_no_data("INSERT INTO dballe_test VALUES (2)");

            auto s = conn.exec("SELECT val FROM dballe_test");
            wassert(actual(s.rowcount()) == 2);

            int val = 0;
            for (unsigned row = 0; row < 2; ++row)
                val += s.get_int4(row, 0);

            wassert(actual(val) == 3);
        });

        add_method("int_null", [](Fixture& f) {
            // Test querying int values, with potential NULLs
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_testnull");
            conn.exec_no_data("CREATE TABLE dballe_testnull (val INTEGER)");
            conn.exec_no_data("INSERT INTO dballe_testnull VALUES (NULL)");
            conn.exec_no_data("INSERT INTO dballe_testnull VALUES (42)");

            auto s = conn.exec("SELECT val FROM dballe_testnull");
            wassert(actual(s.rowcount()) == 2);

            int val = 0;
            unsigned countnulls = 0;
            for (unsigned row = 0; row < 2; ++row)
            {
                if (s.is_null(row, 0))
                    ++countnulls;
                else
                    val += s.get_int4(row, 0);
            }

            wassert(actual(val) == 42);
            wassert(actual(countnulls) == 1);
        });

        add_method("unsigned", [](Fixture& f) {
            // Test querying unsigned values
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_testbig");
            conn.exec_no_data("CREATE TABLE dballe_testbig (val BIGINT)");
            conn.exec_no_data("INSERT INTO dballe_testbig VALUES (x'FFFFFFFE'::bigint)");

            auto s = conn.exec("SELECT val FROM dballe_testbig");
            wassert(actual(s.rowcount()) == 1);

            unsigned val = 0;
            for (unsigned row = 0; row < 1; ++row)
                val += s.get_int8(row, 0);
            wassert(actual(val) == 0xFFFFFFFE);
        });

        add_method("unsigned_short", [](Fixture& f) {
            // Test querying unsigned short values
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_testshort");
            conn.exec_no_data("CREATE TABLE dballe_testshort (val SMALLINT)");
            conn.exec_no_data("INSERT INTO dballe_testshort VALUES (123)");

            auto s = conn.exec("SELECT val FROM dballe_testshort");
            wassert(actual(s.rowcount()) == 1);

            Varcode val = 0;
            for (unsigned row = 0; row < 1; ++row)
                val += s.get_int2(row, 0);
            wassert(actual(val) == 123);
        });

        add_method("has_tables", [](Fixture& f) {
            // Test has_tables
            auto& conn = f.conn;
            wassert(actual(conn.has_table("this_should_not_exist")).isfalse());
            wassert(actual(conn.has_table("dballe_test")).istrue());
        });

        add_method("settings", [](Fixture& f) {
            // Test settings
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_settings");
            wassert(actual(conn.has_table("dballe_settings")).isfalse());

            wassert(actual(conn.get_setting("test_key")) == "");

            conn.set_setting("test_key", "42");
            wassert(actual(conn.has_table("dballe_settings")).istrue());

            wassert(actual(conn.get_setting("test_key")) == "42");
            wassert(actual(conn.get_setting("test_key1")) == "");
        });

        add_method("autoid", [](Fixture& f) {
            // Test auto_increment
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_testai");
            conn.exec_no_data("CREATE TABLE dballe_testai (id SERIAL PRIMARY KEY, val INTEGER)");
            auto r1 = conn.exec_one_row("INSERT INTO dballe_testai (id, val) VALUES (DEFAULT, 42) RETURNING id");
            wassert(actual(r1.get_int4(0, 0)) == 1);
            auto r2 = conn.exec_one_row("INSERT INTO dballe_testai (id, val) VALUES (DEFAULT, 43) RETURNING id");
            wassert(actual(r2.get_int4(0, 0)) == 2);
        });

        add_method("prepared", [](Fixture& f) {
            // Test prepared statements
            auto& conn = f.conn;
            conn.exec_no_data("INSERT INTO dballe_test VALUES (1)");
            conn.exec_no_data("INSERT INTO dballe_test VALUES (2)");

            conn.prepare("db_postgresql_internals_9", "SELECT val FROM dballe_test");

            auto s = conn.exec_prepared("db_postgresql_internals_9");
            wassert(actual(s.rowcount()) == 2);

            int val = 0;
            for (unsigned row = 0; row < 2; ++row)
                val += s.get_int4(row, 0);

            wassert(actual(val) == 3);
        });

        add_method("prepared_int", [](Fixture& f) {
            // Test prepared statements with int arguments
            auto& conn = f.conn;
            conn.exec_no_data("INSERT INTO dballe_test VALUES (1)");
            conn.exec_no_data("INSERT INTO dballe_test VALUES (2)");
            conn.prepare("db_postgresql_internals_10", "SELECT val FROM dballe_test WHERE val=$1::int4");

            auto res = conn.exec_prepared("db_postgresql_internals_10", 1);
            wassert(actual(res.rowcount()) == 1);
            wassert(actual(res.get_int4(0, 0)) == 1);
        });

        add_method("prepared_string", [](Fixture& f) {
            // Test prepared statements with string arguments
            auto& conn = f.conn;
            conn.drop_table_if_exists("db_postgresql_internals_11");
            conn.exec_no_data("CREATE TABLE db_postgresql_internals_11 (val TEXT)");
            conn.exec_no_data("INSERT INTO db_postgresql_internals_11 VALUES ('foo')");
            conn.exec_no_data("INSERT INTO db_postgresql_internals_11 VALUES ('bar')");
            conn.prepare("db_postgresql_internals_11_select", "SELECT val FROM db_postgresql_internals_11 WHERE val=$1::text");

            auto res1 = conn.exec_prepared("db_postgresql_internals_11_select", "foo");
            wassert(actual(res1.rowcount()) == 1);
            wassert(actual(res1.get_string(0, 0)) == "foo");

            auto res2 = conn.exec_prepared("db_postgresql_internals_11_select", string("foo"));
            wassert(actual(res2.rowcount()) == 1);
            wassert(actual(res2.get_string(0, 0)) == "foo");
        });

        add_method("prepared_datetime", [](Fixture& f) {
            // Test prepared statements with datetime arguments
            auto& conn = f.conn;
            conn.drop_table_if_exists("db_postgresql_internals_12");
            conn.exec_no_data("CREATE TABLE db_postgresql_internals_12 (val TIMESTAMP)");
            conn.exec_no_data("INSERT INTO db_postgresql_internals_12 VALUES ('2015-04-01 12:30:45')");
            conn.exec_no_data("INSERT INTO db_postgresql_internals_12 VALUES ('1945-04-25 08:10:20')");
            conn.prepare("db_postgresql_internals_12_select", "SELECT val FROM db_postgresql_internals_12 WHERE val=$1::timestamp");

            auto res1 = conn.exec("SELECT val FROM db_postgresql_internals_12 WHERE val=TIMESTAMP '2015-04-01 12:30:45'");
            wassert(actual(res1.rowcount()) == 1);
            wassert(actual(res1.get_timestamp(0, 0)) == Datetime(2015, 4, 1, 12, 30, 45));

            auto res2 = conn.exec_prepared("db_postgresql_internals_12_select", Datetime(2015, 4, 1, 12, 30, 45));
            wassert(actual(res2.rowcount()) == 1);
            wassert(actual(res2.get_timestamp(0, 0)) == Datetime(2015, 4, 1, 12, 30, 45));

            auto res3 = conn.exec("SELECT val FROM db_postgresql_internals_12 WHERE val=TIMESTAMP '1945-04-25 08:10:20'");
            wassert(actual(res3.rowcount()) == 1);
            wassert(actual(res3.get_timestamp(0, 0)) == Datetime(1945, 4, 25, 8, 10, 20));

            auto res4 = conn.exec_prepared("db_postgresql_internals_12_select", Datetime(1945, 4, 25, 8, 10, 20));
            wassert(actual(res4.rowcount()) == 1);
            wassert(actual(res4.get_timestamp(0, 0)) == Datetime(1945, 4, 25, 8, 10, 20));
        });
    }
} test("db_postgresql_internals");

}
