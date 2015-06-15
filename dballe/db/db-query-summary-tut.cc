#include "config.h"
#include "db/test-utils-db.h"
#include <wibble/string.h>

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace {

struct DBData : public TestFixture
{
    DBData()
    {
        stations["st1_synop"].info.coords = Coords(12.34560, 76.54320);
        stations["st1_synop"].info.report = "synop";
        stations["st1_synop"].values.set(newvar("B07030", 42.0)); // height
        stations["st1_metar"].info = stations["st1_synop"].info;
        stations["st1_metar"].info.report = "metar";
        stations["st1_metar"].values.set(newvar("block", 1));
        stations["st1_metar"].values.set(newvar("station", 2));
        stations["st1_metar"].values.set(newvar("B07030", 50.0)); // height
        stations["st2_temp"].info.coords = Coords(23.45670, 65.43210);
        stations["st2_temp"].info.report = "temp";
        stations["st2_temp"].values.set(newvar("B07030", 100.0)); // height
        stations["st2_metar"].info = stations["st2_temp"].info;
        stations["st2_metar"].info.report = "metar";
        stations["st2_metar"].values.set(newvar("block", 3));
        stations["st2_metar"].values.set(newvar("station", 4));
        stations["st2_metar"].values.set(newvar("B07030", 110.0)); // height
        data["rec1a"].info = stations["st1_metar"].info;
        data["rec1a"].info.datetime = Datetime(1945, 4, 25, 8);
        data["rec1a"].info.level = Level(10, 11, 15, 22);
        data["rec1a"].info.trange = Trange(20, 111, 122);
        data["rec1a"].values.set("B12101", 290.0);
        data["rec1a"].values.set("B12103", 280.0);
        data["rec1b"] = data["rec1a"];
        data["rec1b"].info.datetime = Datetime(1945, 4, 26, 8);
        data["rec1b"].values.set("B12101", 291.0);
        data["rec1b"].values.set("B12103", 281.0);
        data["rec2a"].info = stations["st2_metar"].info;
        data["rec2a"].info.datetime = Datetime(1945, 4, 25, 8);
        data["rec2a"].info.level = Level(10, 11, 15, 22);
        data["rec2a"].info.trange = Trange(20, 111, 122);
        data["rec2a"].values.set("B12101", 300.0);
        data["rec2a"].values.set("B12103", 298.0);
        data["rec2b"] = data["rec2a"];
        data["rec2b"].info.datetime = Datetime(1945, 4, 26, 8);
        data["rec2b"].values.set("B12101", 301.0);
        data["rec2b"].values.set("B12103", 291.0);
    }
};

struct Fixture : public dballe::tests::DBFixture
{
    int st1_id;
    int st2_id;

    Fixture()
    {
        insert_data();
    }

    void insert_data()
    {
        DBData dbdata;
        wruntest(populate_database, dbdata);
        st1_id = dbdata.stations["st1_metar"].info.ana_id;
        st2_id = dbdata.stations["st2_metar"].info.ana_id;
    }

    void reset()
    {
        dballe::tests::DBFixture::reset();
        insert_data();
    }
};

typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

std::vector<Test> tests {
    Test("query_ana_id", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", f.st1_id), 2));
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", f.st2_id), 2));
        wassert(actual(db).try_summary_query(str::fmtf("ana_id=%d", (f.st1_id + f.st2_id) * 2), 0));
    }),
#if 0
    // TODO: summary of station vars is not supported at the moment, waiting for a use case for it
    Test("query_station_vars", [](Fixture& f) {
        auto& db = *f.db;
        core::Query query;
        query.query_station_vars = true;
        unique_ptr<db::Cursor> cur = db.query_summary(query);
        ensure_equals(cur->test_iterate(), 8);
    }),
#endif
    Test("query_year", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("year=1001", 0));
        wassert(actual(db).try_summary_query("yearmin=1999", 0));
        auto check_base = [](WIBBLE_TEST_LOCPRM, const vector<core::Record>& res) {
            wassert(actual(res[0].enq("lat", MISSING_INT)) == 1234560);
            wassert(actual(res[0].enq("lon", MISSING_INT)) == 7654320);
            wassert(actual(res[0].get_level()) == Level(10, 11, 15, 22));
            wassert(actual(res[0].get_trange()) == Trange(20, 111, 122));
            wassert(actual(res[0].enq("var", "")) == "B12101");
        };
        auto check_nodetails = [&](WIBBLE_TEST_LOCPRM, const vector<core::Record>& res) {
            wruntest(check_base, res);
            wassert(actual(res[0].isset("context_id")).isfalse());
            wassert(actual(res[0].isset("yearmin")).isfalse());
            wassert(actual(res[0].isset("yearmax")).isfalse());
        };
        auto check_details = [&](WIBBLE_TEST_LOCPRM, const vector<core::Record>& res) {
            wruntest(check_base, res);
            wassert(actual(res[0].enq("context_id", MISSING_INT)) == 2);
            DatetimeRange dtr = res[0].get_datetimerange();
            wassert(actual(dtr.min) == Datetime(1945, 4, 25, 8));
            wassert(actual(dtr.max) == Datetime(1945, 4, 26, 8));
        };
        wassert(actual(db).try_summary_query("yearmin=1945", 4, check_nodetails));
        wassert(actual(db).try_summary_query("yearmin=1945, query=details", 4, check_details));
        wassert(actual(db).try_summary_query("yearmax=1944", 0));
        wassert(actual(db).try_summary_query("yearmax=1945", 4));
        wassert(actual(db).try_summary_query("yearmax=2030", 4));
        wassert(actual(db).try_summary_query("year=1944", 0));
        wassert(actual(db).try_summary_query("year=1945", 4));
        wassert(actual(db).try_summary_query("year=1946", 0));
    }),
    Test("query_blockstation", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("B01001=1", 2));
        wassert(actual(db).try_summary_query("B01001=2", 0));
        wassert(actual(db).try_summary_query("B01002=3", 0));
        wassert(actual(db).try_summary_query("B01002=4", 2));
    }),
    Test("query_ana_filter", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("ana_filter=block=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=block>1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001>1", 2));
        wassert(actual(db).try_summary_query("ana_filter=block<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001>3", 0));
        wassert(actual(db).try_summary_query("ana_filter=B01001>=3", 2));
        wassert(actual(db).try_summary_query("ana_filter=B01001<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=0<=B01001<=2", 2));
        wassert(actual(db).try_summary_query("ana_filter=1<=B01001<=1", 2));
        wassert(actual(db).try_summary_query("ana_filter=2<=B01001<=4", 2));
        wassert(actual(db).try_summary_query("ana_filter=4<=B01001<=6", 0));
    }),
    Test("query_data_filter", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("data_filter=B12101<300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101<=300.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12101=300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101>=300,0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101>300.0", 1));
        wassert(actual(db).try_summary_query("data_filter=B12101<400.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12101<=400.0", 2));
        wassert(actual(db).try_summary_query("data_filter=B12102>400.0", 0));
    }),
    Test("query_lat_lon", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("latmin=11.0", 4));
        wassert(actual(db).try_summary_query("latmin=12.34560", 4));
        wassert(actual(db).try_summary_query("latmin=13.0", 2));
        wassert(actual(db).try_summary_query("latmax=11.0", 0));
        wassert(actual(db).try_summary_query("latmax=12.34560", 2));
        wassert(actual(db).try_summary_query("latmax=13.0", 2));
        wassert(actual(db).try_summary_query("lonmin=75., lonmax=77.", 2));
        wassert(actual(db).try_summary_query("lonmin=76.54320, lonmax=76.54320", 2));
        wassert(actual(db).try_summary_query("lonmin=76.54330, lonmax=77.", 0));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=76.54310", 2));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=76.54320", 4));
        wassert(actual(db).try_summary_query("lonmin=77., lonmax=-10", 0));
    }),
    Test("query_mobile", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("mobile=0", 4));
        wassert(actual(db).try_summary_query("mobile=1", 0));
    }),
    Test("query_ident", [](Fixture& f) {
        //auto& db = *f.db;
        // TODO: add mobile stations to the fixture so we can query ident
    }),
    Test("query_timerange", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("pindicator=20", 4));
        wassert(actual(db).try_summary_query("pindicator=21", 0));
        wassert(actual(db).try_summary_query("p1=111", 4));
        wassert(actual(db).try_summary_query("p1=112", 0));
        wassert(actual(db).try_summary_query("p2=121", 0));
        wassert(actual(db).try_summary_query("p2=122", 4));
        wassert(actual(db).try_summary_query("p2=123", 0));
    }),
    Test("query_level", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("leveltype1=10", 4));
        wassert(actual(db).try_summary_query("leveltype1=11", 0));
        wassert(actual(db).try_summary_query("leveltype2=15", 4));
        wassert(actual(db).try_summary_query("leveltype2=16", 0));
        wassert(actual(db).try_summary_query("l1=11", 4));
        wassert(actual(db).try_summary_query("l1=12", 0));
        wassert(actual(db).try_summary_query("l2=22", 4));
        wassert(actual(db).try_summary_query("l2=23", 0));
    }),
    Test("query_var", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("var=B01001", 0));
        wassert(actual(db).try_summary_query("var=B12101", 2));
        wassert(actual(db).try_summary_query("var=B12102", 0));
    }),
    Test("query_rep_memo", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("rep_memo=synop", 0));
        wassert(actual(db).try_summary_query("rep_memo=metar", 4));
        wassert(actual(db).try_summary_query("rep_memo=temp", 0));
    }),
    Test("query_priority", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_summary_query("priority=101", 0));
        wassert(actual(db).try_summary_query("priority=81", 4));
        wassert(actual(db).try_summary_query("priority=102", 0));
        wassert(actual(db).try_summary_query("priomin=70", 4));
        wassert(actual(db).try_summary_query("priomin=80", 4));
        wassert(actual(db).try_summary_query("priomin=90", 0));
        wassert(actual(db).try_summary_query("priomax=70", 0));
        wassert(actual(db).try_summary_query("priomax=81", 4));
        wassert(actual(db).try_summary_query("priomax=100", 4));
    }),
    Test("query_context_id", [](Fixture& f) {
        auto& db = *f.db;
        // Collect a vector of valid context IDs
        vector<int> context_ids;
        // And an invalid one
        int sum = 1;
        unique_ptr<db::Cursor> cur = db.query_data(core::Query());
        while (cur->next())
        {
            context_ids.push_back(cur->attr_reference_id());
            sum += context_ids.back();
        }

        WIBBLE_TEST_INFO(info);

        wassert(actual(db).try_summary_query(str::fmtf("context_id=%d", sum), 0));
        for (vector<int>::const_iterator i = context_ids.begin(); i != context_ids.end(); ++i)
        {
            info() << "Context ID " << *i;
            wassert(actual(db).try_summary_query(str::fmtf("context_id=%d", *i), 1));
        }
    }),
};

test_group tg1("db_query_summary_mem", nullptr, db::MEM, tests);
test_group tg2("db_query_summary_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_query_summary_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_query_summary_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_query_summary_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
