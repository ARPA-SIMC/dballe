#include "db/tests.h"
#include "sql/sql.h"
#include "db/v6/db.h"
#include "db/v6/driver.h"
#include "db/v6/repinfo.h"
#include "db/v7/db.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include <wreport/utils/sys.h>
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace std;
using namespace wreport;

namespace {

template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    using FixtureTestCase<EmptyTransactionFixture<DB>>::FixtureTestCase;
    typedef EmptyTransactionFixture<DB> Fixture;

    void register_tests() override
    {
        // Test simple queries
        this->add_method("query", [](Fixture& f) {
            auto& ri = f.db->repinfo();
            wassert(actual(ri.get_id("synop")) == 1);
            wassert(actual(ri.get_id("generic")) == 255);
            wassert(actual(ri.get_rep_memo(1)) == "synop");
            wassert(actual(ri.get_priority("synop")) == 101);
            wassert(actual(ri.get_priority("wrong")) == INT_MAX);
        });
        // Test update
        this->add_method("update", [](Fixture& f) {
            auto& ri = f.db->repinfo();

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
            auto& ri = f.db->repinfo();

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
            auto& ri = f.db->repinfo();

            int id = ri.get_id("generic");
            wassert(actual(ri.get_priority("generic")) == 1000);

            int added, deleted, updated;
            wassert(ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated));

            wassert(actual(added) == 3);
            wassert(actual(deleted) == 11);
            wassert(actual(updated) == 2);

            wassert(actual(ri.get_priority("generic")) == -5);
        });
        // Test automatic repinfo creation
        this->add_method("fail2", [](Fixture& f) {
            auto& ri = f.db->repinfo();

            int id = ri.obtain_id("foobar");
            wassert(actual(id) > 0);
            wassert(actual(ri.get_rep_memo(id)) == "foobar");
            wassert(actual(ri.get_priority("foobar")) == 1001);

            id = ri.obtain_id("barbaz");
            wassert(actual(id) > 0);
            wassert(actual(ri.get_rep_memo(id)) == "barbaz");
            wassert(actual(ri.get_priority("barbaz")) == 1002);
        });
        // See https://github.com/ARPA-SIMC/dballe/issues/30
        this->add_method("case", [](Fixture& f) {
            auto& ri = f.db->repinfo();

            int added, deleted, updated;
            sys::write_file("test_case.csv", "234,foOBar,foOBar,100,oss,0\n");
            ri.update("test_case.csv", &added, &deleted, &updated);
            wassert(actual(added) == 1);
            wassert(actual(deleted) == 13);
            wassert(actual(updated) == 0);

            int id = ri.obtain_id("fooBAR");
            wassert(actual(id) == 234);
            wassert(actual(ri.get_rep_memo(id)) == "foobar");
            wassert(actual(ri.get_priority("foobar")) == 100);
            wassert(actual(ri.get_id("foobar")) == id);
            wassert(actual(ri.get_id("Foobar")) == id);
            wassert(actual(ri.get_id("FOOBAR")) == id);
            wassert(actual(ri.obtain_id("FOOBAR")) == id);
        });
    }
};

Tests<V6DB> test_sqlitev6("db_v6_repinfo_sqlite", "SQLITE");
Tests<V7DB> test_sqlitev7("db_v7_repinfo_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V6DB> test_psqlv6("db_v6_repinfo_postgresql", "POSTGRESQL");
Tests<V7DB> test_psqlv7("db_v7_repinfo_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V6DB> test_mysqlv6("db_v6_repinfo_mysql", "MYSQL");
Tests<V7DB> test_mysqlv7("db_v7_repinfo_mysql", "MYSQL");
#endif

}
