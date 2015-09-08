#include "tests.h"
#include "json.h"

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
            string out;
            core::JSONWriter writer(out);
            writer.add_null();
            wassert(actual(out) == "null");
        });
        add_method("bool", []() {
            // bool value
            string out;
            core::JSONWriter writer(out);

            writer.add(true);
            wassert(actual(out) == "true");

            out.clear();
            writer.add(false);
            wassert(actual(out) == "false");
        });
        add_method("int", []() {
            // int value
            string out;
            core::JSONWriter writer(out);

            writer.add(1);
            wassert(actual(out) == "1");

            out.clear();
            writer.add(-1234567);
            wassert(actual(out) == "-1234567");
        });
        add_method("double", []() {
            // double value
            string out;
            core::JSONWriter writer(out);
            writer.add(1.1);
            wassert(actual(out) == "1.100000");

            out.clear();
            writer.add(-1.1);
            wassert(actual(out) == "-1.100000");

            out.clear();
            writer.add(1.0);
            wassert(actual(out) == "1.0");

            out.clear();
            writer.add(-1.0);
            wassert(actual(out) == "-1.0");
        });
        add_method("string", []() {
            // string value
            string out;
            core::JSONWriter writer(out);
            writer.add("");
            wassert(actual(out) == "\"\"");

            out.clear();
            writer.add("antani");
            wassert(actual(out) == "\"antani\"");

            out.clear();
            writer.add("\n");
            wassert(actual(out) == "\"\\n\"");
        });
        add_method("list", []() {
            // list value
            string out;
            core::JSONWriter writer(out);
            writer.start_list();
            writer.add("");
            writer.add(1);
            writer.add(1.0);
            writer.end_list();
            wassert(actual(out) == "[\"\",1,1.0]");
        });
        add_method("mapping", []() {
            // mapping value
            string out;
            core::JSONWriter writer(out);
            writer.start_mapping();
            writer.add("", 1);
            writer.add("antani", 1.0);
            writer.end_mapping();
            wassert(actual(out) == "{\"\":1,\"antani\":1.0}");
        });
    };
} test("core_json");

}
