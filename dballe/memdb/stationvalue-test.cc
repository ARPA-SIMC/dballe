#include "dballe/var.h"
#include "memdb/tests.h"
#include "stationvalue.h"
#include "station.h"

using namespace dballe;
using namespace dballe::tests;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
        add_method("basic", []() {
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
        });
    }
} test("memdb_stationvalue");

}
