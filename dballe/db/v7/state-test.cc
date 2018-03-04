#include "core/tests.h"
#include "state.h"
#include <cstring>

using namespace std;
using namespace dballe::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("db_v7_state");

void Tests::register_tests()
{

add_method("stations", []() {
    db::v7::Stations stations;

    wassert_false(stations.find_station(1));
    wassert_false(stations.find_station(MISSING_INT));
    wassert(actual(stations.find_id(Station())) == MISSING_INT);

    Station st;
    st.ana_id = 1;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    stations.insert(st);

    wassert_true(stations.find_station(1));
    wassert(actual(*stations.find_station(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    stations.insert(st);

    wassert_true(stations.find_station(1));
    wassert(actual(*stations.find_station(1)) == st);
    wassert(actual(stations.find_id(st)) == 1);

    wassert(actual(stations.by_lon[st.coords.lon].size()) == 1u);
});

}

}
