#include "config.h"
#include "dballe/db/tests.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : EmptyTransactionFixture<V7DB>
{
    using EmptyTransactionFixture::EmptyTransactionFixture;
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override;
};

Tests tg1("db_v7_utils_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests tg3("db_v7_utils_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_v7_utils_mysql", "MYSQL");
#endif

void Tests::register_tests()
{

    add_method("empty",
               [](Fixture& f) noexcept { using namespace dballe::db::v7; });
}

} // namespace
