#include "matcher.h"
#include "tests.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace dballe;
using namespace dballe::core;
using namespace dballe::tests;

namespace {

#if 0
std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(*dballe::tests::query_from_string(q));
}
#endif

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_matcher");

void Tests::register_tests()
{
    add_method("empty", []() noexcept {});
#if 0
// Test station_id matcher
add_method("station_id", []() {
    auto m = get_matcher("ana_id=1");

    core::Record matched;
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

    matched.station.id = 2;
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

    matched.station.id = 1;
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
});

// Test station WMO matcher
add_method("station_wmo", []() {
    {
        auto m = get_matcher("block=11");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 1)).set(1);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 1)).set(11);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

        matched.obtain(WR_VAR(0, 1, 2)).set(222);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }

    {
        auto m = get_matcher("block=11, station=222");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 1)).set(1);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 1)).set(11);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 2)).set(22);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.obtain(WR_VAR(0, 1, 2)).set(222);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

        matched.obtain(WR_VAR(0, 1, 1)).set(1);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.unset_var(WR_VAR(0, 1, 1));
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
    }
});

// Test date matcher
add_method("date", []() {
    {
        auto m = get_matcher("yearmin=2000");

        core::Record matched;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 1999;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 2000;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmax=2000");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 2001;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 2000;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("yearmin=2000, yearmax=2010");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 1999;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 2011;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.datetime.min.year = matched.datetime.max.year = 2000;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

        matched.datetime.min.year = matched.datetime.max.year = 2005;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

        matched.datetime.min.year = matched.datetime.max.year = 2010;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
});

// Test coordinates matcher
add_method("coords", []() {
    {
        auto m = get_matcher("latmin=45.00");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.set_lat(43.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.set_lat(45.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        matched.station.coords.set_lat(46.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmax=45.00");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.lat = 4600000;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.lat = 4500000;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        matched.station.coords.lat = 4400000;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=45.00, lonmax=180.0");

        core::Record matched;
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.station.coords.set_lon(43.0);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.set_lon(45.0);
        wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("lonmin=-180, lonmax=45.0");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.set_lon(46.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
        matched.station.coords.set_lon(45.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        matched.station.coords.set_lon(44.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
    {
        auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

        core::Record matched;
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.station.coords.set_lat(45.5);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.station.coords.set_lon(13.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

        matched.station.coords.set_lon(11.0);
        wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
    }
});

// Test rep_memo matcher
add_method("rep_memo", []() {
    auto m = get_matcher("rep_memo=synop");

    core::Record matched;
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

    matched.station.report = "temp";
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

    matched.station.report = "synop";
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
});

// Test empty matcher
add_method("empty", []() {
    auto query = dballe::Query::create();
    std::unique_ptr<Matcher> m = Matcher::create(*query);

    core::Record matched;
    wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
});
#endif
}

} // namespace
