#include "db/tests.h"
#include "sql/sql.h"
#include "db/v7/db.h"
#include "db/v7/transaction.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "db/v7/station.h"
#include "db/v7/levtr.h"
#include "db/v7/data.h"
#include "db/v7/attr.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : V7DriverFixture
{
    using V7DriverFixture::V7DriverFixture;

    unique_ptr<db::v7::Attr> attr;

    void reset_attr()
    {
        using namespace dballe::db::v7;

        auto st = driver->create_station();
        auto lt = driver->create_levtr();
        auto da = driver->create_data();

        int added, deleted, updated;
        driver->create_repinfo()->update(nullptr, &added, &deleted, &updated);

        db::v7::StationDesc sde1;
        db::v7::StationDesc sde2;
        db::v7::State state;
        db::v7::stations_t::iterator si;
        db::v7::levtrs_t::iterator li;

        // Insert a mobile station
        sde1.rep = 1;
        sde1.coords = Coords(4500000, 1100000);
        sde1.ident = "ciao";
        si = st->obtain_id(state, sde1);
        wassert(actual(si->second.id) == 1);

        // Insert a fixed station
        sde2.rep = 1;
        sde2.coords = Coords(4600000, 1200000);
        sde2.ident = nullptr;
        si = st->obtain_id(state, sde2);
        wassert(actual(si->second.id) == 2);

        // Insert a lev_tr
        li = lt->obtain_id(state, db::v7::LevTrDesc(Level(1, 2, 0, 3), Trange(4, 5, 6)));
        wassert(actual(li->second.id) == 1);

        // Insert another lev_tr
        li = lt->obtain_id(state, db::v7::LevTrDesc(Level(2, 3, 1, 4), Trange(5, 6, 7)));
        wassert(actual(li->second.id) == 2);

        auto conn_t = conn->transaction();
        unique_ptr<dballe::db::v7::Transaction> t(new dballe::db::v7::Transaction(move(conn_t)));

        // Insert a datum
        {
            bulk::InsertVars vars;
            vars.station = state.stations.find(sde1);
            vars.datetime = Datetime(2001, 2, 3, 4, 5, 6);
            Var var(varinfo(WR_VAR(0, 1, 2)), 123);
            vars.add(&var, 1);
            da->insert(*t, vars, bulk::ERROR);
        }

        // Insert another datum
        {
            bulk::InsertVars vars;
            vars.station = state.stations.find(sde2);
            vars.datetime = Datetime(2002, 3, 4, 5, 6, 7);
            Var var(varinfo(WR_VAR(0, 1, 2)), 234);
            vars.add(&var, 2);
            da->insert(*t, vars, bulk::ERROR);
        }
        t->commit();
    }

    void test_setup()
    {
        V7DriverFixture::test_setup();
        attr = driver->create_attr();
        reset_attr();
    }

    Var query(int id_data, unsigned expected_attr_count)
    {
        Var res(varinfo(WR_VAR(0, 12, 101)));
        unsigned count = 0;
        attr->read(id_data, [&](unique_ptr<Var> attr) { res.seta(move(attr)); ++count; });
        wassert(actual(count) == expected_attr_count);
        return res;
    }
};

class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("insert", [](Fixture& f) {
            using namespace dballe::db::v7;
            auto& at = *f.attr;

            auto conn_t = f.conn->transaction();
            unique_ptr<dballe::db::v7::Transaction> t(new dballe::db::v7::Transaction(move(conn_t)));

            Var var1(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var1.seta(newvar(WR_VAR(0, 33, 7), 50));

            Var var2(varinfo(WR_VAR(0, 12, 101)), 280.0);
            var2.seta(newvar(WR_VAR(0, 33, 7), 75));

            // Insert two attributes
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, Attr::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, Attr::ERROR);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).istrue());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the first attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var1, 1);
                at.insert(*t, attrs, Attr::IGNORE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Reinsert the second attribute: it should work, doing no insert/update queries
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var2, 2);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).isfalse());
            }

            // Load the attributes for the first variable
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // Load the attributes for the second variable
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }

            // Update both values
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var2, 1);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            {
                bulk::InsertAttrsV7 attrs;
                attrs.add_all(var1, 2);
                at.insert(*t, attrs, Attr::UPDATE);
                wassert(actual(attrs.size()) == 1);
                wassert(actual(attrs[0].needs_insert()).isfalse());
                wassert(actual(attrs[0].inserted()).isfalse());
                wassert(actual(attrs[0].needs_update()).isfalse());
                wassert(actual(attrs[0].updated()).istrue());
            }
            // Load the attributes again to verify that they changed
            {
                Var var(f.query(1, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 75);
            }
            {
                Var var(f.query(2, 1));
                wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
                wassert(actual(*var.next_attr()) == 50);
            }

            // TODO: test a mix of update and insert
        });
    }
};

Tests tg1("db_sql_attr_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_ODBC
Tests tg2("db_sql_attr_v7_odbc", "ODBC", db::V7);
#endif
#ifdef HAVE_LIBPQ
Tests tg3("db_sql_attr_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_sql_attr_v7_mysql", "MYSQL", db::V7);
#endif

}
