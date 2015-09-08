#include "core/tests.h"
#include "core/aliases.h"

using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("resolve", []() {
            wassert(actual_varcode(varcode_alias_resolve("block")) == WR_VAR(0, 1, 1));
            wassert(actual_varcode(varcode_alias_resolve("station")) == WR_VAR(0, 1,  2));
            wassert(actual_varcode(varcode_alias_resolve("height")) == WR_VAR(0, 7, 30));
            wassert(actual_varcode(varcode_alias_resolve("heightbaro")) == WR_VAR(0, 7, 31));
            wassert(actual_varcode(varcode_alias_resolve("name")) == WR_VAR(0, 1, 19));
            wassert(actual_varcode(varcode_alias_resolve("cippolippo")) == 0);
        });
    }
} test("core_aliases");

}
