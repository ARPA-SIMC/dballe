#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
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

    Batch& batch = f.tr->batch;
    batch::Station* station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), Ident()));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_true(station->ident.is_missing());
    wassert_true(station->is_new);
    wassert_true(station->station_data.ids_by_code.empty());
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());

    station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), "AB123"));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_false(station->ident.is_missing());
    wassert(actual(station->ident) == "AB123");
    wassert_true(station->is_new);
    wassert_true(station->station_data.ids_by_code.empty());
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());
});

add_method("reuse", [](Fixture& f) {
    using namespace dballe::db::v7;

    Batch& batch = f.tr->batch;
    batch::Station* station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), Ident()));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_true(station->ident.is_missing());
    wassert_true(station->is_new);
    wassert_true(station->station_data.ids_by_code.empty());
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());

    batch::Station* station1 = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), Ident()));
    wassert(actual(station) == station1);

    station = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), "AB123"));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) == MISSING_INT);
    wassert(actual(station->coords) == Coords(11.0, 45.0));
    wassert_false(station->ident.is_missing());
    wassert(actual(station->ident) == "AB123");
    wassert_true(station->is_new);
    wassert_true(station->station_data.ids_by_code.empty());
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());

    station1 = wcallchecked(batch.get_station("synop", Coords(11.0, 45.0), "AB123"));
    wassert(actual(station) == station1);
});

add_method("from_db", [](Fixture& f) {
    using namespace dballe::db::v7;

    Coords coords(44.5008, 11.3288);

    TestDataSet ds;
    ds.stations["synop"].info.coords = coords;
    ds.stations["synop"].info.report = "synop";
    ds.stations["synop"].values.set("B07030", 78); // Height
    ds.data["synop"].info = ds.stations["synop"].info;
    ds.data["synop"].info.datetime = Datetime(2013, 10, 16, 10);
    ds.data["synop"].values.set(WR_VAR(0, 12, 101), 16.5);
    wassert(f.populate(ds));

    Batch& batch = f.tr->batch;
    batch.clear();
    batch::Station* station = wcallchecked(batch.get_station("synop", coords, Ident()));
    wassert(actual(station->report) == "synop");
    wassert(actual(station->id) != MISSING_INT);
    wassert(actual(station->coords) == coords);
    wassert_true(station->ident.is_missing());
    wassert_false(station->is_new);

    wassert_false(station->station_data.loaded);
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());
    wassert_true(station->station_data.ids_by_code.empty());

    station->get_station_data();
    wassert_true(station->station_data.loaded);
    wassert_true(station->station_data.to_insert.empty());
    wassert_true(station->station_data.to_update.empty());
    wassert(actual(station->station_data.ids_by_code.size()) == 1u);
    {
        auto it = station->station_data.ids_by_code.begin();
        wassert(actual(it->first) == WR_VAR(0, 7, 30));
        wassert(actual(it->second) > 0u);
    }

    auto measured_data = station->get_measured_data(Datetime(2013, 10, 16, 10));
    wassert(actual(measured_data.datetime) == Datetime(2013, 10, 16, 10));
    wassert_true(measured_data.to_insert.empty());
    wassert_true(measured_data.to_update.empty());
    wassert(actual(measured_data.ids_on_db.size()) == 1u);
    {
        auto it = measured_data.ids_on_db.begin();
        wassert(actual(it->first.id) > 0);
        wassert(actual(it->first.varcode) == WR_VAR(0, 12, 101));
        wassert(actual(it->second) > 0u);
    }
});

}

}
