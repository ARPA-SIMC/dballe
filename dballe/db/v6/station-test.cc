#include "db/tests.h"
#include "db/v6/db.h"
#include "sql/sql.h"
#include "db/v6/station.h"
#include "db/v6/driver.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : DriverFixture
{
    using DriverFixture::DriverFixture;

    unique_ptr<db::v6::Station> station;

    void reset_station()
    {
        if (conn->has_table("station"))
            driver->connection.execute("DELETE FROM station");

        switch (format)
        {
            case db::V5: throw error_unimplemented("v5 db is not supported");
            case db::V6:
                station = driver->create_stationv6();
                break;
            default:
                throw error_consistency("cannot test station on the current DB format");
        }
    }

    void test_setup()
    {
        DriverFixture::test_setup();
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
            wassert(actual(st.obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
            wassert(actual(inserted).istrue());
            wassert(actual(st.obtain_id(4500000, 1100000, "ciao", &inserted)) == 1);
            wassert(actual(inserted).isfalse());

            // Insert a fixed station
            wassert(actual(st.obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
            wassert(actual(inserted).istrue());
            wassert(actual(st.obtain_id(4600000, 1200000, NULL, &inserted)) == 2);
            wassert(actual(inserted).isfalse());

            // Get the ID of the first station
            wassert(actual(st.get_id(4500000, 1100000, "ciao")) == 1);

            // Get the ID of the second station
            wassert(actual(st.get_id(4600000, 1200000)) == 2);
        });
    }
};

Tests test_sqlite("db_sql_station_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_LIBPQ
Tests test_psql("db_sql_station_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_sql_station_v6_mysql", "MYSQL", db::V6);
#endif

}
