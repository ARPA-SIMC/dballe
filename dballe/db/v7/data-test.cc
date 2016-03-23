#include "db/tests.h"
#include "sql/sql.h"
#include "db/v7/db.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "db/v7/station.h"
#include "db/v7/levtr.h"
#include "db/v7/data.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : V7DriverFixture
{
    using V7DriverFixture::V7DriverFixture;

    unique_ptr<db::v7::Data> data;
    db::v7::State state;
    db::v7::StationDesc sde1;
    db::v7::StationDesc sde2;

    Fixture(const char* backend, dballe::db::Format format)
        : V7DriverFixture(backend, format)
    {
        sde1.rep = 1;
        sde1.coords = Coords(4500000, 1100000);
        sde1.ident = "ciao";

        sde2.rep = 1;
        sde2.coords = Coords(4600000, 1200000);
        sde2.ident = nullptr;
    }

    void reset_data()
    {
        state.clear();

        auto st = driver->create_station();
        auto lt = driver->create_levtr();

        int added, deleted, updated;
        driver->create_repinfo()->update(nullptr, &added, &deleted, &updated);

        db::v7::State::stations_t::iterator si;
        db::v7::State::levels_t::iterator li;

        // Insert a mobile station
        si = st->obtain_id(state, sde1);
        wassert(actual(si->second.id) == 1);

        // Insert a fixed station
        si = st->obtain_id(state, sde2);
        wassert(actual(si->second.id) == 2);

        // Insert a lev_tr
        li = lt->obtain_id(state, db::v7::LevTrDesc(Level(1, 2, 0, 3), Trange(4, 5, 6)));
        wassert(actual(li->second.id) == 1);

        // Insert another lev_tr
        li = lt->obtain_id(state, db::v7::LevTrDesc(Level(2, 3, 1, 4), Trange(5, 6, 7)));
        wassert(actual(li->second.id) == 2);
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        data = driver->create_data();
        reset_data();
    }
};


class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            using namespace dballe::db::v7;
            auto& da = *f.data;

            auto t = f.conn->transaction();

            Var var(varinfo(WR_VAR(0, 1, 2)));

            auto insert_sample1 = [&](bulk::InsertVars& vars, int value, bulk::UpdateMode update) {
                vars.station = f.state.stations[f.sde1];
                vars.datetime = Datetime(2001, 2, 3, 4, 5, 6);
                var.seti(value);
                vars.add(&var, 1);
                wassert(da.insert(*t, vars, update));
            };

            // Insert a datum
            {
                bulk::InsertVars vars;
                wassert(insert_sample1(vars, 123, bulk::ERROR));
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).istrue());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Insert another datum
            {
                bulk::InsertVars vars;
                vars.station = f.state.stations[f.sde2];
                vars.datetime = Datetime(2002, 3, 4, 5, 6, 7);
                Var var(varinfo(WR_VAR(0, 1, 2)), 234);
                vars.add(&var, 2);
                wassert(da.insert(*t, vars, bulk::ERROR));
                wassert(actual(vars[0].id_data) == 2);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).istrue());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum: it should find its ID and do nothing
            {
                bulk::InsertVars vars;
                wassert(insert_sample1(vars, 123, bulk::ERROR));
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and ignore
            // overwrite: it should find its ID and do nothing
            {
                bulk::InsertVars vars;
                wassert(insert_sample1(vars, 125, bulk::IGNORE));
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).istrue());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and overwrite:
            // it should find its ID and update it
            {
                bulk::InsertVars vars;
                insert_sample1(vars, 125, bulk::UPDATE);
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
                bulk::InsertVars vars;
                insert_sample1(vars, 125, bulk::ERROR);
                wassert(actual(vars[0].id_data) == 1);
                wassert(actual(vars[0].needs_insert()).isfalse());
                wassert(actual(vars[0].inserted()).isfalse());
                wassert(actual(vars[0].needs_update()).isfalse());
                wassert(actual(vars[0].updated()).isfalse());
            }

            // Reinsert the first datum, with a different value and error on
            // overwrite: it should throw an error
            {
                bulk::InsertVars vars;
                try {
                    insert_sample1(vars, 126, bulk::IGNORE);
                    wassert(actual(false).isfalse());
                } catch (std::exception& e) {
                    wassert(actual(e.what()).contains("refusing to overwrite existing data"));
                }
            }

            t->commit();
        });
    }
};

Tests tg1("db_sql_data_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_ODBC
Tests tg2("db_sql_data_v7_odbc", "ODBC", db::V7);
#endif
#ifdef HAVE_LIBPQ
Tests tg3("db_sql_data_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_sql_data_v7_mysql", "MYSQL", db::V7);
#endif

}
