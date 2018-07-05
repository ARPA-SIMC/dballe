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

    core::Query query;
    query.query = "details";
    Summary s(query);
    wassert(actual(s.is_valid()).isfalse());

    // Build the whole db summary
    auto cur = f.tr->query_summary(query);
    while (cur->next())
        s.add_summary(*cur);

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

    // Check what it can support

    // An existing station is ok: we know we have it
    wassert(actual(s.supports(*query_from_string("ana_id=1"))) == summary::Support::EXACT);

    // A non-existing station is also ok: we know we don't have it
    wassert(actual(s.supports(*query_from_string("ana_id=2"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

    // Still exact, because the query matches the entire summary
    wassert(actual(s.supports(*query_from_string("yearmin=1945"))) == summary::Support::EXACT);

    // Still exact, because although the query partially matches the summary,
    // each summary entry is entier included completely or excluded completely
    wassert(actual(s.supports(*query_from_string("yearmin=1945, monthmin=4, daymin=25, hourmin=8, yearmax=1945, monthmax=4, daymax=25, hourmax=8, minumax=10"))) == summary::Support::EXACT);
});

this->add_method("json", [](Fixture& f) {
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

}

}
