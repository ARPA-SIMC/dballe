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

    void register_tests() override
    {
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
    }
} test("core_var");

}
