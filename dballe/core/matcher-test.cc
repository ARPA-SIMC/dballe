#include "tests.h"
#include "matcher.h"
#include "record.h"

#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace dballe::core;
using namespace dballe::tests;

namespace {

std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(*dballe::tests::query_from_string(q));
}

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        // Test station_id matcher
        add_method("station_id", []() {
            auto m = get_matcher("ana_id=1");

            core::Record matched;
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

            matched.set("ana_id", 2);
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

            matched.set("ana_id", 1);
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        });

        // Test station WMO matcher
        add_method("station_wmo", []() {
            {
                auto m = get_matcher("block=11");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("block", 1);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("block", 11);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

                matched.set("station", 222);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }

            {
                auto m = get_matcher("block=11, station=222");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("block", 1);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("block", 11);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("station", 22);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("station", 222);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

                matched.set("block", 1);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.unset("block");
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);
            }
        });

        // Test date matcher
        add_method("date", []() {
            {
                auto m = get_matcher("yearmin=2000");

                core::Record matched;
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 1999);
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 2000);
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmax=2000");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 2001);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 2000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmin=2000, yearmax=2010");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 1999);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 2011);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("year", 2000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

                matched.set("year", 2005);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);

                matched.set("year", 2010);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
        });

        // Test coordinates matcher
        add_method("coords", []() {
            {
                auto m = get_matcher("latmin=45.00");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lat", 43.0);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lat", 45.0);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
                matched.set("lat", 46.0);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmax=45.00");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lat", 4600000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lat", 4500000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
                matched.set("lat", 4400000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=45.00, lonmax=180.0");

                core::Record matched;
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 4300000);
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 4500000);
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
                matched.set("lon", 4500000);
                wassert(actual(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=-180, lonmax=45.0");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 4600000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 4500000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
                matched.set("lon", 4400000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

                core::Record matched;
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lat", 4550000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 1300000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

                matched.set("lon", 1100000);
                wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
            }
        });

        // Test rep_memo matcher
        add_method("rep_memo", []() {
            auto m = get_matcher("rep_memo=synop");

            core::Record matched;
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

            matched.set("rep_memo", "temp");
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_NO);

            matched.set("rep_memo", "synop");
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        });

        // Test empty matcher
        add_method("empty", []() {
            auto query = dballe::Query::create();
            std::unique_ptr<Matcher> m = Matcher::create(*query);

            core::Record matched;
            wassert(actual_matcher_result(m->match(MatchedRecord(matched))) == matcher::MATCH_YES);
        });
    }
} test("core_matcher");

}
