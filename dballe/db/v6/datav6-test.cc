#include "db/tests.h"
#include "db/v6/db.h"
#include "sql/sql.h"
#include "db/v6/driver.h"
#include "db/v6/repinfo.h"
#include "db/v6/station.h"
#include "db/v6/levtr.h"
#include "db/v6/datav6.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : DriverFixture
{
    using DriverFixture::DriverFixture;

    unique_ptr<db::v6::DataV6> data;

    void reset_data()
    {
        auto st = driver->create_stationv6();
        auto lt = driver->create_levtrv6();

        int added, deleted, updated;
        driver->create_repinfov6()->update(nullptr, &added, &deleted, &updated);

        // Insert a mobile station
        wassert(actual(st->obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st->obtain_id(4600000, 1200000)) == 2);

        // Insert a lev_tr
        wassert(actual(lt->obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

        // Insert another lev_tr
        wassert(actual(lt->obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);
    }

    void test_setup()
    {
        DriverFixture::test_setup();
        data = driver->create_datav6();
        reset_data();
    }
};


class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            using namespace dballe::db::v6;
            auto& da = *f.data;

            auto t = f.conn->transaction();

            Var var(varinfo(WR_VAR(0, 1, 2)));

            auto insert_sample1 = [&](bulk::InsertV6& vars, int value, DataV6::UpdateMode update) {
                vars.id_station = 1;
                vars.id_report = 1;
                vars.datetime = Datetime(2001, 2, 3, 4, 5, 6);
                var.seti(value);
                vars.add(&var, 1);
                da.insert(*t, vars, update);
            };

            // Insert a datum
            {
                bulk::InsertV6 vars;
                insert_sample1(vars, 123, DataV6::ERROR);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).istrue());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Insert another datum
            {
                bulk::InsertV6 vars;
                vars.id_station = 2;
                vars.id_report = 2;
                vars.datetime = Datetime(2002, 3, 4, 5, 6, 7);
                Var var(varinfo(WR_VAR(0, 1, 2)), 234);
                vars.add(&var, 2);
                da.insert(*t, vars, DataV6::ERROR);
                wassert(actual(vars[0].id_data) == 2);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).istrue());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum: it should find its ID and do nothing
            {
                bulk::InsertV6 vars;
                insert_sample1(vars, 123, DataV6::ERROR);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and ignore
            // overwrite: it should find its ID and do nothing
            {
                bulk::InsertV6 vars;
                insert_sample1(vars, 125, DataV6::IGNORE);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).istrue());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and overwrite:
            // it should find its ID and update it
            {
                bulk::InsertV6 vars;
                insert_sample1(vars, 125, DataV6::UPDATE);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).istrue());
            }

            // Reinsert the first datum, with the same value and error on
            // overwrite: it should find its ID and do nothing, because the value
            // does not change.
            {
                bulk::InsertV6 vars;
                insert_sample1(vars, 125, DataV6::ERROR);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and error on
            // overwrite: it should throw an error
            {
                bulk::InsertV6 vars;
                try {
                    insert_sample1(vars, 126, DataV6::IGNORE);
                    wassert(actual(false).isfalse());
                } catch (std::exception& e) {
                    wassert(actual(e.what()).contains("refusing to overwrite existing data"));
                }
            }

            t->commit();
        });
    }
};

Tests tg1("db_sql_data_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg2("db_sql_data_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg3("db_sql_data_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_sql_data_v6_mysql", "MYSQL", db::V6);
#endif

}