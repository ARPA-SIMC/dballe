#include "core/tests.h"
#include "msg/msg.h"
#include "importer.h"

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_importer");

void Tests::register_tests()
{

add_method("options", []() {
    impl::ImporterOptions simplified;
    impl::ImporterOptions accurate;
    accurate.simplified = false;

    wassert(actual(impl::ImporterOptions("") == simplified).istrue());
    wassert(actual(impl::ImporterOptions("simplified") == simplified).istrue());
    wassert(actual(impl::ImporterOptions("accurate") == accurate).istrue());
});

}

}

