#include "tests.h"
#include "vars.h"
#include "dballe/core/defs.h"
#include "dballe/core/shortcuts.h"

using namespace dballe;
using namespace dballe::impl;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("empty", []() {
        });
#if 0
        // Test variable alias resolution
        add_method("aliases", []() {
            // First
            wassert(actual(resolve_var("block")) == sc::block);
            wassert(actual(resolve_var_substring("blocks", 5)) == sc::block);

            // Last
            wassert(actual(resolve_var("tot_prec1")) == sc::tot_prec1);

            // Inbetween
            wassert(actual(resolve_var("cloud_h4")) == sc::cloud_h4);
            wassert(actual(resolve_var("st_type")) == sc::st_type);
            wassert(actual(resolve_var("tot_snow")) == sc::tot_snow);
        });
#endif
    }
} test("msg_vars");

}
