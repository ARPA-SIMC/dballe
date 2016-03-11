#include "db/tests.h"
#include "db/v6/db.h"
#include "sql/sql.h"
#include "db/sql/levtr.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : DriverFixture
{
    using DriverFixture::DriverFixture;

    unique_ptr<db::sql::LevTr> levtr;

    void reset_levtr()
    {
        if (conn->has_table("levtr"))
            driver->exec_no_data("DELETE FROM levtr");
        levtr = driver->create_levtrv6();
    }

    void test_setup()
    {
        DriverFixture::test_setup();
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

Tests test_sqlite("db_sql_levtr_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests test_odbc("db_sql_levtr_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests test_psql("db_sql_levtr_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_sql_levtr_v6_mysql", "MYSQL", db::V6);
#endif

}
