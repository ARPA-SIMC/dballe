#include "core/tests.h"
#include "core/values.h"

using namespace std;
using namespace dballe::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("empty", []() {
        });
    }
} test("core_values");

}

