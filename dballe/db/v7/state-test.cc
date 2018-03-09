#include "core/tests.h"
#include "state.h"
#include <cstring>

using namespace std;
using namespace dballe::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("db_v7_state");

void Tests::register_tests()
{
}

}
