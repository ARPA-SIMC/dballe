#include "tests.h"
#include "wr_codec.h"
#include <cstring>

using namespace std;
using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("empty", []() {
        });
    }
} test("msg_wr_codec");

}
