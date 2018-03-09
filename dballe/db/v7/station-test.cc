#include "db/tests.h"
#include "db/v7/db.h"
#include "db/v7/state.h"
#include "sql/sql.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "db/v7/station.h"
#include "db/v7/transaction.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public FixtureTestCase<EmptyTransactionFixture<V7DB>>
{
    using FixtureTestCase::FixtureTestCase;
    typedef EmptyTransactionFixture<V7DB> Fixture;

    void register_tests() override;
};

Tests test_sqlite("db_v7_station_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests test_psql("db_v7_station_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_v7_station_mysql", "MYSQL");
#endif

void Tests::register_tests()
{
add_method("insert", [](Fixture& f) {
    db::v7::StationCache stations;

    wassert_false(stations.find_station(1));
    wassert_false(stations.find_station(MISSING_INT));
    wassert(actual(stations.find_id(Station())) == MISSING_INT);

    Station st;
    st.ana_id = 1;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    stations.insert(st);

    wassert_true(stations.find_station(1));
    wassert(actual(*stations.find_station(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    stations.insert(st);

    wassert_true(stations.find_station(1));
    wassert(actual(*stations.find_station(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    wassert(actual(stations.by_lon[st.coords.lon].size()) == 1u);
});

add_method("insert", [](Fixture& f) {
    // Insert some values and try to read them again
    auto& st = f.tr->station();
    dballe::Station sde1;
    dballe::Station sde2;
    int si;

    // Insert a mobile station
    sde1.report = "synop";
    sde1.coords = Coords(4500000, 1100000);
    sde1.ident = "ciao";

    // Create
    si = st.obtain_id(*f.tr, sde1);
    wassert(actual(si) == 1);
    // TODO: wassert(actual(si->second.is_new).istrue());

    // Retrieve from cache, we know it was created in this transaction
    si = st.obtain_id(*f.tr, sde1);
    wassert(actual(si) == 1);
    // TODO: wassert(actual(si->second.is_new).istrue());

    // Clear cache, "new in this transaction" state is lost, it will
    // look it up in the database
    f.tr->state.clear();
    si = st.obtain_id(*f.tr, sde1);
    wassert(actual(si) == 1);
    // TODO: wassert(actual(si->second.is_new).isfalse());

    // Insert a fixed station
    sde2.report = "synop";
    sde2.coords = Coords(4600000, 1200000);
    sde2.ident = nullptr;

    // Create
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);
    // TODO: wassert(actual(si->second.is_new).istrue());

    // Retrieve from cache, we know it was created in this transaction
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);
    // TODO: wassert(actual(si->second.is_new).istrue());

    // Clear cache, "new in this transaction" state is lost, it will
    // look it up in the database
    f.tr->state.clear();
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);
    // TODO: wassert(actual(si->second.is_new).isfalse());

    // Get the ID of the first station
    si = st.get_id(*f.tr, sde1);
    wassert(actual(si) == 1);
    // TODO: wassert(actual(si->second.is_new).isfalse());

    // Get the ID of the second station
    si = st.get_id(*f.tr, sde2);
    wassert(actual(si) == 2);
    // TODO: wassert(actual(si->second.is_new).isfalse());
});

}

}
