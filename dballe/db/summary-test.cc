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

template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg2("db_summary_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg4("db_summary_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg6("db_summary_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests()
{

this->add_method("summary", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    Summary s;
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
    wassert(actual(s.levels().size()) == 1);
    wassert(actual(s.tranges().size()) == 2);
    wassert(actual(s.varcodes().size()) == 2);
    wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
    wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
    wassert(actual(s.data_count()) == 4);
});

this->add_method("merge_entries", [](Fixture& f) {
    Station station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    db::Summary summary;
    wassert(actual(summary.data_count()) == 0u);

    summary.add(station, vd, dtrange, 12);
    wassert(actual(summary.data_count()) == 12u);

    summary.add(station, vd, dtrange, 12);
    wassert(actual(summary.data_count()) == 24u);

    wassert(actual(summary.stations().size()) == 1);
    wassert(actual(summary.stations().begin()->size()) == 1);

    summary.add(station, vd, dtrange, 12);
    station.report = "test1";
    summary.add(station, vd, dtrange, 12);
    summary.add(station, vd, dtrange, 12);
    wassert(actual(summary.stations().size()) == 2);
    wassert(actual(summary.stations().begin()->size()) == 1);
    wassert(actual(summary.stations().rbegin()->size()) == 1);
    wassert(actual(summary.data_count()) == 36u + 24u);
});

this->add_method("json_entry", [](Fixture& f) {
#if 0
    summary::Entry entry;
    entry.station.report = "test";
    entry.station.coords = Coords(44.5, 11.5);
    entry.level = Level(1);
    entry.trange = Trange::instant();
    entry.varcode = WR_VAR(0, 1, 112);
    entry.dtrange.set(Datetime(2018, 1, 1), Datetime(2018, 7, 1));
    entry.count = 12;

    std::stringstream json;
    core::JSONWriter writer(json);
    entry.to_json(writer);

    wassert(actual(json.str()) == R"({"s":{"r":"test","c":[4450000,1150000],"i":null},"l":[1,null,null,null],"t":[254,0,0],"v":368,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12})");

    json.seekg(0);
    core::json::Stream in(json);
    summary::Entry entry1 = summary::Entry::from_json(in);

    wassert(actual(entry1) == entry);
#endif
});

this->add_method("json_summary", [](Fixture& f) {
    Station station;
    station.report = "test";
    station.coords = Coords(44.5, 11.5);
    summary::VarDesc vd(Level(1), Trange::instant(), WR_VAR(0, 1, 112));
    DatetimeRange dtrange(Datetime(2018, 1, 1), Datetime(2018, 7, 1));

    core::Query query;
    query.rep_memo = "synop";
    Summary summary;
    summary.add(station, vd, dtrange, 12);
    vd.varcode = WR_VAR(0, 1, 113);
    summary.add(station, vd, dtrange, 12);

    std::stringstream json;
    core::JSONWriter writer(json);
    summary.to_json(writer);

    wassert(actual(json.str()) == R"({"e":[{"s":{"r":"test","c":[4450000,1150000],"i":null},"v":[{"l":[1,null,null,null],"t":[254,0,0],"v":368,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12},{"l":[1,null,null,null],"t":[254,0,0],"v":369,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12}]}]})");

    json.seekg(0);
    core::json::Stream in(json);
    Summary summary1 = wcallchecked(Summary::from_json(in));
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
