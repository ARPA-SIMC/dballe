#include "config.h"
#include "db/tests.h"
#include "db/v6/db.h"
#include "db/v7/db.h"
#include "db/v7/transaction.h"
#include "db/v6/station.h"
#include "db/v7/station.h"

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

template<typename DB>
class Tests : public FixtureTestCase<TransactionFixture<DB, DBData>>
{
    typedef TransactionFixture<DB, DBData> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override
    {
        this->add_method("query_ana_id", [](Fixture& f) {
            wassert(actual(f.tr).try_station_query("ana_id=1", 1));
            wassert(actual(f.tr).try_station_query("ana_id=2", 1));
        });
        this->add_method("query_lat_lon", [](Fixture& f) {
            const auto some = DB::format != V6 ? 2 : 1;
            const auto all = DB::format != V6 ? 4 : 2;
            wassert(actual(f.tr).try_station_query("lat=12.00000", 0));
            wassert(actual(f.tr).try_station_query("lat=12.34560", some));
            wassert(actual(f.tr).try_station_query("lat=23.45670", some));
            wassert(actual(f.tr).try_station_query("latmin=12.00000", all));
            wassert(actual(f.tr).try_station_query("latmin=12.34560", all));
            wassert(actual(f.tr).try_station_query("latmin=12.34570", some));
            wassert(actual(f.tr).try_station_query("latmin=23.45670", some));
            wassert(actual(f.tr).try_station_query("latmin=23.45680", 0));
            wassert(actual(f.tr).try_station_query("latmax=12.00000", 0));
            wassert(actual(f.tr).try_station_query("latmax=12.34560", some));
            wassert(actual(f.tr).try_station_query("latmax=12.34570", some));
            wassert(actual(f.tr).try_station_query("latmax=23.45670", all));
            wassert(actual(f.tr).try_station_query("latmax=23.45680", all));
            wassert(actual(f.tr).try_station_query("lon=76.00000", 0));
            wassert(actual(f.tr).try_station_query("lon=76.54320", some));
            wassert(actual(f.tr).try_station_query("lon=65.43210", some));
            wassert(actual(f.tr).try_station_query("lonmin=10., lonmax=20.", 0));
            wassert(actual(f.tr).try_station_query("lonmin=76.54320, lonmax=76.54320", some));
            wassert(actual(f.tr).try_station_query("lonmin=76.54320, lonmax=77.", some));
            wassert(actual(f.tr).try_station_query("lonmin=76.54330, lonmax=77.", 0));
            wassert(actual(f.tr).try_station_query("lonmin=60., lonmax=77.", all));
            wassert(actual(f.tr).try_station_query("lonmin=77., lonmax=76.54310", some));
            wassert(actual(f.tr).try_station_query("lonmin=77., lonmax=76.54320", all));
            wassert(actual(f.tr).try_station_query("lonmin=77., lonmax=-10", 0));
        });
        this->add_method("query_mobile", [](Fixture& f) {
            const auto all = DB::format != V6 ? 4 : 2;
            wassert(actual(f.tr).try_station_query("mobile=0", all));
            wassert(actual(f.tr).try_station_query("mobile=1", 0));
        });
        this->add_method("query_ident", [](Fixture& f) {
            // FIXME: add some mobile stations to the test fixture to test ident
        });
        this->add_method("query_block_station", [](Fixture& f) {
            const auto some = DB::format != V6 ? 2 : 1;
            wassert(actual(f.tr).try_station_query("B01001=1", some));
            wassert(actual(f.tr).try_station_query("B01001=2", 0));
            wassert(actual(f.tr).try_station_query("B01001=3", some));
            wassert(actual(f.tr).try_station_query("B01001=4", 0));
            wassert(actual(f.tr).try_station_query("B01002=1", 1));
            wassert(actual(f.tr).try_station_query("B01002=2", 1));
            wassert(actual(f.tr).try_station_query("B01002=3", 0));
            wassert(actual(f.tr).try_station_query("B01002=4", some));
        });
        this->add_method("query_mobile", [](Fixture& f) {
        });
        this->add_method("query_ana_filter", [](Fixture& f) {
            const auto some = DB::format != V6 ? 2 : 1;
            const auto all = DB::format != V6 ? 4 : 2;
            wassert(actual(f.tr).try_station_query("ana_filter=block=1", some));
            wassert(actual(f.tr).try_station_query("ana_filter=block=2", 0));
            wassert(actual(f.tr).try_station_query("ana_filter=block=3", some));
            wassert(actual(f.tr).try_station_query("ana_filter=block>=1", all));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030=42", 1));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030=50", 1));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030=100", 1));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030=110", 1));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030=120", 0));
            wassert(actual(f.tr).try_station_query("ana_filter=B07030>50", some));
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
            if (f.db->format() != V6)
                wassert(actual(f.tr).try_station_query("ana_filter=B07030>=50", 3));
            else
                wassert(actual(f.tr).try_station_query("ana_filter=B07030>=50", 2));
            wassert(actual(f.tr).try_station_query("ana_filter=50<=B07030<=100", 2));
        });
        this->add_method("query_var", [](Fixture& f) {
            /*
             * Querying var= or varlist= on a station query means querying stations
             * that measure that variable or those variables.
             */
            wassert(actual(f.tr).try_station_query("var=B12101", 2));
            wassert(actual(f.tr).try_station_query("var=B12103", 1));
            wassert(actual(f.tr).try_station_query("varlist=B12101", 2));
            wassert(actual(f.tr).try_station_query("varlist=B12103", 1));
            wassert(actual(f.tr).try_station_query("varlist=B12101,B12103", 2));
        });
        this->add_method("stations_without_data", [](Fixture& f) {
            // Manually insert an orphan station
            switch (DB::format)
            {
                case V6:
                    if (auto d = dynamic_cast<v6::DB*>(f.db.get()))
                        d->station().obtain_id(1100000, 4500000);
                    break;
                case V7:
                    if (auto d = dynamic_cast<v7::DB*>(f.db.get()))
                    {
                        db::v7::StationDesc sde;
                        db::v7::State state;
                        sde.rep = 1;
                        sde.coords = Coords(1100000, 4500000);
                        sde.ident = "ciao";
                        d->station().obtain_id(state, sde);
                    }
                    break;
                case V5: throw error_unimplemented("v5 db is not supported");
                case MESSAGES: throw error_unimplemented("testing stations_without_data on MESSAGES database");
                case MEM: throw error_unimplemented("MEM databases should not be created anymore");
            }

            // Query stations and make sure that they do not appear. They should
            // not appear, but they currently do because of a bug. I need to
            // preserve the bug until the software that relies on it has been
            // migrated to use standard DB-All.e features.
            core::Query query;
            query.latrange.set(11.0, 11.0);
            query.lonrange.set(45.0, 45.0);
            auto cur = f.tr->query_stations(query);
#warning TODO: fix this test to give an error once we do not need to support this bug anymore
            //wassert(actual(cur->remaining()) == 0);
            wassert(actual(cur->remaining()) == 1);
        });
        this->add_method("query_ordering", [](Fixture& f) {
            auto cur = f.tr->query_stations(core::Query());
            switch (f.db->format())
            {
                case V7:
                    wassert(actual(cur->remaining()) == 4);
                    break;
                case V6:
                    wassert(actual(cur->remaining()) == 2);
                    break;
                default: error_unimplemented::throwf("cannot run this test on a database of format %d", (int)DB::format);
            }
        });
        this->add_method("query_rep_memo", [](Fixture& f) {
            // https://github.com/ARPA-SIMC/dballe/issues/35
            wassert(actual(f.tr).try_station_query("rep_memo=synop", 1));
            wassert(actual(f.tr).try_station_query("rep_memo=metar", 2));
        });
    }
};

Tests<V6DB> tg1("db_query_station_v6_sqlite", "SQLITE");
Tests<V7DB> tg2("db_query_station_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V6DB> tg3("db_query_station_v6_postgresql", "POSTGRESQL");
Tests<V7DB> tg4("db_query_station_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V6DB> tg5("db_query_station_v6_mysql", "MYSQL");
Tests<V7DB> tg6("db_query_station_v7_mysql", "MYSQL");
#endif

}
