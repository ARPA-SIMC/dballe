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

add_method("populate", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    wassert(f.populate<OldDballeTestDataSet>());

    Explorer explorer(*f.db);
    explorer.revalidate();

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
    wassert(actual(stations.size()) == 2);
    wassert(actual(stations[0].report) == "metar");
    wassert(actual(stations[1].report) == "synop");
});

}

}
