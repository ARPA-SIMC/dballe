#include "config.h"
#include "db/test-utils-db.h"
#include "db/mem/db.h"
#include "db/v6/db.h"
#include "db/sql/station.h"

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
        stations["st1_synop"].values.set(newvar("block", 1));
        stations["st1_synop"].values.set(newvar("station", 1));
        stations["st1_synop"].values.set(newvar("B07030", 42.0)); // height
        stations["st1_metar"].info = stations["st1_synop"].info;
        stations["st1_metar"].info.report = "metar";
        stations["st1_metar"].values.set(newvar("block", 1));
        stations["st1_metar"].values.set(newvar("station", 2));
        stations["st1_metar"].values.set(newvar("B07030", 50.0)); // height
        stations["st2_temp"].info.coords = Coords(23.45670, 65.43210);
        stations["st2_temp"].info.report = "temp";
        stations["st2_temp"].values.set(newvar("block", 3));
        stations["st2_temp"].values.set(newvar("station", 4));
        stations["st2_temp"].values.set(newvar("B07030", 100.0)); // height
        stations["st2_metar"].info = stations["st2_temp"].info;
        stations["st2_metar"].info.report = "metar";
        stations["st2_metar"].values.set(newvar("block", 3));
        stations["st2_metar"].values.set(newvar("station", 4));
        stations["st2_metar"].values.set(newvar("B07030", 110.0)); // height
        data["rec1"].info = stations["st1_metar"].info;
        data["rec1"].info.datetime = Datetime(1945, 4, 25, 8);
        data["rec1"].info.level = Level(10, 11, 15, 22);
        data["rec1"].info.trange = Trange(20, 111, 122);
        data["rec1"].values.set("B12101", 290.0);
        data["rec2"].info = stations["st2_metar"].info;
        data["rec2"].info.datetime = Datetime(1945, 4, 25, 8);
        data["rec2"].info.level = Level(10, 11, 15, 22);
        data["rec2"].info.trange = Trange(20, 111, 122);
        data["rec2"].values.set("B12101", 300.0);
        data["rec2"].values.set("B12103", 298.0);
    }
};

struct Fixture : public dballe::tests::DBFixture
{
    const int some;
    const int all;

    Fixture()
        : some(db->format() == MEM ? 2 : 1), all(db->format() == MEM ? 4 : 2)
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
    {
        wruntest(populate<DBData>);
    }

    void reset()
    {
        dballe::tests::DBFixture::reset();
        wruntest(populate<DBData>);
    }
};

typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

#define TRY_QUERY(qstring, expected_count) wassert(actual(*f.db).try_data_query(qstring, expected_count))

