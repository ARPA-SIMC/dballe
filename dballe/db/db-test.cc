#include "config.h"
#include "dballe/db/tests.h"
#include "dballe/msg/msg.h"
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
} tests("db_db");

void Tests::register_tests()
{

    add_method("empty", []() noexcept {});
}

} // namespace
