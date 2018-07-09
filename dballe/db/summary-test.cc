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
    wassert(actual(s.is_valid()).isfalse());

    // Build the whole db summary
    core::Query query;
    query.query = "details";
    auto cur = f.tr->query_summary(query);
    while (cur->next())
        s.add_cursor(*cur);

    // Check its contents
    wassert(actual(s.is_valid()).istrue());
    switch (DB::format)
    {
        case V7:
        default:
            wassert(actual(s.all_stations.size()) == 2);
            break;
    }
    wassert(actual(s.all_levels.size()) == 1);
    wassert(actual(s.all_tranges.size()) == 2);
    wassert(actual(s.all_varcodes.size()) == 2);
    wassert(actual(s.datetime_min()) == Datetime(1945, 4, 25, 8));
    wassert(actual(s.datetime_max()) == Datetime(1945, 4, 25, 8, 30));
    wassert(actual(s.data_count()) == 4);
});

this->add_method("merge_entries", [](Fixture& f) {
    summary::Entry entry;
    entry.station.report = "test";
    entry.station.coords = Coords(44.5, 11.5);
    entry.level = Level(1);
    entry.trange = Trange::instant();
    entry.varcode = WR_VAR(0, 1, 112);
    entry.dtrange.set(Datetime(2018, 1, 1), Datetime(2018, 7, 1));
    entry.count = 12;

    db::Summary summary;
    wassert(summary.merge_entries());
    wassert(actual(summary.test_entries().size()) == 0);
    summary.add_entry(entry);
    wassert(summary.merge_entries());
    wassert(actual(summary.test_entries().size()) == 1);
    summary.add_entry(entry);
    wassert(actual(summary.test_entries().size()) == 2);
    wassert(summary.merge_entries());
    wassert(actual(summary.test_entries().size()) == 1);
    wassert(actual(summary.test_entries()[0].count) == 24u);

    summary.add_entry(entry);
    entry.station.report = "test1";
    summary.add_entry(entry);
    summary.add_entry(entry);
    wassert(actual(summary.test_entries().size()) == 4);
    wassert(summary.merge_entries());
    wassert(actual(summary.test_entries().size()) == 2);
    wassert(actual(summary.test_entries()[0].count) == 36u);
    wassert(actual(summary.test_entries()[1].count) == 24u);
});

this->add_method("json_entry", [](Fixture& f) {
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
});

this->add_method("json_summary", [](Fixture& f) {
    summary::Entry entry;
    entry.station.report = "test";
    entry.station.coords = Coords(44.5, 11.5);
    entry.level = Level(1);
    entry.trange = Trange::instant();
    entry.varcode = WR_VAR(0, 1, 112);
    entry.dtrange.set(Datetime(2018, 1, 1), Datetime(2018, 7, 1));
    entry.count = 12;

    core::Query query;
    query.rep_memo = "synop";
    Summary summary;
    summary.add_entry(entry);
    entry.varcode = WR_VAR(0, 1, 113);
    summary.add_entry(entry);

    std::stringstream json;
    core::JSONWriter writer(json);
    summary.to_json(writer);

    wassert(actual(json.str()) == R"({"e":[{"s":{"r":"test","c":[4450000,1150000],"i":null},"l":[1,null,null,null],"t":[254,0,0],"v":368,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12},{"s":{"r":"test","c":[4450000,1150000],"i":null},"l":[1,null,null,null],"t":[254,0,0],"v":369,"d":[[2018,1,1,0,0,0],[2018,7,1,0,0,0]],"c":12}]})");

    json.seekg(0);
    core::json::Stream in(json);
    Summary summary1 = wcallchecked(Summary::from_json(in));
    wassert_true(summary == summary1);
    wassert(actual(summary.all_stations.size()) == summary1.all_stations.size());
    wassert_true(summary.all_stations == summary1.all_stations);
    wassert_true(summary.all_reports == summary1.all_reports);
    wassert_true(summary.all_levels == summary1.all_levels);
    wassert_true(summary.all_tranges == summary1.all_tranges);
    wassert_true(summary.all_varcodes == summary1.all_varcodes);
    wassert_true(summary.dtrange == summary1.dtrange);
    wassert_true(summary.count == summary1.count);
    wassert_true(summary.valid == summary1.valid);
});

}

}
