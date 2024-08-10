#include "core/tests.h"
#include "value.h"

using namespace std;
using namespace wreport::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_value");

void Tests::register_tests() {

add_method("empty", []() noexcept {
});

}

}
