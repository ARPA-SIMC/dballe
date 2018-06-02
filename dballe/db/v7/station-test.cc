#include "dballe/db/tests.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/db.h"
#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "transaction.h"
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

    // Retrieve from cache, we know it was created in this transaction
    si = st.obtain_id(*f.tr, sde1);
    wassert(actual(si) == 1);

    // Clear cache, "new in this transaction" state is lost, it will
    // look it up in the database
    f.tr->clear_cached_state();
    si = st.obtain_id(*f.tr, sde1);
    wassert(actual(si) == 1);

    // Insert a fixed station
    sde2.report = "synop";
    sde2.coords = Coords(4600000, 1200000);
    sde2.ident = nullptr;

    // Create
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);

    // Retrieve from cache, we know it was created in this transaction
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);

    // Clear cache, "new in this transaction" state is lost, it will
    // look it up in the database
    f.tr->clear_cached_state();
    si = st.obtain_id(*f.tr, sde2);
    wassert(actual(si) == 2);

    // Get the ID of the first station
    si = st.get_id(*f.tr, sde1);
    wassert(actual(si) == 1);

    // Get the ID of the second station
    si = st.get_id(*f.tr, sde2);
    wassert(actual(si) == 2);
});

}

}
