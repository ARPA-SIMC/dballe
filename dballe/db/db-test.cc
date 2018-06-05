#include "dballe/msg/msg.h"
#include "dballe/db/tests.h"
#include "config.h"
#include <cstring>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("db");

void Tests::register_tests()
{

add_method("empty", []{
});

}

}

