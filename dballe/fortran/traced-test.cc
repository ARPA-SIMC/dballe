#include "dballe/core/tests.h"
#include "traced.h"

using namespace std;
using namespace dballe;
using namespace dballe::fortran;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("fortran_traced");

void Tests::register_tests() {

add_method("empty", []() noexcept {
});

}

}
