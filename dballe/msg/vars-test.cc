#include "msg/tests.h"
#include "core/defs.h"
#include "msg/vars.h"

using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test variable alias resolution
        add_method("aliases", []() {
            // First
            wassert(actual(resolve_var("block")) == DBA_MSG_BLOCK);
            wassert(actual(resolve_var_substring("blocks", 5)) == DBA_MSG_BLOCK);

            // Last
            wassert(actual(resolve_var("tot_prec1")) == DBA_MSG_TOT_PREC1);

            // Inbetween
            wassert(actual(resolve_var("cloud_h4")) == DBA_MSG_CLOUD_H4);
            wassert(actual(resolve_var("st_type")) == DBA_MSG_ST_TYPE);
            wassert(actual(resolve_var("tot_snow")) == DBA_MSG_TOT_SNOW);
        });
    }
} test("msg_vars");

}
