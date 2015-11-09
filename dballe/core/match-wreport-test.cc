#include "tests.h"
#include "match-wreport.h"
#include "var.h"
#include <wreport/vartable.h>
#include <wreport/subset.h>
#include <wreport/bulletin.h>
#include <wreport/tableinfo.h>
#include <wreport/internals/tabledir.h>

#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace dballe::tests;
using namespace wreport;

namespace {

std::unique_ptr<Matcher> get_matcher(const char* q)
{
    return Matcher::create(*dballe::tests::query_from_string(q));
}

struct Fixture : public dballe::tests::Fixture
{
    Tables tables;

    Fixture()
    {
        tables.load_bufr(BufrTableID(0, 0, 0, 24, 0));
    }
};

class TestSubset : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        // Test station_id matcher
        add_method("station_id", [](Fixture& f) {
            auto m = get_matcher("ana_id=1");

            Tables tables;
            tables.load_bufr(BufrTableID(200, 0, 0, 14, 1));

            Subset s(tables);
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

            s.store_variable_i(WR_VAR(0, 1, 1), 1);
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

            s.store_variable_i(WR_VAR(0, 1, 192), 1);
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
        });

        // Test station WMO matcher
        add_method("station_wmo", [](Fixture& f) {
            {
                auto m = get_matcher("block=11");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 1, 1), 1);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.back().seti(11);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);

                s.store_variable_i(WR_VAR(0, 1, 2), 222);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }

            {
                auto m = get_matcher("block=11, station=222");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 1, 1), 1);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.back().seti(11);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 1, 2), 22);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.back().seti(222);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);

                s[0].seti(1);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0] = var(WR_VAR(0, 1, 192));
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);
            }
        });

        // Test date matcher
        add_method("date", [](Fixture& f) {
            {
                auto m = get_matcher("yearmin=2000");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 4, 1), 1999);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmax=2000");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 4, 1), 2001);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmin=2000, yearmax=2010");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_i(WR_VAR(0, 4, 1), 1999);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].seti(2011);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);

                s[0].seti(2005);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);

                s[0].seti(2010);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
        });

        // Test coordinates matcher
        add_method("coords", [](Fixture& f) {
            {
                auto m = get_matcher("latmin=45.00");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 5, 1), 43.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].setd(45.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
                s[0].setd(46.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmax=45.00");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 5, 1), 46.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].setd(45.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
                s[0].setd(44.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=11.00, lonmax=180.0");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 6, 1), 10.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
                s[0].setd(12.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=-180, lonmax=11.0");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 6, 1), 12.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[0].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
                s[0].setd(10.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

                Subset s(f.tables);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 5, 1), 45.5);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s.store_variable_d(WR_VAR(0, 6, 1), 13.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

                s[1].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
            }
        });

        // Test rep_memo matcher
        add_method("rep_memo", [](Fixture& f) {
            auto m = get_matcher("rep_memo=synop");

            Tables tables;
            tables.load_bufr(BufrTableID(200, 0, 0, 14, 1));

            Subset s(tables);
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

            wassert(s.store_variable_c(WR_VAR(0, 1, 194), "temp"));
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_NO);

            s[0].setc("synop");
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
        });

        // Test empty matcher
        add_method("empty", [](Fixture& f) {
            std::unique_ptr<Matcher> m = Matcher::create(*Query::create());

            Subset s(f.tables);
            wassert(actual_matcher_result(m->match(MatchedSubset(s))) == matcher::MATCH_YES);
        });
    }
} test1("core_match_wreport_subset");


struct FixtureBulletin : public dballe::tests::Fixture
{
    BufrBulletin* bulletin = nullptr;

    ~FixtureBulletin() { delete bulletin; }

    BufrBulletin& get_bulletin()
    {
        delete bulletin;
        bulletin = BufrBulletin::create().release();
        BufrBulletin& b = *bulletin;
        b.edition_number = 4;
        b.originating_centre = 200;
        b.originating_subcentre = 0;
        b.master_table_version_number = 14;
        b.master_table_version_number_local = 0;
        b.load_tables();
        return b;
    }
};

class TestBulletin : public FixtureTestCase<FixtureBulletin>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        // Test station_id matcher
        add_method("station_id", [](Fixture& f) {
            auto m = get_matcher("ana_id=1");

            BufrBulletin& b = f.get_bulletin();
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

            b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

            b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 192), 1);
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
        });

        // Test station WMO matcher
        add_method("station_wmo", [](Fixture& f) {
            {
                auto m = get_matcher("block=11");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 1), 1);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1).back().seti(11);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);

                b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 2), 222);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }

            {
                auto m = get_matcher("block=11, station=222");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 1), 1);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).back().seti(11);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 2), 22);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).back().seti(222);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);

                b.obtain_subset(0)[0].seti(1);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0] = var(WR_VAR(0, 1, 192));
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);
            }

            // Valid block and station must be in the same subset
            {
                auto m = get_matcher("block=11, station=222");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 1), 11);
                b.obtain_subset(1).store_variable_i(WR_VAR(0, 1, 2), 222);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 1, 2), 222);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
        });

        // Test date matcher
        add_method("date", [](Fixture& f) {
            {
                auto m = get_matcher("yearmin=2000");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 1999);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmax=2000");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 2001);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("yearmin=2000, yearmax=2010");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_i(WR_VAR(0, 4, 1), 1999);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].seti(2011);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].seti(2000);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);

                b.obtain_subset(0)[0].seti(2005);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);

                b.obtain_subset(0)[0].seti(2010);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
        });

        // Test coordinates matcher
        add_method("coords", [](Fixture& f) {
            {
                auto m = get_matcher("latmin=45.0");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 43.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1)[0].setd(45.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
                b.obtain_subset(1)[0].setd(46.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmax=45.0");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1).store_variable_d(WR_VAR(0, 5, 1), 46.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(1)[0].setd(45.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
                b.obtain_subset(1)[0].setd(44.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=11.00, lonmax=180.0");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 10.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
                b.obtain_subset(0)[0].setd(12.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("lonmin=-180, lonmax=11.0");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 12.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[0].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
                b.obtain_subset(0)[0].setd(10.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
            {
                auto m = get_matcher("latmin=45.0, latmax=46.0, lonmin=10.0, lonmax=12.0");

                BufrBulletin& b = f.get_bulletin();
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_d(WR_VAR(0, 5, 1), 45.5);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0).store_variable_d(WR_VAR(0, 6, 1), 13.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

                b.obtain_subset(0)[1].setd(11.0);
                wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
            }
        });

        // Test rep_memo matcher
        add_method("rep_memo", [](Fixture& f) {
            auto m = get_matcher("rep_memo=synop");

            BufrBulletin& b = f.get_bulletin();
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

            b.obtain_subset(0).store_variable_c(WR_VAR(0, 1, 194), "temp");
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_NO);

            b.obtain_subset(0)[0].setc("synop");
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
        });

        // Test empty matcher
        add_method("empty", [](Fixture& f) {
            std::unique_ptr<Matcher> m = Matcher::create(*Query::create());

            BufrBulletin& b = f.get_bulletin();
            wassert(actual_matcher_result(m->match(MatchedBulletin(b))) == matcher::MATCH_YES);
        });
    }
} test2("core_match_wreport_bulletin");

}
