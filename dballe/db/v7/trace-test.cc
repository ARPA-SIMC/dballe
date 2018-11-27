#include "dballe/db/tests.h"

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("db_trace");

void Tests::register_tests()
{
}

}
