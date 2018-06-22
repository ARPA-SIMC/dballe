#include "dballe/db/tests.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : EmptyTransactionFixture<V7DB>
{
    using EmptyTransactionFixture::EmptyTransactionFixture;

    dballe::Station sde1;
    dballe::Station sde2;
    int lt1;
    int lt2;

    Fixture(const char* backend)
        : EmptyTransactionFixture(backend)
    {
    }

    void create_db() override
    {
        EmptyTransactionFixture::create_db();

        sde1.report = "synop";
        sde1.coords = Coords(4500000, 1100000);
        sde1.ident = "ciao";

        sde2.report = "synop";
        sde2.coords = Coords(4600000, 1200000);
        sde2.ident = nullptr;

        auto t = dynamic_pointer_cast<dballe::db::v7::Transaction>(db->transaction());

        // Insert a mobile station
        sde1.id = wcallchecked(t->station().insert_new(sde1));

        // Insert a fixed station
        sde2.id = wcallchecked(t->station().insert_new(sde2));

        // Insert a lev_tr
        lt1 = t->levtr().obtain_id(db::v7::LevTrEntry(Level(1, 2, 0, 3), Trange(4, 5, 6)));

        // Insert another lev_tr
        lt2 = t->levtr().obtain_id(db::v7::LevTrEntry(Level(2, 3, 1, 4), Trange(5, 6, 7)));


        t->commit();
    }
};


class Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override;
};

Tests tg1("db_v7_data_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests tg3("db_v7_data_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_v7_data_mysql", "MYSQL");
#endif


void Tests::register_tests()
{

add_method("insert", [](Fixture& f) {
    using namespace dballe::db::v7;
    auto& da = f.tr->data();


    // Insert a datum
    {
        Var var(varinfo(WR_VAR(0, 1, 2)), 123);
        std::vector<batch::MeasuredDatum> vars;
        vars.emplace_back(f.lt1, &var);
        wassert(da.insert(*f.tr, f.sde1.id, Datetime(2001, 2, 3, 4, 5, 6), vars, false));
        wassert(actual(vars[0].id) == 1);
    }

    // Insert another datum
    {
        Var var(varinfo(WR_VAR(0, 1, 2)), 234);
        std::vector<batch::MeasuredDatum> vars;
        vars.emplace_back(f.lt2, &var);
        wassert(da.insert(*f.tr, f.sde2.id, Datetime(2002, 3, 4, 5, 6, 7), vars, false));
        wassert(actual(vars[0].id) == 2);
    }

    // Reinsert the first datum: it should give an error, since V7 does
    // not check if old and new values are the same
    // Disabled because postgresql spits error also on stderr
    //{
    //    Var var(varinfo(WR_VAR(0, 1, 2)), 123);
    //    std::vector<batch::MeasuredDatum> vars;
    //    vars.emplace_back(f.lt1, &var);
    //    auto e = wassert_throws(std::runtime_error, da.insert(*f.tr, f.tr->station().get_id(*f.tr, f.sde1), Datetime(2001, 2, 3, 4, 5, 6), vars));
    //    wassert(actual(e.what()).matches("refusing to overwrite existing data"));
    //}

    // Reinsert the first datum, with a different value and overwrite:
    // it should find its ID and update it
    {
        Var var(varinfo(WR_VAR(0, 1, 2)), 125);
        std::vector<batch::MeasuredDatum> vars;
        vars.emplace_back(1, f.lt1, &var);
        wassert(da.update(*f.tr, vars, false));
        wassert(actual(vars[0].id) == 1);
    }
});

add_method("attrs", [](Fixture& f) {
    using namespace dballe::db::v7;
    auto& da = f.tr->data();

    // Insert a datum with attributes
    Var var(varinfo(WR_VAR(0, 1, 2)), 123);
    var.seta(newvar(WR_VAR(0, 33, 7), 50));
    std::vector<batch::MeasuredDatum> vars;
    vars.emplace_back(f.lt1, &var);
    wassert(da.insert(*f.tr, f.sde1.id, Datetime(2001, 2, 3, 4, 5, 6), vars, true));
    int id = vars[0].id;

    vector<wreport::Var> attrs;
    da.read_attrs(id, [&](std::unique_ptr<wreport::Var> a) {
        attrs.emplace_back(*a);
    });

    wassert(actual(attrs.size()) == 1);
    wassert(actual_varcode(attrs[0].code()) == WR_VAR(0, 33, 7));
    wassert(actual(attrs[0]) == 50);
});

}

}
