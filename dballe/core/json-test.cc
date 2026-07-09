#include "json.h"
#include "tests.h"
#include <sstream>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("null", []() {
            // null value
            stringstream out;
            core::JSONWriter writer(out);
            writer.add_null();
            wassert(actual(out.str()) == "null");
        });
        add_method("bool", []() {
            // bool value
            stringstream out;
            core::JSONWriter writer(out);

            writer.add(true);
            wassert(actual(out.str()) == "true");

            out.str("");
            writer.add(false);
            wassert(actual(out.str()) == "false");
        });
        add_method("int", []() {
            // int value
            stringstream out;
            core::JSONWriter writer(out);

            writer.add(1);
            wassert(actual(out.str()) == "1");

            out.str("");
            writer.add(-1234567);
            wassert(actual(out.str()) == "-1234567");
        });
        add_method("double", []() {
            // double value
            stringstream out;
            core::JSONWriter writer(out);
            writer.add(1.1);
            wassert(actual(out.str()) == "1.100000");

            out.str("");
            writer.add(-1.1);
            wassert(actual(out.str()) == "-1.100000");

            out.str("");
            writer.add(1.0);
            wassert(actual(out.str()) == "1.0");

            out.str("");
            writer.add(-1.0);
            wassert(actual(out.str()) == "-1.0");
        });
        add_method("string", []() {
            // string value
            stringstream out;
            core::JSONWriter writer(out);
            writer.add("");
            wassert(actual(out.str()) == "\"\"");

            out.str("");
            writer.add("antani");
            wassert(actual(out.str()) == "\"antani\"");

            out.str("");
            writer.add("\n");
            wassert(actual(out.str()) == "\"\\n\"");
        });
        add_method("list", []() {
            // list value
            stringstream out;
            core::JSONWriter writer(out);
            writer.start_list();
            writer.add("");
            writer.add(1);
            writer.add(1.0);
            writer.end_list();
            wassert(actual(out.str()) == "[\"\",1,1.0]");
        });
        add_method("mapping", []() {
            // mapping value
            stringstream out;
            core::JSONWriter writer(out);
            writer.start_mapping();
            writer.add("", 1);
            writer.add("antani", 1.0);
            writer.end_mapping();
            wassert(actual(out.str()) == "{\"\":1,\"antani\":1.0}");
        });
    };
} test("core_json");

} // namespace