std::vector<Test> tests {
    Test("query_ana_id", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_station_query("ana_id=1", 1));
        wassert(actual(db).try_station_query("ana_id=2", 1));
    }),
    Test("query_lat_lon", [](Fixture& f) {
        auto& db = *f.db;
        const auto some = f.some;
        const auto all = f.all;
        wassert(actual(db).try_station_query("lat=12.00000", 0));
        wassert(actual(db).try_station_query("lat=12.34560", some));
        wassert(actual(db).try_station_query("lat=23.45670", some));
        wassert(actual(db).try_station_query("latmin=12.00000", all));
        wassert(actual(db).try_station_query("latmin=12.34560", all));
        wassert(actual(db).try_station_query("latmin=12.34570", some));
        wassert(actual(db).try_station_query("latmin=23.45670", some));
        wassert(actual(db).try_station_query("latmin=23.45680", 0));
        wassert(actual(db).try_station_query("latmax=12.00000", 0));
        wassert(actual(db).try_station_query("latmax=12.34560", some));
        wassert(actual(db).try_station_query("latmax=12.34570", some));
        wassert(actual(db).try_station_query("latmax=23.45670", all));
        wassert(actual(db).try_station_query("latmax=23.45680", all));
        wassert(actual(db).try_station_query("lon=76.00000", 0));
        wassert(actual(db).try_station_query("lon=76.54320", some));
        wassert(actual(db).try_station_query("lon=65.43210", some));
        wassert(actual(db).try_station_query("lonmin=10., lonmax=20.", 0));
        wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=76.54320", some));
        wassert(actual(db).try_station_query("lonmin=76.54320, lonmax=77.", some));
        wassert(actual(db).try_station_query("lonmin=76.54330, lonmax=77.", 0));
        wassert(actual(db).try_station_query("lonmin=60., lonmax=77.", all));
        wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54310", some));
        wassert(actual(db).try_station_query("lonmin=77., lonmax=76.54320", all));
        wassert(actual(db).try_station_query("lonmin=77., lonmax=-10", 0));
    }),
    Test("query_mobile", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_station_query("mobile=0", f.all));
        wassert(actual(db).try_station_query("mobile=1", 0));
    }),
    Test("query_ident", [](Fixture& f) {
        // FIXME: add some mobile stations to the test fixture to test ident
    }),
    Test("query_block_station", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_station_query("B01001=1", f.some));
        wassert(actual(db).try_station_query("B01001=2", 0));
        wassert(actual(db).try_station_query("B01001=3", f.some));
        wassert(actual(db).try_station_query("B01001=4", 0));
        wassert(actual(db).try_station_query("B01002=1", 1));
        wassert(actual(db).try_station_query("B01002=2", 1));
        wassert(actual(db).try_station_query("B01002=3", 0));
        wassert(actual(db).try_station_query("B01002=4", f.some));
    }),
    Test("query_mobile", [](Fixture& f) {
        auto& db = *f.db;
    }),
    Test("query_ana_filter", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_station_query("ana_filter=block=1", f.some));
        wassert(actual(db).try_station_query("ana_filter=block=2", 0));
        wassert(actual(db).try_station_query("ana_filter=block=3", f.some));
        wassert(actual(db).try_station_query("ana_filter=block>=1", f.all));
        wassert(actual(db).try_station_query("ana_filter=B07030=42", 1));
        wassert(actual(db).try_station_query("ana_filter=B07030=50", 1));
        wassert(actual(db).try_station_query("ana_filter=B07030=100", 1));
        wassert(actual(db).try_station_query("ana_filter=B07030=110", 1));
        wassert(actual(db).try_station_query("ana_filter=B07030=120", 0));
        wassert(actual(db).try_station_query("ana_filter=B07030>50", f.some));
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
        if (db.format() == MEM)
            wassert(actual(db).try_station_query("ana_filter=B07030>=50", 3));
        else
            wassert(actual(db).try_station_query("ana_filter=B07030>=50", 2));
        wassert(actual(db).try_station_query("ana_filter=50<=B07030<=100", 2));
    }),
    Test("query_var", [](Fixture& f) {
        auto& db = *f.db;
        wassert(actual(db).try_station_query("var=B12101", 2));
        wassert(actual(db).try_station_query("var=B12103", 1));
        wassert(actual(db).try_station_query("varlist=B12101", 2));
        wassert(actual(db).try_station_query("varlist=B12103", 1));
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
        if (db.format() == MEM)
            wassert(actual(db).try_station_query("varlist=B12101,B12103", 3));
        else
            wassert(actual(db).try_station_query("varlist=B12101,B12103", 2));
    }),
    Test("stations_without_data", [](Fixture& f) {
        auto& db = *f.db;

        // Manually insert an orphan station
        switch (db.format())
        {
            case MEM:
                if (auto d = dynamic_cast<mem::DB*>(f.db))
                    d->memdb.stations.obtain_fixed(Coords(11.0, 45.0), "synop");
                break;
            case V6:
                if (auto d = dynamic_cast<v6::DB*>(f.db))
                    d->station().obtain_id(1100000, 4500000);
                break;
            case V5: throw error_unimplemented("v5 db is not supported");
            case MESSAGES: throw error_unimplemented("testing stations_without_data on MESSAGES database");
        }

        // Query stations and make sure that they do not appear. They should
        // not appear, but they currently do because of a bug. I need to
        // preserve the bug until the software that relies on it has been
        // migrated to use standard DB-All.e features.
        core::Query query;
        query.latrange.set(11.0, 11.0);
        query.lonrange.set(45.0, 45.0);
        auto cur = db.query_stations(query);
#warning TODO: fix this test to give an error once we do not need to support this bug anymore
        //wassert(actual(cur->remaining()) == 0);
        wassert(actual(cur->remaining()) == 1);
    }),
};

test_group tg1("db_query_station_mem", nullptr, db::MEM, tests);
test_group tg2("db_query_station_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_query_station_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_query_station_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_query_station_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
