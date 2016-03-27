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

struct Fixture : V7DriverFixture
{
    using V7DriverFixture::V7DriverFixture;

    unique_ptr<db::v7::Station> station;
    unique_ptr<db::v7::Repinfo> repinfo;

    void reset_station()
    {
        if (conn->has_table("station"))
            conn->execute("DELETE FROM station");

        repinfo = driver->create_repinfo();
        int added, deleted, updated;
        repinfo->update(nullptr, &added, &deleted, &updated);

        station = driver->create_station();
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        reset_station();
    }
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            // Insert some values and try to read them again
            auto& st = *f.station;
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

Tests test_sqlite("db_sql_station_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_ODBC
Tests test_odbc("db_sql_station_v7_odbc", "ODBC", db::V7);
#endif
#ifdef HAVE_LIBPQ
Tests test_psql("db_sql_station_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_sql_station_v7_mysql", "MYSQL", db::V7);
#endif

}
