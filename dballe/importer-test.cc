#include "core/tests.h"
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
} test("msg_codec");

void Tests::register_tests()
{

add_method("options", []() {
    ImporterOptions simplified;
    ImporterOptions accurate;
    accurate.simplified = false;

    wassert(actual(ImporterOptions::from_string("") == simplified).istrue());
    wassert(actual(ImporterOptions::from_string("simplified") == simplified).istrue());
    wassert(actual(ImporterOptions::from_string("accurate") == accurate).istrue());
});

}

}

