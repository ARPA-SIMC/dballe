#include "db/tests.h"
#include "sql/mysql.h"

using namespace std;
using namespace dballe;
using namespace dballe::sql;
using namespace dballe::tests;
using namespace wreport;

namespace {

struct ConnectorFixture : public Fixture
{
    MySQLConnection conn;
    bool conn_open = false;

    ConnectorFixture(const char* backend)
    {
    }

    void test_setup()
    {
        Fixture::test_setup();
        if (!BaseDBFixture::has_driver("MYSQL")) throw TestSkipped();
        if (!conn_open)
        {
            conn.open_test();
            conn_open = true;
        }
        conn.drop_table_if_exists("dballe_test");
        conn.exec_no_data("CREATE TABLE dballe_test (val INTEGER NOT NULL)");
    }
};

class Tests : public DBFixtureTestCase<ConnectorFixture>
{
    using DBFixtureTestCase::DBFixtureTestCase;

    void register_tests() override
    {
        add_method("parse_url", [](Fixture& f) {
            // Test parsing urls
            mysql::ConnectInfo info;

            info.parse_url("mysql:");
            wassert(actual(info.host) == "");
            wassert(actual(info.user) == "");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).isfalse());
            wassert(actual(info.dbname) == "");
            wassert(actual(info.port) == 0);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://");
            wassert(actual(info.host) == "");
            wassert(actual(info.user) == "");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).isfalse());
            wassert(actual(info.dbname) == "");
            wassert(actual(info.port) == 0);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost/");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).isfalse());
            wassert(actual(info.dbname) == "");
            wassert(actual(info.port) == 0);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost:1234/");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).isfalse());
            wassert(actual(info.dbname) == "");
            wassert(actual(info.port) == 1234);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost:1234/?user=enrico");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "enrico");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).isfalse());
            wassert(actual(info.dbname) == "");
            wassert(actual(info.port) == 1234);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost:1234/foo?user=enrico");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "enrico");
            wassert(actual(info.has_passwd).isfalse());
            wassert(actual(info.passwd) == "");
            wassert(actual(info.has_dbname).istrue());
            wassert(actual(info.dbname) == "foo");
            wassert(actual(info.port) == 1234);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost:1234/foo?user=enrico&password=secret");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "enrico");
            wassert(actual(info.has_passwd).istrue());
            wassert(actual(info.passwd) == "secret");
            wassert(actual(info.has_dbname).istrue());
            wassert(actual(info.dbname) == "foo");
            wassert(actual(info.port) == 1234);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql://localhost/foo?user=enrico&password=secret");
            wassert(actual(info.host) == "localhost");
            wassert(actual(info.user) == "enrico");
            wassert(actual(info.has_passwd).istrue());
            wassert(actual(info.passwd) == "secret");
            wassert(actual(info.has_dbname).istrue());
            wassert(actual(info.dbname) == "foo");
            wassert(actual(info.port) == 0);
            wassert(actual(info.unix_socket) == "");

