#include "core/tests.h"
#include "var.h"

using namespace std;
using namespace wreport::tests;
using namespace dballe;
using namespace wreport;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("resolve", []() {
            wassert(actual(resolve_varcode("B12101")) == WR_VAR(0, 12, 101));
            wassert(actual(resolve_varcode("t")) == WR_VAR(0, 12, 101));
            auto e = wassert_throws(error_notfound, resolve_varcode("B121"));
            wassert(actual(e.what()).contains("cannot parse"));

            auto e1 = wassert_throws(error_notfound, resolve_varcode("B1210A"));
            wassert(actual(e1.what()).contains("cannot parse"));
        });
        add_method("varinfo", []() {
            Varinfo i = varinfo(WR_VAR(0, 12, 101));
            wassert(actual(i->unit) == "K");
        });
    }
} test("dballe_var");

}
