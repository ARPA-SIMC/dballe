#include "dballe/core/tests.h"
#include "cache.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} tests("db_v7_cache");

void Tests::register_tests() {

add_method("cache", [] {
    db::v7::StationCache stations;

    wassert_false(stations.find_entry(1));
    wassert_false(stations.find_entry(MISSING_INT));
    wassert(actual(stations.find_id(Station())) == MISSING_INT);

    Station st;
    st.id = 1;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    stations.insert(st);

    wassert_true(stations.find_entry(1));
    wassert(actual(*stations.find_entry(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    stations.insert(st);

    wassert_true(stations.find_entry(1));
    wassert(actual(*stations.find_entry(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    wassert(actual(stations.reverse[st.coords.lon].size()) == 1u);
});

add_method("levtr", [] {
    db::v7::LevTrCache cache;

    wassert_false(cache.find_entry(1));
    wassert_false(cache.find_entry(MISSING_INT));
    wassert(actual(cache.find_id(db::v7::LevTrEntry())) == MISSING_INT);

    db::v7::LevTrEntry lt;
    lt.id = 1;
    lt.level = Level(1);
    lt.trange = Trange(4, 2, 2);

    cache.insert(lt);

    wassert_true(cache.find_entry(1));
    wassert(actual(*cache.find_entry(1)) == lt);
    wassert(actual(cache.find_id(lt)) == 1);

    cache.insert(lt);

    wassert_true(cache.find_entry(1));
    wassert(actual(*cache.find_entry(1)) == lt);
    wassert(actual(cache.find_id(lt)) == 1);

    wassert(actual(cache.reverse[lt.level].size()) == 1u);
});

}

}
