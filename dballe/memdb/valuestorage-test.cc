#include "memdb/tests.h"
#include "valuestorage.h"

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test Positions
        add_method("positions", []() {
#if 0
            Positions pa;
            pa.insert(1);
            pa.insert(2);
            pa.insert(3);

            Positions pb;
            pb.insert(1);
            pb.insert(3);
            pb.insert(4);

            pa.inplace_intersect(pb);
            wassert(actual(pa.size()) == 2u);

            wassert(actual(pa.contains(1)).istrue());
            wassert(actual(pa.contains(3)).istrue());
#endif
        });
    }
} test("memdb_core");

}
