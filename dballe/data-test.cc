#include "core/tests.h"
#include "data.h"

using namespace std;
using namespace wreport::tests;
using namespace dballe;
using namespace wreport;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_data");

void Tests::register_tests()
{

add_method("empty", []() noexcept {
});

}

}

