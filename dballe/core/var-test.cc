#include "tests.h"
#include "var.h"

using namespace dballe;
using namespace wreport;
using namespace wreport::tests;
using namespace std;

namespace tut {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_var");

void Tests::register_tests() {

add_method("single", []() {
    set<Varcode> codes;
    resolve_varlist("B12101", codes);
    wassert(actual(codes.size()) == 1);
    wassert(actual(*codes.begin()) == WR_VAR(0, 12, 101));
});

add_method("multi", []() {
    set<Varcode> codes;
    resolve_varlist("B12101,B12103", codes);
    wassert(actual(codes.size()) == 2);
    set<Varcode>::const_iterator i = codes.begin();
    wassert(actual(*i) == WR_VAR(0, 12, 101));
    ++i;
    wassert(actual(*i) == WR_VAR(0, 12, 103));
});

add_method("format", []() {
    char buf[7];
    format_code(WR_VAR(0, 0, 0), buf);
    wassert(actual(buf) == "B00000");
    format_code(WR_VAR(0, 1, 2), buf);
    wassert(actual(buf) == "B01002");
    format_code(WR_VAR(0, 63, 0), buf);
    wassert(actual(buf) == "B63000");
    format_code(WR_VAR(0, 0, 255), buf);
    wassert(actual(buf) == "B00255");
    format_code(WR_VAR(1, 1, 2), buf);
    wassert(actual(buf) == "R01002");
    format_code(WR_VAR(2, 1, 2), buf);
    wassert(actual(buf) == "C01002");
    format_code(WR_VAR(3, 1, 2), buf);
    wassert(actual(buf) == "D01002");
    format_code(WR_VAR(4, 1, 2), buf);
    wassert(actual(buf) == "B01002");

    format_bcode(WR_VAR(0, 1, 2), buf);
    wassert(actual(buf) == "B01002");
    format_bcode(WR_VAR(1, 1, 2), buf);
    wassert(actual(buf) == "B01002");
    format_bcode(WR_VAR(2, 1, 2), buf);
    wassert(actual(buf) == "B01002");
});

}

}
