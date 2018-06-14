#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/var.h"
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

add_method("import", [](Fixture& f) {
    Messages msgs1 = read_msgs("bufr/test-airep1.bufr", File::BUFR);
    f.tr->import_msg(msgs1[0], NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA);
    db::v7::Batch& batch = f.tr->batch;
    wassert(actual(batch.count_select_stations) == 1u);
    wassert(actual(batch.count_select_station_data) == 0u);
    wassert(actual(batch.count_select_data) == 0u);
});

add_method("insert", [](Fixture& f) {
    using namespace db::v7;
    Batch& batch = f.tr->batch;
    batch.set_write_attrs(false);
    auto st = batch.get_station("synop", Coords(45.0, 11.0), Ident());
    auto& st_data = st->get_station_data();
    auto& data = st->get_measured_data(Datetime(2018, 6, 1));

    Var sv(var(WR_VAR(0, 7, 30), 1000.0));
    st_data.add(&sv, batch::ERROR);
    Var dv(var(WR_VAR(0, 12, 101), 25.6));
    int id_levtr = f.tr->levtr().obtain_id(LevTrEntry(Level(1), Trange(254)));
    data.add(id_levtr, &dv, batch::ERROR);

    batch.write_pending();
    wassert(actual(batch.count_select_stations) == 1u);
    wassert(actual(batch.count_select_station_data) == 0u);
    wassert(actual(batch.count_select_data) == 0u);
});

add_method("insert_double_station_value", [](Fixture& f) {
    using namespace db::v7;
    Batch& batch = f.tr->batch;
    batch.set_write_attrs(false);
    auto st = batch.get_station("synop", Coords(45.0, 11.0), Ident());
    auto& st_data = st->get_station_data();

    Var sv1(var(WR_VAR(0, 7, 30), 1000.0));
    Var sv2(var(WR_VAR(0, 7, 30), 1001.0));
    st_data.add(&sv1, batch::ERROR);
    st_data.add(&sv2, batch::ERROR);
    batch.write_pending();

    wassert(actual(batch.count_select_stations) == 1u);
    wassert(actual(batch.count_select_station_data) == 0u);
    wassert(actual(batch.count_select_data) == 0u);

    // Query var and check that it is 1001
    auto cur = f.tr->query_station_data(core::Query());
    wassert(actual(cur->remaining()) == 1);
    wassert(cur->next());
    wassert(actual(cur->get_var()) == sv2);
});

add_method("insert_double_measured_value", [](Fixture& f) {
    using namespace db::v7;
    Batch& batch = f.tr->batch;
    batch.set_write_attrs(false);
    auto st = batch.get_station("synop", Coords(45.0, 11.0), Ident());
    auto& data = st->get_measured_data(Datetime(2018, 6, 1));

    Var dv1(var(WR_VAR(0, 12, 101), 25.6));
    Var dv2(var(WR_VAR(0, 12, 101), 25.7));
    int id_levtr = f.tr->levtr().obtain_id(LevTrEntry(Level(1), Trange(254)));
    data.add(id_levtr, &dv1, batch::ERROR);
    data.add(id_levtr, &dv2, batch::ERROR);
    batch.write_pending();

    wassert(actual(batch.count_select_stations) == 1u);
    wassert(actual(batch.count_select_station_data) == 0u);
    wassert(actual(batch.count_select_data) == 0u);

    // Query var and check that it is 25.7
    auto cur = f.tr->query_data(core::Query());
    wassert(actual(cur->remaining()) == 1);
    wassert(cur->next());
    wassert(actual(cur->get_var()) == dv2);
});

}

}
