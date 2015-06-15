#include "memdb/tests.h"
#include "dballe/core/var.h"
#include "value.h"
#include "station.h"
#include "levtr.h"

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

        memdb::LevTrs levtrs;
        const memdb::LevTr& levtr = *levtrs[levtrs.obtain(Level(1), Trange::instant())];

        Datetime datetime(2013, 10, 30, 23);

        // Insert a station value and check that all data is there
        memdb::Values values;
        const memdb::Value& v = *values[values.insert(stf, levtr, datetime, newvar(WR_VAR(0, 12, 101), 28.5))];
        wassert(actual(&v.station) == &stf);
        wassert(actual(&v.levtr) == &levtr);
        wassert(actual(v.datetime) == datetime);
        wassert(actual(v.var->code()) == WR_VAR(0, 12, 101));
        wassert(actual(v.var->enqd()) == 28.5);

        // Replacing a value should reuse an existing one
        const memdb::Value& v1 = *values[values.insert(stf, levtr, datetime, newvar(WR_VAR(0, 12, 101), 29.5))];
        wassert(actual(&v1.station) == &stf);
        wassert(actual(&v1.levtr) == &levtr);
        wassert(actual(v1.datetime) == datetime);
        wassert(actual(v1.var->code()) == WR_VAR(0, 12, 101));
        wassert(actual(v1.var->enqd()) == 29.5);
    }),
};

test_group newtg("memdb_value", tests);

}
