#include "core/tests.h"
#include "values.h"
#include <cstring>

using namespace std;
using namespace dballe::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_values");

void Tests::register_tests()
{

    add_method("codec", []() {
        Values vals;
        // Integer variable
        vals.set(newvar(WR_VAR(0, 1, 2), 123));
        // Floating point variable
        vals.set(newvar(WR_VAR(0, 12, 101), 280.23));
        // Text variable
        vals.set(newvar(WR_VAR(0, 1, 19), "Test string value"));

        vector<uint8_t> encoded = vals.encode();
        wassert(actual(encoded.size()) ==
                (14 + strlen("Test string value") + 1));

        Values vals1;
        Values::decode(encoded, [&](std::unique_ptr<wreport::Var> var) {
            vals1.set(move(var));
        });

        wassert(actual(vals1.var(WR_VAR(0, 1, 2))) ==
                vals.var(WR_VAR(0, 1, 2)));
        wassert(actual(vals1.var(WR_VAR(0, 12, 101))) ==
                vals.var(WR_VAR(0, 12, 101)));
        wassert(actual(vals1.var(WR_VAR(0, 1, 19))) ==
                vals.var(WR_VAR(0, 1, 19)));
    });
}

} // namespace
