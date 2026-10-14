#include "config.h"
#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/fwd.h"
#include "dballe/sql/sql.h"
#include "driver.h"
#include "repinfo.h"
#include "station.h"
#include "transaction.h"

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
        db::v7::Tracer<> trc;
        // Insert some values and try to read them again
        auto& st = f.tr->station();
        dballe::DBStation sde1;
        dballe::DBStation sde2;
        int si;

        // Insert a mobile station
        sde1.report = "synop";
        sde1.coords = Coords(4500000, 1100000);
        sde1.ident  = "ciao";

        si = st.maybe_get_id(trc, sde1);
        wassert(actual(si) == MISSING_INT);

        // Create
        si = st.insert_new(trc, sde1);
        wassert(actual(si) == 1);

        // Insert a fixed station
        sde2.report = "synop";
        sde2.coords = Coords(4600000, 1200000);
        sde2.ident  = nullptr;

        si = st.maybe_get_id(trc, sde2);
        wassert(actual(si) == MISSING_INT);

        // Create
        si = st.insert_new(trc, sde2);
        wassert(actual(si) == 2);

        // Get the ID of the first station
        si = st.maybe_get_id(trc, sde1);
        wassert(actual(si) == 1);

        // Get the ID of the second station
        si = st.maybe_get_id(trc, sde2);
        wassert(actual(si) == 2);
    });
}

} // namespace
