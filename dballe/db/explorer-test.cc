#include "db/tests.h"
#include "explorer.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public DBFixtureTestCase<DBFixture>
{
    using DBFixtureTestCase::DBFixtureTestCase;

    void register_tests() override;
};

Tests tg1("db_explorer_mem", nullptr, db::MEM);
Tests tg2("db_explorer_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_LIBPQ
Tests tg4("db_explorer_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg5("db_explorer_v6_mysql", "MYSQL", db::V6);
#endif
Tests tg6("db_explorer_v7_sqlite", "SQLITE", db::V7);
#ifdef HAVE_LIBPQ
Tests tg7("db_explorer_v7_postgresql", "POSTGRESQL", db::V7);
#endif
#ifdef HAVE_MYSQL
Tests tg8("db_explorer_v7_mysql", "MYSQL", db::V7);
#endif

void Tests::register_tests()
{

add_method("filter_rep_memo", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    wassert(f.populate<OldDballeTestDataSet>());

    Explorer explorer(*f.db);
    explorer.revalidate();

    core::Query query;
    query.set_from_test_string("rep_memo=metar");
    explorer.set_filter(query);

    vector<Station> stations;

    stations.clear();
    for (const auto& s: explorer.global_summary().all_stations)
        stations.push_back(s.second);
    if (f.db->format() == db::V6)
    {
        wassert(actual(stations.size()) == 1);
    } else {
        wassert(actual(stations.size()) == 2);
        wassert(actual(stations[0].report) == "metar");
        wassert(actual(stations[1].report) == "synop");
    }

    stations.clear();
    for (const auto& s: explorer.active_summary().all_stations)
        stations.push_back(s.second);
    wassert(actual(stations.size()) == 1);
    wassert(actual(stations[0].report) == "metar");

    vector<string> reports;

    reports.clear();
    for (const auto& v: explorer.global_summary().all_reports)
        reports.push_back(v);
    wassert(actual(reports.size()) == 2);
    wassert(actual(reports[0]) == "metar");
    wassert(actual(reports[1]) == "synop");

    reports.clear();
    for (const auto& v: explorer.active_summary().all_reports)
        reports.push_back(v);
    wassert(actual(reports.size()) == 1);
    wassert(actual(reports[0]) == "metar");

    vector<Level> levels;

    levels.clear();
    for (const auto& v: explorer.global_summary().all_levels)
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    levels.clear();
    for (const auto& v: explorer.active_summary().all_levels)
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    vector<Trange> tranges;

    tranges.clear();
    for (const auto& v: explorer.global_summary().all_tranges)
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 2);
    wassert(actual(tranges[0]) == Trange(20, 111, 122));
    wassert(actual(tranges[1]) == Trange(20, 111, 123));

    tranges.clear();
    for (const auto& v: explorer.active_summary().all_tranges)
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 1);
    wassert(actual(tranges[0]) == Trange(20, 111, 123));

    vector<wreport::Varcode> vars;

    vars.clear();
    for (const auto& v: explorer.global_summary().all_varcodes)
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));

    vars.clear();
    for (const auto& v: explorer.active_summary().all_varcodes)
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));
});

}

}
