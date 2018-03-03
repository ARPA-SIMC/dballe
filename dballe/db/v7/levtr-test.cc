#include "db/tests.h"
#include "sql/sql.h"
#include "db/v7/db.h"
#include "db/v7/state.h"
#include "db/v7/driver.h"
#include "db/v7/levtr.h"
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
            auto& lt = f.db->levtr();

            db::v7::State state;

            // Insert a lev_tr
            auto i = lt.obtain_id(state, db::v7::LevTrDesc(Level(1, 2, 0, 3), Trange(4, 5, 6)));
            wassert(actual(i->second.id) == 1);

            // Insert another lev_tr
            i = lt.obtain_id(state, db::v7::LevTrDesc(Level(2, 3, 1, 4), Trange(5, 6, 7)));
            wassert(actual(i->second.id) == 2);
        });
    }
};

Tests test_sqlite("db_v7_levtr_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests test_psql("db_v7_levtr_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests test_mysql("db_v7_levtr_mysql", "MYSQL");
#endif

}
