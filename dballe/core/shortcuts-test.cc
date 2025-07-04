#include "dballe/core/shortcuts.h"
#include "tests.h"

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
        // Test variable alias resolution
        add_method("shortcuts", []() {
            // First
            wassert(actual(Shortcut::by_name("block")) == sc::block);
            wassert(actual(Shortcut::by_name("blocks", 5)) == sc::block);

            // Last
            wassert(actual(Shortcut::by_name("tot_prec1")) == sc::tot_prec1);

            // Inbetween
            wassert(actual(Shortcut::by_name("cloud_h4")) == sc::cloud_h4);
            wassert(actual(Shortcut::by_name("st_type")) == sc::st_type);
            wassert(actual(Shortcut::by_name("tot_snow")) == sc::tot_snow);
        });
    }
} test("msg_vars");

} // namespace
