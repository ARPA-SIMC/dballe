#include "db/tests.h"
#include "db/mem/db.h"
#include "db/mem/repinfo.h"

using namespace dballe;
using namespace dballe::db::mem;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("prio", []() {
            Repinfo ri;
            ri.load();

            wassert(actual(ri.get_prio("synop")) == 101);
            wassert(actual(ri.get_prio("generic")) == 1000);
        });

        add_method("update", []() {
            Repinfo ri;
            ri.load();

            wassert(actual(ri.get_prio("synop")) == 101);

            int added, deleted, updated;
            ri.update(NULL, &added, &deleted, &updated);

            wassert(actual(added) == 0);
            wassert(actual(deleted) == 0);
            wassert(actual(updated) == 13);

            wassert(actual(ri.get_prio("synop")) == 101);
        });

        add_method("update_regression", []() {
            // Test update from a file that was known to fail
            Repinfo ri;
            ri.load();

            wassert(actual(ri.get_prio("synop")) == 101);

            int added, deleted, updated;
            ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo1.csv").c_str(), &added, &deleted, &updated);

            wassert(actual(added) == 3);
            wassert(actual(deleted) == 11);
            wassert(actual(updated) == 2);

            wassert(actual(ri.get_prio("synop")) == 101);
            wassert(actual(ri.get_prio("FIXspnpo")) == 200);
        });

        add_method("update_negative_prio", []() {
            // Test update from a file with a negative priority
            Repinfo ri;
            ri.load();

            wassert(actual(ri.get_prio("generic")) == 1000);

            int added, deleted, updated;
            ri.update((string(getenv("DBA_TESTDATA")) + "/test-repinfo2.csv").c_str(), &added, &deleted, &updated);

            wassert(actual(added) == 3);
            wassert(actual(deleted) == 11);
            wassert(actual(updated) == 2);

            wassert(actual(ri.get_prio("generic")) == -5);
        });

        add_method("autocreate", []() {
            // Test automatic repinfo creation
            Repinfo ri;
            ri.load();

            wassert(actual(ri.get_prio("foobar")) == 1001);
            wassert(actual(ri.get_prio("foobar")) == 1001);
            wassert(actual(ri.get_prio("barbaz")) == 1002);
            wassert(actual(ri.get_prio("barbaz")) == 1002);
            wassert(actual(ri.get_prio("foobar")) == 1001);
        });
    }
} test("mem_repinfo");

}

