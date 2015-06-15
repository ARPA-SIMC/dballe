#include <dballe/core/var.h>
#include "memdb/tests.h"
#include "stationvalue.h"
#include "station.h"

using namespace dballe;
using namespace wibble::tests;
using namespace std;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("basic", [](Fixture& f) {
        memdb::Stations stations;
        const memdb::Station& stf = *stations[stations.obtain_fixed(Coords(44.0, 11.0), "synop")];

        // Insert a station value and check that all data is there
        memdb::StationValues svalues;
        const memdb::StationValue& sv = *svalues[svalues.insert(stf, newvar(WR_VAR(0, 12, 101), 28.5))];
        wassert(actual(&sv.station) == &stf);
        wassert(actual(sv.var->code()) == WR_VAR(0, 12, 101));
        wassert(actual(sv.var->enqd()) == 28.5);

        // Replacing a value should reuse an existing one
        const memdb::StationValue& sv1 = *svalues[svalues.insert(stf, newvar(WR_VAR(0, 12, 101), 29.5))];
        wassert(actual(&sv1) == &sv);
        wassert(actual(&sv1.station) == &stf);
        wassert(actual(sv1.var->code()) == WR_VAR(0, 12, 101));
        wassert(actual(sv1.var->enqd()) == 29.5);
    }),
};

test_group newtg("memdb_stationvalue", tests);

}
