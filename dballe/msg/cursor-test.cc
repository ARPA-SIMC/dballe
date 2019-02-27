#include "dballe/msg/tests.h"
#include <cstring>

using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("msg_cursor");

void Tests::register_tests()
{

add_method("empty", []{
});

}

}