            info.parse_url("mysql:///foo?user=enrico&password=secret");
            wassert(actual(info.host) == "");
            wassert(actual(info.user) == "enrico");
            wassert(actual(info.has_passwd).istrue());
            wassert(actual(info.passwd) == "secret");
            wassert(actual(info.has_dbname).istrue());
            wassert(actual(info.dbname) == "foo");
            wassert(actual(info.port) == 0);
            wassert(actual(info.unix_socket) == "");
        });
        add_method("query_int", [](Fixture& f) {
            // Test querying int values
            using namespace mysql;

            f.conn.exec_no_data("INSERT INTO dballe_test VALUES (1)");
            f.conn.exec_no_data("INSERT INTO dballe_test VALUES (2)");

            auto res = f.conn.exec_store("SELECT val FROM dballe_test");

            int val = 0;
            unsigned count = 0;
            while (Row row = res.fetch())
            {
                val += row.as_int(0);
                ++count;
            }
            wassert(actual(count) == 2);
            wassert(actual(val) == 3);
        });
        add_method("query_int_null", [](Fixture& f) {
            // Test querying int values, with potential NULLs
            f.conn.drop_table_if_exists("dballe_testnull");
            f.conn.exec_no_data("CREATE TABLE dballe_testnull (val INTEGER)");
            f.conn.exec_no_data("INSERT INTO dballe_testnull VALUES (NULL)");
            f.conn.exec_no_data("INSERT INTO dballe_testnull VALUES (42)");

            auto res = f.conn.exec_store("SELECT val FROM dballe_testnull");
            wassert(actual(res.rowcount()) == 2);

            int val = 0;
            unsigned count = 0;
            unsigned countnulls = 0;
            while (auto row = res.fetch())
            {
                if (row.isnull(0))
                    ++countnulls;
                else
                    val += row.as_int(0);
                ++count;
            }

            wassert(actual(val) == 42);
            wassert(actual(count) == 2);
            wassert(actual(countnulls) == 1);
        });
        add_method("query_unsigned", [](Fixture& f) {
            // Test querying unsigned values
            f.conn.drop_table_if_exists("dballe_testbig");
            f.conn.exec_no_data("CREATE TABLE dballe_testbig (val BIGINT)");
            f.conn.exec_no_data("INSERT INTO dballe_testbig VALUES (0xFFFFFFFE)");

            auto res = f.conn.exec_store("SELECT val FROM dballe_testbig");
            wassert(actual(res.rowcount()) == 1);

            unsigned val = 0;
            unsigned count = 0;
            while (auto row = res.fetch())
            {
                val += row.as_unsigned(0);
                ++count;
            }
            wassert(actual(val) == 0xFFFFFFFE);
            wassert(actual(count) == 1);
        });
        add_method("query_unsigned_short", [](Fixture& f) {
            // Test querying unsigned short values
            char buf[200];
            snprintf(buf, 200, "INSERT INTO dballe_test VALUES (%d)", (int)WR_VAR(3, 1, 12));
            f.conn.exec_no_data(buf);

            auto res = f.conn.exec_store("SELECT val FROM dballe_test");
            wassert(actual(res.rowcount()) == 1);

            Varcode val = 0;
            unsigned count = 0;
            while (auto row = res.fetch())
            {
                val = (Varcode)row.as_int(0);
                ++count;
            }
            wassert(actual(count) == 1);
            wassert(actual(val) == WR_VAR(3, 1, 12));
        });
        add_method("bytes", [](Fixture& f) {
            // Test querying unsigned short values
            auto& conn = f.conn;
            conn.drop_table_if_exists("dballe_testbytes");
            conn.exec_no_data("CREATE TABLE dballe_testbytes (val VARBINARY(2048))");
            conn.exec_no_data("INSERT INTO dballe_testbytes VALUES (x'0011EEFF')");

            auto s = conn.exec_store("SELECT val FROM dballe_testbytes");
            auto row = s.expect_one_result();
            std::vector<uint8_t> val = row.as_blob(0);
            wassert(actual(val.size()) == 4);
            wassert(actual(val[0]) == 0x00);
            wassert(actual(val[1]) == 0x11);
            wassert(actual(val[2]) == 0xEE);
            wassert(actual(val[3]) == 0xFF);

            val[0] = 0xff;
            val[3] = 0x00;
            conn.exec_no_data("UPDATE dballe_testbytes SET val=x'" + conn.escape(val) + "'");

            val.clear();
            s = conn.exec_store("SELECT val FROM dballe_testbytes");
            row = s.expect_one_result();
            val = row.as_blob(0);
            wassert(actual(val.size()) == 4);
            wassert(actual(val[0]) == 0xFF);
            wassert(actual(val[1]) == 0x11);
            wassert(actual(val[2]) == 0xEE);
            wassert(actual(val[3]) == 0x00);
        });
        add_method("has_tables", [](Fixture& f) {
            // Test has_tables
            wassert(actual(f.conn.has_table("this_should_not_exist")).isfalse());
            wassert(actual(f.conn.has_table("dballe_test")).istrue());
        });
        add_method("settings", [](Fixture& f) {
            // Test settings
            f.conn.drop_table_if_exists("dballe_settings");
            wassert(actual(f.conn.has_table("dballe_settings")).isfalse());

            wassert(actual(f.conn.get_setting("test_key")) == "");

            f.conn.set_setting("test_key", "42");
            wassert(actual(f.conn.has_table("dballe_settings")).istrue());

            wassert(actual(f.conn.get_setting("test_key")) == "42");
        });
        add_method("auto_increment", [](Fixture& f) {
            // Test auto_increment
            f.conn.drop_table_if_exists("dballe_testai");
            f.conn.exec_no_data("CREATE TABLE dballe_testai (id INTEGER AUTO_INCREMENT PRIMARY KEY, val INTEGER)");
            f.conn.exec_no_data("INSERT INTO dballe_testai (val) VALUES (42)");
            wassert(actual(f.conn.get_last_insert_id()) == 1);
            f.conn.exec_no_data("INSERT INTO dballe_testai (val) VALUES (43)");
            wassert(actual(f.conn.get_last_insert_id()) == 2);
        });
    }
} test("db_sql_mysql", "MYSQL");

}
