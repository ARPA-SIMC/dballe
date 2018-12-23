#include "dballe/core/tests.h"
#include "db.h"
#include <cstring>

using namespace dballe;
using namespace dballe::tests;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("db");

void Tests::register_tests()
{

add_method("dbconnectoptions", []{
    auto opts = DBConnectOptions::create("sqlite://test.sqlite");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_false(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("postgresql:///testuser@testhost/testdb?port=5433");
    wassert(actual(opts->url) == "postgresql:///testuser@testhost/testdb?port=5433");
    wassert_false(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("mysql://host/db?user=testuser&password=testpass");
    wassert(actual(opts->url) == "mysql://host/db?user=testuser&password=testpass");
    wassert_false(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=yes");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_true(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("postgresql:///testuser@testhost/testdb?port=5433&wipe=yes");
    wassert(actual(opts->url) == "postgresql:///testuser@testhost/testdb?port=5433");
    wassert_true(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("mysql://host/db?wipe=yes&user=testuser&password=testpass");
    wassert(actual(opts->url) == "mysql://host/db?user=testuser&password=testpass");
    wassert_true(opts->wipe);
    opts->reset_actions();
    wassert_false(opts->wipe);
});

add_method("parse_wipe", []{
    auto opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=yes");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_true(opts->wipe);

    // opts = DBConnectOptions::create("sqlite://test.sqlite?wipe");
    // wassert(actual(opts->url) == "sqlite://test.sqlite");
    // wassert_true(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=true");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_true(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=tRUe");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_true(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=1");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_true(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=false");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=0");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=no");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_false(opts->wipe);

    opts = DBConnectOptions::create("sqlite://test.sqlite?wipe=NO");
    wassert(actual(opts->url) == "sqlite://test.sqlite");
    wassert_false(opts->wipe);
});

}

}
