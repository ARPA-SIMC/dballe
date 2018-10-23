#define _DBALLE_TEST_CODE
#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "summary.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

template<typename DB, typename STATION>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB, Station> tg1("db_summary_v7_sqlite_summary", "SQLITE");
Tests<V7DB, DBStation> tg2("db_summary_v7_sqlite_dbsummary", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB, Station> tg3("db_summary_v7_postgresql_summary", "POSTGRESQL");
Tests<V7DB, DBStation> tg4("db_summary_v7_postgresql_dbsummary", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB, Station> tg5("db_summary_v7_mysql_summary", "MYSQL");
Tests<V7DB, DBStation> tg6("db_summary_v7_mysql_dbsummary", "MYSQL");
#endif

void station_id_isset(const Station& station) {}
void station_id_isset(const DBStation& station) { wassert(actual(station.id) != MISSING_INT); }

void set_query_station(core::Query& query, const Station& station)
{
    query.rep_memo = station.report;
    query.set_latrange(LatRange(station.coords.lat, station.coords.lat));
    query.set_lonrange(LonRange(station.coords.lon, station.coords.lon));
    query.ident = station.ident;
}
void set_query_station(core::Query& query, const DBStation& station)
{
    query.ana_id = station.id;
}

template<typename DB, typename STATION>
void Tests<DB, STATION>::register_tests()
{

this->add_method("summary", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    BaseSummary<STATION> s;
    wassert_true(s.datetime_min().is_missing());
    wassert_true(s.datetime_max().is_missing());
    wassert(actual(s.data_count()) == 0u);

    // Build the whole db summary
    core::Query query;
    query.query = "details";
    auto cur = f.tr->query_summary(query);
    while (cur->next())
        s.add_cursor(*cur);

    // Check its contents
    wassert(actual(s.stations().size()) == 2);
    wassert(station_id_isset(s.stations().begin()->station));
    wassert(actual(s.levels().size()) == 1);
    wassert(actual(s.tranges().size()) == 2);
    wassert(actual(s.varcodes().size()) == 2);
    wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
    wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
    wassert(actual(s.data_count()) == 4);

    set_query_station(query, s.stations().begin()->station);
    BaseSummary<STATION> s1;
    s1.add_filtered(s, query);
    wassert(actual(s1.stations().size()) == 1);
    // wassert(actual(s1.stations().begin()->station.id) == query.ana_id);
});

this->add_method("summary_msg", [](Fixture& f) {
    BaseSummary<STATION> s;

    // Summarise a message
    Messages msgs = dballe::tests::read_msgs("bufr/synop-rad1.bufr", File::BUFR, msg::ImporterOptions::from_string("accurate"));
    s.add_messages(msgs);

    // Check its contents
    wassert(actual(s.stations().size()) == 25);
    wassert(actual(s.levels().size()) == 37);
    wassert(actual(s.tranges().size()) == 9);
    wassert(actual(s.varcodes().size()) == 34);
    wassert(actual(s.datetime_min()) == Datetime(2015, 3, 5, 3));
    wassert(actual(s.datetime_max()) == Datetime(2015, 3, 5, 3));
    wassert(actual(s.data_count()) == 970);
});

this->add_method("merge_entries", [](Fixture& f) {
    STATION station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    BaseSummary<STATION> summary;
    wassert(actual(summary.data_count()) == 0u);

    summary.add(station, vd, dtrange, 12u);
    wassert(actual(summary.data_count()) == 12u);

    summary.add(station, vd, dtrange, 12u);
    wassert(actual(summary.data_count()) == 24u);

    wassert(actual(summary.stations().size()) == 1);
    wassert(actual(summary.stations().begin()->size()) == 1);

    summary.add(station, vd, dtrange, 12u);
    station.report = "test1";
    summary.add(station, vd, dtrange, 12u);
    summary.add(station, vd, dtrange, 12u);
    wassert(actual(summary.stations().size()) == 2);
    wassert(actual(summary.stations().begin()->size()) == 1);
    wassert(actual(summary.stations().rbegin()->size()) == 1);
    wassert(actual(summary.data_count()) == 36u + 24u);
});

this->add_method("json_summary", [](Fixture& f) {
    STATION station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    core::Query query;
    query.rep_memo = "synop";
    BaseSummary<STATION> summary;
    summary.add(station, vd, dtrange, 12);
    vd.varcode = WR_VAR(0, 1, 113);
    summary.add(station, vd, dtrange, 12);

    std::stringstream json;
    core::JSONWriter writer(json);
    summary.to_json(writer);

    wassert(actual(json.str()) == R"({"e":[{"s":{"r":"test","c":[4450000,1150000]},"v":[{"l":[1,null,null,null],"t":[254,0,0],"v":368,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12},{"l":[1,null,null,null],"t":[254,0,0],"v":369,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12}]}]})");

    json.seekg(0);
    core::json::Stream in(json);
    BaseSummary<STATION> summary1;
    wassert(summary1.from_json(in));
    wassert_true(summary == summary1);
    wassert(actual(summary.stations().size()) == summary1.stations().size());
    wassert_true(summary.stations() == summary1.stations());
    wassert_true(summary.reports() == summary1.reports());
    wassert_true(summary.levels() == summary1.levels());
    wassert_true(summary.tranges() == summary1.tranges());
    wassert_true(summary.varcodes() == summary1.varcodes());
    wassert_true(summary.datetime_min() == summary1.datetime_min());
    wassert_true(summary.datetime_max() == summary1.datetime_max());
    wassert_true(summary.data_count() == summary1.data_count());
});

}

}
