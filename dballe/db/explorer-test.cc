#define _DBALLE_TEST_CODE
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

template<typename DB, typename EXPLORER>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB, Explorer> tg1("db_explorer_v7_sqlite_explorer", "SQLITE");
Tests<V7DB, DBExplorer> tg2("db_explorer_v7_sqlite_dbexplorer", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB, Explorer> tg3("db_explorer_v7_postgresql_explorer", "POSTGRESQL");
Tests<V7DB, DBExplorer> tg4("db_explorer_v7_postgresql_dbexplorer", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB, Explorer> tg5("db_explorer_v7_mysql_explorer", "MYSQL");
Tests<V7DB, DBExplorer> tg6("db_explorer_v7_mysql_dbexplorer", "MYSQL");
#endif

template<typename Station>
void test_explorer_contents(const BaseExplorer<Station>& explorer)
{
    vector<Station> stations;

    stations.clear();
    for (const auto& s: explorer.global_summary().stations())
        stations.push_back(s.station);
    wassert(actual(stations.size()) == 2);
    wassert(actual(stations[0].report) == "metar");
    wassert(actual(stations[1].report) == "synop");

    stations.clear();
    for (const auto& s: explorer.active_summary().stations())
        stations.push_back(s.station);
    wassert(actual(stations.size()) == 1);
    wassert(actual(stations[0].report) == "metar");

    vector<string> reports;

    reports.clear();
    for (const auto& v: explorer.global_summary().reports())
        reports.push_back(v);
    wassert(actual(reports.size()) == 2);
    wassert(actual(reports[0]) == "metar");
    wassert(actual(reports[1]) == "synop");

    reports.clear();
    for (const auto& v: explorer.active_summary().reports())
        reports.push_back(v);
    wassert(actual(reports.size()) == 1);
    wassert(actual(reports[0]) == "metar");

    vector<Level> levels;

    levels.clear();
    for (const auto& v: explorer.global_summary().levels())
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    levels.clear();
    for (const auto& v: explorer.active_summary().levels())
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    vector<Trange> tranges;

    tranges.clear();
    for (const auto& v: explorer.global_summary().tranges())
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 2);
    wassert(actual(tranges[0]) == Trange(20, 111, 122));
    wassert(actual(tranges[1]) == Trange(20, 111, 123));

    tranges.clear();
    for (const auto& v: explorer.active_summary().tranges())
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 1);
    wassert(actual(tranges[0]) == Trange(20, 111, 123));

    vector<wreport::Varcode> vars;

    vars.clear();
    for (const auto& v: explorer.global_summary().varcodes())
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));

    vars.clear();
    for (const auto& v: explorer.active_summary().varcodes())
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));
}

template<typename DB, typename EXPLORER>
void Tests<DB, EXPLORER>::register_tests()
{

this->add_method("filter_rep_memo", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    EXPLORER explorer;
    {
        auto update = explorer.rebuild();
        wassert(update.add_db(*f.tr));
    }

    core::Query query;
    query.set_from_test_string("rep_memo=metar");
    explorer.set_filter(query);

    wassert(test_explorer_contents(explorer));

    std::stringstream json;
    core::JSONWriter writer(json);
    explorer.to_json(writer);
    wassert(actual(json.str()).startswith("{\"summary\":{"));

    json.seekg(0);
    core::json::Stream in(json);
    EXPLORER explorer1;
    {
        auto update = explorer1.update();
        wassert(update.add_json(in));
    }

    explorer1.set_filter(query);

    wassert(test_explorer_contents(explorer1));
});

this->add_method("merge", [](Fixture& f) {
// TODO: test merge
//  - values are merged
//  - query is preserved
//  - filtered values are regenerated
});

}

}
