#include "db/tests.h"
#include "db/v7/db.h"
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
            driver->exec_no_data("DELETE FROM station");

        repinfo = driver->create_repinfov7();
        int added, deleted, updated;
        repinfo->update(nullptr, &added, &deleted, &updated);

        station = driver->create_stationv7();
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
            bool inserted;

            // Insert a mobile station
            wassert(actual(st.obtain_id(1, 4500000, 1100000, "ciao", &inserted)) == 1);
            wassert(actual(inserted).istrue());
            wassert(actual(st.obtain_id(1, 4500000, 1100000, "ciao", &inserted)) == 1);
            wassert(actual(inserted).isfalse());

            // Insert a fixed station
            wassert(actual(st.obtain_id(1, 4600000, 1200000, NULL, &inserted)) == 2);
            wassert(actual(inserted).istrue());
            wassert(actual(st.obtain_id(1, 4600000, 1200000, NULL, &inserted)) == 2);
            wassert(actual(inserted).isfalse());

            // Get the ID of the first station
            wassert(actual(st.get_id(1, 4500000, 1100000, "ciao")) == 1);

            // Get the ID of the second station
            wassert(actual(st.get_id(1, 4600000, 1200000)) == 2);
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
