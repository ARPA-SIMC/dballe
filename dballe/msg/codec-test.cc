#include "msg/tests.h"
#include "msg/codec.h"

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

            Importer::Options simplified;
            Importer::Options accurate;
            accurate.simplified = false;

            wassert(actual(Importer::Options::from_string("") == simplified).istrue());
            wassert(actual(Importer::Options::from_string("simplified") == simplified).istrue());
            wassert(actual(Importer::Options::from_string("accurate") == accurate).istrue());
        });
    }
} test("msg_codec");

}
