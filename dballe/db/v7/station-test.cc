#include "db/tests.h"
#include "db/v7/db.h"
#include "db/v7/state.h"
#include "sql/sql.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "db/v7/station.h"
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

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            // Insert some values and try to read them again
            auto& st = f.db->station();
            db::v7::StationDesc sde1;
            db::v7::StationDesc sde2;
            db::v7::State state;
            db::v7::stations_t::iterator si;

            // Insert a mobile station
            sde1.rep = 1;
            sde1.coords = Coords(4500000, 1100000);
            sde1.ident = "ciao";

            // Create
            si = st.obtain_id(state, sde1);
            wassert(actual(si->second.id) == 1);
            wassert(actual(si->second.is_new).istrue());

            // Retrieve from cache, we know it was created in this transaction
            si = st.obtain_id(state, sde1);
            wassert(actual(si->second.id) == 1);
            wassert(actual(si->second.is_new).istrue());

            // Clear cache, "new in this transaction" state is lost, it will
            // look it up in the database
            state.clear();
            si = st.obtain_id(state, sde1);
            wassert(actual(si->second.id) == 1);
            wassert(actual(si->second.is_new).isfalse());

            // Insert a fixed station
            sde2.rep = 1;
            sde2.coords = Coords(4600000, 1200000);
            sde2.ident = nullptr;

            // Create
            si = st.obtain_id(state, sde2);
            wassert(actual(si->second.id) == 2);
            wassert(actual(si->second.is_new).istrue());

            // Retrieve from cache, we know it was created in this transaction
            si = st.obtain_id(state, sde2);
            wassert(actual(si->second.id) == 2);
            wassert(actual(si->second.is_new).istrue());

            // Clear cache, "new in this transaction" state is lost, it will
            // look it up in the database
            state.clear();
            si = st.obtain_id(state, sde2);
            wassert(actual(si->second.id) == 2);
            wassert(actual(si->second.is_new).isfalse());

            // Get the ID of the first station
            si = st.get_id(state, sde1);
            wassert(actual(si->second.id) == 1);
            wassert(actual(si->second.is_new).isfalse());

            // Get the ID of the second station
            si = st.get_id(state, sde2);
            wassert(actual(si->second.id) == 2);
            wassert(actual(si->second.is_new).isfalse());
        });
    }
};

Tests test_sqlite("db_v7_station_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests test_psql("db_v7_station_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_v7_station_mysql", "MYSQL");
#endif

}
