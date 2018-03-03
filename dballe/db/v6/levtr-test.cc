#include "db/tests.h"
#include "db/v6/db.h"
#include "sql/sql.h"
#include "db/v6/levtr.h"
#include "db/v6/driver.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public FixtureTestCase<EmptyTransactionFixture<V6DB>>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            auto& lt = f.db->lev_tr();

            // Insert a lev_tr
            wassert(actual(lt.obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

            // Insert another lev_tr
            wassert(actual(lt.obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);
        });
    }
};

Tests test_sqlite("db_v6_levtr_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests test_psql("db_v6_levtr_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_v6_levtr_mysql", "MYSQL");
#endif

}
