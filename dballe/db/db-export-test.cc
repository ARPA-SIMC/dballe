#include "config.h"
#include "db/tests.h"
#include "db/db.h"
#include "dballe/record.h"
#include "msg/msg.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct DBData : public TestDataSet
{
    DBData()
    {
        data["ds0"].info.coords = Coords(12.34560, 76.54321);
        data["ds0"].info.report = "synop";
        data["ds0"].info.datetime = Datetime(1945, 4, 25, 8, 0);
        data["ds0"].info.level = Level(1, 2, 0, 3);
        data["ds0"].info.trange = Trange(4, 5, 6);
        data["ds0"].values.set("B01012", 500);
        data["ds1"] = data["ds0"];
        data["ds1"].info.datetime = Datetime(1945, 4, 26, 8, 0);
        data["ds1"].values.set("B01012", 400);

        data["ds2"].info.coords = Coords(12.34560, 76.54321);
        data["ds2"].info.report = "synop";
        data["ds2"].info.ident = "ciao";
        data["ds2"].info.datetime = Datetime(1945, 4, 26, 8, 0);
        data["ds2"].info.level = Level(1, 2, 0, 3);
        data["ds2"].info.trange = Trange(4, 5, 6);
        data["ds2"].values.set("B01012", 300);
        data["ds3"] = data["ds2"];
        data["ds3"].info.report = "metar";
        data["ds3"].values.set("B01012", 200);
    }
};

class Tests : public DBFixtureTestCase<DBFixture>
{
    using DBFixtureTestCase::DBFixtureTestCase;

    void register_tests() override;
};

void Tests::register_tests()
{

add_method("export", [](Fixture& f) {
    // Simple export
    auto& db = f.db;
    wassert(f.populate<DBData>());

    // Put some data in the database and check that it gets exported properly
    // Query back the data
    Messages messages = dballe::tests::messages_from_db(*db, core::Query());
    wassert(actual(messages.size()) == 4u);

    int synmsg = 2;
    int metmsg = 3;
    if (Msg::downcast(messages[2]).type == MSG_METAR)
    {
        // Since the order here is not determined, enforce one
        std::swap(synmsg, metmsg);
    }

    wassert(actual(Msg::downcast(messages[0]).type) == MSG_SYNOP);
    wassert(actual(messages[0], DBA_MSG_LATITUDE) == 12.34560);
    wassert(actual(messages[0], DBA_MSG_LONGITUDE) == 76.54321);
    wassert(actual(messages[0]).is_undef(DBA_MSG_IDENT));
    wassert(actual(messages[0].get_datetime()) == Datetime(1945, 4, 25, 8, 0, 0));
    wassert(actual(messages[0], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 500);

    wassert(actual(Msg::downcast(messages[1]).type) == MSG_SYNOP);
    wassert(actual(messages[1], DBA_MSG_LATITUDE) == 12.34560);
    wassert(actual(messages[1], DBA_MSG_LONGITUDE) == 76.54321);
    wassert(actual(messages[1]).is_undef(DBA_MSG_IDENT));
    wassert(actual(messages[1].get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual(messages[1], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 400);

    wassert(actual(Msg::downcast(messages[synmsg]).type) == MSG_SYNOP);
    wassert(actual(messages[synmsg], DBA_MSG_LATITUDE) == 12.34560);
    wassert(actual(messages[synmsg], DBA_MSG_LONGITUDE) == 76.54321);
    wassert(actual(messages[synmsg], DBA_MSG_IDENT), "ciao");
    wassert(actual(messages[synmsg].get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual(messages[synmsg], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 300);

    wassert(actual(Msg::downcast(messages[metmsg]).type) == MSG_METAR);
    wassert(actual(messages[metmsg], DBA_MSG_LATITUDE) == 12.34560);
    wassert(actual(messages[metmsg], DBA_MSG_LONGITUDE) == 76.54321);
    wassert(actual(messages[metmsg], DBA_MSG_IDENT), "ciao");
    wassert(actual(messages[metmsg].get_datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
    wassert(actual(messages[metmsg], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)) == 200);
});
add_method("export", [](Fixture& f) {
    // Text exporting of extra station information
    auto& db = f.db;

    // Import some data in the station extra information context
    StationValues st;
    // do not set datetime, level, trange, to insert a station variable
    st.info.coords = Coords(45.0, 11.0);
    st.info.report = "synop";
    st.values.set("B01001", 10);
    db->insert_station_data(st, false, true);

    // Import one real datum
    DataValues dv;
    dv.info = st.info;
    dv.info.datetime = Datetime(2000, 1, 1, 0, 0, 0);
    dv.info.level = Level(103, 2000);
    dv.info.trange = Trange(254, 0, 0);
    dv.values.set("B12101", 290.0);
    db->insert_data(dv, false, true);

    // Query back the data
    Messages msgs = dballe::tests::messages_from_db(*db, core::Query());
    wassert(actual(msgs.size()) == 1u);
    Msg& msg = Msg::downcast(msgs[0]);

    wassert(actual(msg.type) == MSG_SYNOP);
    wassert(actual(msgs[0], DBA_MSG_LATITUDE) == 45.0);
    wassert(actual(msgs[0], DBA_MSG_LONGITUDE) == 11.0);
    wassert(actual(msgs[0]).is_undef(DBA_MSG_IDENT));
    wassert(actual(msgs[0], DBA_MSG_BLOCK) == 10);
    wassert(actual(msgs[0], DBA_MSG_TEMP_2M) == 290.0);
});

add_method("missing_repmemo", [](Fixture& f) {
    // Text exporting of extra station information
    auto& db = f.db;
    core::Query query;
    query.rep_memo = "nonexisting";
    Messages msgs = wcallchecked(dballe::tests::messages_from_db(*db, query));
    wassert(actual(msgs.size()) == 0u);
});

}

Tests tg2("db_export_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_LIBPQ
Tests tg4("db_export_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg5("db_export_v6_mysql", "MYSQL", db::V6);
#endif
Tests tg6("db_export_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_LIBPQ
Tests tg7("db_export_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg8("db_export_v7_mysql", "MYSQL", db::V7);
#endif

}
