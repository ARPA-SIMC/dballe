#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "explorer.h"
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

Tests<V7DB> tg6("db_explorer_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg7("db_explorer_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg8("db_explorer_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests()
{

this->add_method("filter_rep_memo", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    Explorer explorer;
    explorer.revalidate(*f.tr);

    core::Query query;
    query.set_from_test_string("rep_memo=metar");
    explorer.set_filter(query);

    vector<Station> stations;

    stations.clear();
    for (const auto& s: explorer.global_summary().all_stations)
        stations.push_back(s.second);
    wassert(actual(stations.size()) == 2);
    wassert(actual(stations[0].report) == "metar");
    wassert(actual(stations[1].report) == "synop");

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
