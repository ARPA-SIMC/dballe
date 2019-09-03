#include "dballe/db/tests.h"
#include "dballe/db/db.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/core/shortcuts.h"
#include "dballe/msg/msg.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::impl;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct DBData : public TestDataSet
{
    DBData()
    {
        data["ds0"].station.coords = Coords(12.34560, 76.54321);
        data["ds0"].station.report = "synop";
        data["ds0"].datetime = Datetime(1945, 4, 25, 8, 0);
        data["ds0"].level = Level(1, 2, 0, 3);
        data["ds0"].trange = Trange(4, 5, 6);
        data["ds0"].values.set("B01012", 500);
        data["ds1"] = data["ds0"];
        data["ds1"].datetime = Datetime(1945, 4, 26, 8, 0);
        data["ds1"].values.set("B01012", 400);

        data["ds2"].station.coords = Coords(12.34560, 76.54321);
        data["ds2"].station.report = "synop";
        data["ds2"].station.ident = "ciao";
        data["ds2"].datetime = Datetime(1945, 4, 26, 8, 0);
        data["ds2"].level = Level(1, 2, 0, 3);
        data["ds2"].trange = Trange(4, 5, 6);
        data["ds2"].values.set("B01012", 300);
        data["ds3"] = data["ds2"];
        data["ds3"].station.report = "metar";
        data["ds3"].values.set("B01012", 200);
    }
};

template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2("db_export_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("db_export_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("db_export_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests()
{

this->add_method("export", [](Fixture& f) {
    // Simple export
    DBData test_data;
    wassert(f.populate(test_data));

    // Put some data in the database and check that it gets exported properly
    // Query back the data
    impl::Messages messages = dballe::tests::messages_from_db(f.tr, core::Query());
    wassert(actual(messages.size()) == 4u);

    int synmsg = 2;
    int metmsg = 3;
    if (impl::Message::downcast(messages[2])->type == MessageType::METAR)
    {
        // Since the order here is not determined, enforce one
        std::swap(synmsg, metmsg);
    }

    wassert(actual(impl::Message::downcast(messages[0])->type) == MessageType::SYNOP);
    wassert(actual_var(*messages[0], sc::latitude) == 12.34560);
    wassert(actual_var(*messages[0], sc::longitude) == 76.54321);
    wassert(actual(*messages[0]).is_undef(sc::ident));
    wassert(actual(messages[0]->get_datetime()) == Datetime(1945, 4, 25, 8, 0, 0));
    wassert(actual_var(*messages[0], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 500);

    wassert(actual(impl::Message::downcast(messages[1])->type) == MessageType::SYNOP);
    wassert(actual_var(*messages[1], sc::latitude) == 12.34560);
    wassert(actual_var(*messages[1], sc::longitude) == 76.54321);
    wassert(actual(*messages[1]).is_undef(sc::ident));
    wassert(actual(messages[1]->get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual_var(*messages[1], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 400);

    wassert(actual(impl::Message::downcast(messages[synmsg])->type) == MessageType::SYNOP);
    wassert(actual_var(*messages[synmsg], sc::latitude) == 12.34560);
    wassert(actual_var(*messages[synmsg], sc::longitude) == 76.54321);
    wassert(actual_var(*messages[synmsg], sc::ident), "ciao");
    wassert(actual(messages[synmsg]->get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual_var(*messages[synmsg], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 300);

    wassert(actual(impl::Message::downcast(messages[metmsg])->type) == MessageType::METAR);
    wassert(actual_var(*messages[metmsg], sc::latitude) == 12.34560);
    wassert(actual_var(*messages[metmsg], sc::longitude) == 76.54321);
    wassert(actual_var(*messages[metmsg], sc::ident), "ciao");
    wassert(actual(messages[metmsg]->get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual_var(*messages[metmsg], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 200);
});
this->add_method("export", [](Fixture& f) {
    // Text exporting of extra station information

    // Import some data in the station extra information context
    core::Data st;
    // do not set datetime, level, trange, to insert a station variable
    st.station.coords = Coords(45.0, 11.0);
    st.station.report = "synop";
    st.values.set("B01001", 10);
    f.tr->insert_station_data(st);

    // Import one real datum
    core::Data dv;
    dv.station = st.station;
    dv.datetime = Datetime(2000, 1, 1, 0, 0, 0);
    dv.level = Level(103, 2000);
    dv.trange = Trange(254, 0, 0);
    dv.values.set("B12101", 290.0);
    f.tr->insert_data(dv);

    // Query back the data
    impl::Messages msgs = dballe::tests::messages_from_db(f.tr, core::Query());
    wassert(actual(msgs.size()) == 1u);
    auto msg = impl::Message::downcast(msgs[0]);

    wassert(actual(msg->type) == MessageType::SYNOP);
    wassert(actual_var(*msgs[0], sc::latitude) == 45.0);
    wassert(actual_var(*msgs[0], sc::longitude) == 11.0);
    wassert(actual(*msgs[0]).is_undef(sc::ident));
    wassert(actual_var(*msgs[0], sc::block) == 10);
    wassert(actual_var(*msgs[0], sc::temp_2m) == 290.0);
});

this->add_method("missing_repmemo", [](Fixture& f) {
    // Text exporting of extra station information
    core::Query query;
    query.report = "nonexisting";
    impl::Messages msgs = wcallchecked(dballe::tests::messages_from_db(f.tr, query));
    wassert(actual(msgs.size()) == 0u);
});

}

}
