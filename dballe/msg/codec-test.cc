#include "dballe/msg/tests.h"
#include "dballe/msg/codec.h"

using namespace wreport;
using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("options", []() {
            using namespace dballe::msg;

            ImporterOptions simplified;
            ImporterOptions accurate;
            accurate.simplified = false;

            wassert(actual(ImporterOptions::from_string("") == simplified).istrue());
            wassert(actual(ImporterOptions::from_string("simplified") == simplified).istrue());
            wassert(actual(ImporterOptions::from_string("accurate") == accurate).istrue());
        });
    }
} test("msg_codec");

}
