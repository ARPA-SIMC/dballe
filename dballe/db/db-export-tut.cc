#include "config.h"
#include "db/test-utils-db.h"
#include "db/db.h"
#include "dballe/record.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct MsgCollector : public vector<Msg*>, public MsgConsumer
{
    ~MsgCollector()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
    }
    void operator()(unique_ptr<Msg> msg) override
    {
        push_back(msg.release());
    }
};

struct DBData : public dballe::tests::TestFixture
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

struct Fixture : public dballe::tests::DBFixture
{
    Fixture()
    {
    }

    void populate_database()
    {
        wruntest(populate<DBData>);
    }
};


typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("export", [](Fixture& f) {
        // Simple export
        auto& db = f.db;
        f.populate_database();

        // Put some data in the database and check that it gets exported properly
        // Query back the data
        MsgCollector msgs;
        db->export_msgs(core::Query(), msgs);
        ensure_equals(msgs.size(), 4u);

        if (msgs[2]->type == MSG_METAR)
        {
            // Since the order here is not determined, enforce one
            Msg* tmp = msgs[2];
            msgs[2] = msgs[3];
            msgs[3] = tmp;
        }

        ensure_equals(msgs[0]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LONGITUDE), 76.54321);
        ensure_msg_undef(*msgs[0], DBA_MSG_IDENT);
        wassert(actual(msgs[0]->datetime()) == Datetime(1945, 4, 25, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[0], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 500);

        ensure_equals(msgs[1]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[1], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[1], DBA_MSG_LONGITUDE), 76.54321);
        ensure_msg_undef(*msgs[1], DBA_MSG_IDENT);
        wassert(actual(msgs[1]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[1], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 400);

        ensure_equals(msgs[2]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_LONGITUDE), 76.54321);
        ensure_var_equals(want_var(*msgs[2], DBA_MSG_IDENT), "ciao");
        wassert(actual(msgs[2]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[2], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 300);

        ensure_equals(msgs[3]->type, MSG_METAR);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_LATITUDE), 12.34560);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_LONGITUDE), 76.54321);
        ensure_var_equals(want_var(*msgs[3], DBA_MSG_IDENT), "ciao");
        wassert(actual(msgs[3]->datetime()) == Datetime(1945, 4, 26, 8, 0, 0));
        ensure_var_equals(want_var(*msgs[3], WR_VAR(0, 1, 12), Level(1, 2, 0, 3), Trange(4, 5, 6)), 200);
    }),
    Test("export", [](Fixture& f) {
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
        MsgCollector msgs;
        db->export_msgs(core::Query(), msgs);
        ensure_equals(msgs.size(), 1u);

        ensure_equals(msgs[0]->type, MSG_SYNOP);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LATITUDE), 45.0);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_LONGITUDE), 11.0);
        ensure_msg_undef(*msgs[0], DBA_MSG_IDENT);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_BLOCK), 10);
        ensure_var_equals(want_var(*msgs[0], DBA_MSG_TEMP_2M), 290.0);
    }),
};

test_group tg1("db_export_mem", nullptr, db::MEM, tests);
test_group tg2("db_export_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_export_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_export_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_export_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
