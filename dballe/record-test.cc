#include "core/tests.h"
#include "record.h"
#include <algorithm>

using namespace std;
using namespace wreport::tests;
using namespace dballe;
using namespace wreport;

namespace {

ostream& operator<<(ostream& out, Vartype t)
{
    return out << vartype_format(t);
}

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("dballe_record");

void Tests::register_tests()
{

add_method("empty", []() {
});

}

}
