#include "dballe/db/tests.h"
#include "batch.h"
#include "config.h"

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

Tests tg1("db_v7_batch_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests tg3("db_v7_batch_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_v7_batch_mysql", "MYSQL");
#endif


void Tests::register_tests()
{

add_method("empty", [](Fixture& f) {
    using namespace dballe::db::v7;

    Batch batch(f.tr);
    auto station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0)));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_true(station->ident.is_missing());
    wassert_true(station->is_new);
    wassert_true(station->station_data.data.empty());

    station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), "AB123"));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_false(station->ident.is_missing());
    wassert(actual(station->ident) == "AB123");
    wassert_true(station->is_new);
    wassert_true(station->station_data.data.empty());
});

}

}
