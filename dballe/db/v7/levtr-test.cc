#include "db/tests.h"
#include "db/v7/db.h"
#include "sql/sql.h"
#include "db/v7/driver.h"
#include "db/v7/levtr.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : V7DriverFixture
{
    using V7DriverFixture::V7DriverFixture;

    unique_ptr<db::v7::LevTr> levtr;

    void reset_levtr()
    {
        if (conn->has_table("levtr"))
            driver->exec_no_data("DELETE FROM levtr");
        levtr = driver->create_levtrv7();
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        reset_levtr();
    }
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            auto& lt = *f.levtr;

            // Insert a lev_tr
            wassert(actual(lt.obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

            // Insert another lev_tr
            wassert(actual(lt.obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);
        });
    }
};

Tests test_sqlite("db_sql_levtr_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_ODBC
Tests test_odbc("db_sql_levtr_v7_odbc", "ODBC", db::V7);
#endif
#ifdef HAVE_LIBPQ
Tests test_psql("db_sql_levtr_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_sql_levtr_v7_mysql", "MYSQL", db::V7);
#endif

}
