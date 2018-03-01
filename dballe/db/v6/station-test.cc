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

struct Fixture : V6DBFixture
{
    using V6DBFixture::V6DBFixture;

    void reset_station()
    {
        db->disappear();

        switch (format)
        {
            case db::V5: throw error_unimplemented("v5 db is not supported");
            case db::V6:
                db->reset();
                break;
            default:
                throw error_consistency("cannot test station on the current DB format");
        }
    }

    void test_setup()
    {
        V6DBFixture::test_setup();
        reset_station();
    }
};

class Tests : public DBFixtureTestCase<Fixture>
{
    using DBFixtureTestCase::DBFixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            // Insert some values and try to read them again
            auto& st = f.db->station();
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

Tests test_sqlite("db_v6_station_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests test_psql("db_v6_station_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_v6_station_mysql", "MYSQL");
#endif

}
