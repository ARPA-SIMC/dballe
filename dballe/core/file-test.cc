#include "core/file.h"
#include "core/tests.h"

using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_file");

void Tests::register_tests()
{

    add_method("empty", []() noexcept {});
}

} // namespace
