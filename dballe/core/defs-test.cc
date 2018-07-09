#include "tests.h"
#include "defs.h"

using namespace dballe;
using namespace wreport::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_defs");

void Tests::register_tests()
{
}

}
