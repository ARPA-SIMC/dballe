#include "db/tests.h"
#include "sql/sql.h"
#include "db/v6/db.h"
#include "db/v6/driver.h"
#include "db/v6/repinfo.h"
#include "db/v7/db.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace std;
using namespace wreport;

namespace {

struct FixtureV6 : DriverFixture
{
    using DriverFixture::DriverFixture;

    unique_ptr<db::v6::Repinfo> repinfo;

    void reset_repinfo()
    {
        if (conn->has_table("repinfo"))
            driver->connection.execute("DELETE FROM repinfo");

        switch (format)
        {
            case V5: throw error_unimplemented("v5 db is not supported");
            case V6:
                repinfo = driver->create_repinfov6();
                break;
            default:
                throw error_consistency("cannot test repinfo on the current DB format");
        }
        int added, deleted, updated;
        repinfo->update(nullptr, &added, &deleted, &updated);
    }

    void test_setup()
    {
        DriverFixture::test_setup();
        reset_repinfo();
    }
};

struct FixtureV7 : V7DriverFixture
{
    using V7DriverFixture::V7DriverFixture;

    unique_ptr<db::v7::Repinfo> repinfo;

    void reset_repinfo()
    {
        if (conn->has_table("repinfo"))
            conn->execute("DELETE FROM repinfo");

        repinfo = driver->create_repinfo();
        int added, deleted, updated;
        repinfo->update(nullptr, &added, &deleted, &updated);
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        reset_repinfo();
    }
};

template<typename Fixture>
class Tests : public DBFixtureTestCase<Fixture>
{
    using DBFixtureTestCase<Fixture>::DBFixtureTestCase;

    void register_tests() override
    {
        // Test simple queries
        this->add_method("query", [](Fixture& f) {
            auto& ri = *f.repinfo;
            wassert(actual(ri.get_id("synop")) == 1);
            wassert(actual(ri.get_id("generic")) == 255);
            wassert(actual(ri.get_rep_memo(1)) == "synop");
            wassert(actual(ri.get_priority(199)) == INT_MAX);
        });
        // Test update
        this->add_method("update", [](Fixture& f) {
            auto& ri = *f.repinfo;

            wassert(actual(ri.get_id("synop")) == 1);

            int added, deleted, updated;
            ri.update(NULL, &added, &deleted, &updated);

            wassert(actual(added) == 0);
            wassert(actual(deleted) == 0);
            wassert(actual(updated) == 13);

            wassert(actual(ri.get_id("synop")) == 1);
        });
        // Test update from a file that was known to fail
        this->add_method("fail", [](Fixture& f) {
            auto& ri = *f.repinfo;

            wassert(actual(ri.get_id("synop")) == 1);

            int added, deleted, updated;
            ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

            wassert(actual(added) == 3);
            wassert(actual(deleted) == 11);
            wassert(actual(updated) == 2);

            wassert(actual(ri.get_id("synop")) == 1);
            wassert(actual(ri.get_id("FIXspnpo")) == 201);
        });
        // Test update from a file with a negative priority
        this->add_method("fail1", [](Fixture& f) {
            auto& ri = *f.repinfo;

            int id = ri.get_id("generic");
            wassert(actual(ri.get_priority(id)) == 1000);

            int added, deleted, updated;
            wassert(ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated));

            wassert(actual(added) == 3);
            wassert(actual(deleted) == 11);
            wassert(actual(updated) == 2);

            wassert(actual(ri.get_priority(id)) == -5);
        });
        // Test automatic repinfo creation
        this->add_method("fail2", [](Fixture& f) {
            auto& ri = *f.repinfo;

            int id = ri.obtain_id("foobar");
            wassert(actual(id) > 0);
            wassert(actual(ri.get_rep_memo(id)) == "foobar");
            wassert(actual(ri.get_priority(id)) == 1001);

            id = ri.obtain_id("barbaz");
            wassert(actual(id) > 0);
            wassert(actual(ri.get_rep_memo(id)) == "barbaz");
            wassert(actual(ri.get_priority(id)) == 1002);
        });
    }
};

Tests<FixtureV6> test_sqlitev6("db_v6_repinfo_sqlite", "SQLITE", db::V6);
Tests<FixtureV7> test_sqlitev7("db_v7_repinfo_sqlite", "SQLITE", db::V7);
#ifdef HAVE_LIBPQ
Tests<FixtureV6> test_psqlv6("db_v6_repinfo_postgresql", "POSTGRESQL", db::V6);
Tests<FixtureV7> test_psqlv7("db_v7_repinfo_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests<FixtureV6> test_mysqlv6("db_v6_repinfo_mysql", "MYSQL", db::V6);
Tests<FixtureV7> test_mysqlv7("db_v7_repinfo_mysql", "MYSQL", db::V7);
#endif

}
