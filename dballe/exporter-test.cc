#include "core/tests.h"
#include "exporter.h"

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_exporter");

void Tests::register_tests()
{

    add_method("empty", []() noexcept {});
}

} // namespace
