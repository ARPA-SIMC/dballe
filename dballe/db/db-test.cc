#include "config.h"
#include "msg/msg.h"
#include "db/tests.h"
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

add_method("reset_v6_v7", []{
    // Reproduce #108
    DB::set_default_format(db::V6);
    {
        auto d = DB::connect_test();
        wassert(actual(d->format()) == db::V6);
        d->reset();
    }

    DB::set_default_format(db::V7);
    {
        auto d = DB::connect_test();
        wassert(actual(d->format()) == db::V6);
        d->reset();
        // Once a database is instantiated, its reset() method can only create
        // tables for that version
        wassert(actual(d->format()) == db::V6);
    }
});

}

}

