#include "memdb/tests.h"
#include "memdb.h"
#include "serializer.h"
#include "core/defs.h"
#include "dballe/var.h"
#include <wreport/utils/sys.h>

using namespace dballe;
using namespace dballe::memdb;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : public dballe::tests::Fixture
{
    string testdir = "serializer_test_dir";

    void test_setup() override
    {
        if (sys::isdir(testdir))
            sys::rmtree(testdir);
    }

};

struct Tests : public FixtureTestCase<Fixture>
{
    using FixtureTestCase::FixtureTestCase;


    void register_tests() override
    {
        // Test a simple serialize/deserialize round
        add_method("simple", [](Fixture& f) {
            Var var_st_1(varinfo(WR_VAR(0, 7, 7)), 100.0);
            var_st_1.seta(newvar(WR_VAR(0, 33, 7), 30));
            Var var_st_2(varinfo(WR_VAR(0, 7, 7)), 5000.0);
            var_st_2.seta(newvar(WR_VAR(0, 33, 7), 40));
            Var var_1(varinfo(WR_VAR(0, 12, 101)), 274.0);
            var_1.seta(newvar(WR_VAR(0, 33, 7), 50));
            Var var_2(varinfo(WR_VAR(0, 12, 101)), 273.0);
            var_2.seta(newvar(WR_VAR(0, 33, 7), 60));

            // Create a memdb and write it out
            {
                Memdb memdb;

                memdb.insert(Coords(45, 11), string(), "synop", Level(1), Trange::instant(), Datetime(2013, 12, 15), var_1);
                memdb.stationvalues.insert(*memdb.stations[0], var_st_1);
                memdb.insert(Coords(45, 12), "LH1234", "airep", Level(1), Trange::instant(), Datetime(2013, 12, 16), var_2);
                memdb.stationvalues.insert(*memdb.stations[1], var_st_2);

                serialize::CSVWriter serializer(f.testdir);
                serializer.write(memdb);
                serializer.commit();
            }

            // Read it back
            {
                Memdb memdb;
                serialize::CSVReader reader(f.testdir, memdb);
                reader.read();
                wassert(actual(memdb.stations.element_count()) == 2);
                wassert(actual(memdb.stations[0]->coords) == Coords(45, 11));
                wassert(actual(memdb.stations[0]->mobile).isfalse());
                wassert(actual(memdb.stations[0]->ident) == "");
                wassert(actual(memdb.stations[0]->report) == "synop");
                wassert(actual(memdb.stations[1]->coords) == Coords(45, 12));
                wassert(actual(memdb.stations[1]->mobile).istrue());
                wassert(actual(memdb.stations[1]->ident) == "LH1234");
                wassert(actual(memdb.stations[1]->report) == "airep");

                wassert(actual(memdb.stationvalues.element_count()) == 2);
                wassert(actual(memdb.stationvalues[0]->station.id) == memdb.stations[0]->id);
                wassert(actual(*(memdb.stationvalues[0]->var)) == var_st_1);
                wassert(actual(memdb.stationvalues[1]->station.id) == memdb.stations[1]->id);
                wassert(actual(*(memdb.stationvalues[1]->var)) == var_st_2);

                wassert(actual(memdb.levtrs.element_count()) == 1);
                wassert(actual(memdb.levtrs[0]->level) == Level(1));
                wassert(actual(memdb.levtrs[0]->trange) == Trange::instant());

                wassert(actual(memdb.values.element_count()) == 2);
                wassert(actual(memdb.values[0]->station.id) == memdb.stations[0]->id);
                wassert(actual(memdb.values[0]->levtr.level) == Level(1));
                wassert(actual(memdb.values[0]->levtr.trange) == Trange::instant());
                wassert(actual(memdb.values[0]->datetime) == Datetime(2013, 12, 15));
                wassert(actual(*(memdb.values[0]->var)) == var_1);
                wassert(actual(memdb.values[1]->station.id) == memdb.stations[1]->id);
                wassert(actual(memdb.values[1]->levtr.level) == Level(1));
                wassert(actual(memdb.values[1]->levtr.trange) == Trange::instant());
                wassert(actual(memdb.values[1]->datetime) == Datetime(2013, 12, 16));
                wassert(actual(*(memdb.values[1]->var)) == var_2);
            }
        });

        // Test deserializing a nonexisting data dir
        add_method("missing_dir", [](Fixture& f) {
            Memdb memdb;
            serialize::CSVReader reader(f.testdir, memdb);
            reader.read();
            wassert(actual(memdb.stations.element_count()) == 0);
            wassert(actual(memdb.stationvalues.element_count()) == 0);
            wassert(actual(memdb.levtrs.element_count()) == 0);
            wassert(actual(memdb.values.element_count()) == 0);
        });

        // Test nasty chars in values
        add_method("nasty_chars", [](Fixture& f) {
            const char* str_ident = "\"'\n,";
            const char* str_report = "\n\"',";
            Var var_st_1(varinfo(WR_VAR(0, 1, 19)), "'\"\n,");
            var_st_1.seta(newvar(WR_VAR(0, 33, 7), 30));
            Var var_1(varinfo(WR_VAR(0, 12, 101)), 274.0);
            var_1.seta(newvar(WR_VAR(0, 1, 212), "'\"\n,"));

            {
                Memdb memdb;
                memdb.insert(Coords(45, 11), str_ident, str_report, Level(1), Trange::instant(), Datetime(2013, 12, 15), var_1);
                memdb.stationvalues.insert(*memdb.stations[0], var_st_1);

                serialize::CSVWriter serializer(f.testdir);
                serializer.write(memdb);
                serializer.commit();
            }

            {
                Memdb memdb;
                serialize::CSVReader reader(f.testdir, memdb);
                reader.read();
                wassert(actual(memdb.stations.element_count()) == 1);
                wassert(actual(memdb.stations[0]->coords) == Coords(45, 11));
                wassert(actual(memdb.stations[0]->mobile).istrue());
                wassert(actual(memdb.stations[0]->ident) == str_ident);
                wassert(actual(memdb.stations[0]->report) == str_report);

                wassert(actual(memdb.stationvalues.element_count()) == 1);
                wassert(actual(memdb.stationvalues[0]->station.id) == memdb.stations[0]->id);
                wassert(actual(*(memdb.stationvalues[0]->var)) == var_st_1);

                wassert(actual(memdb.levtrs.element_count()) == 1);
                wassert(actual(memdb.levtrs[0]->level) == Level(1));
                wassert(actual(memdb.levtrs[0]->trange) == Trange::instant());

                wassert(actual(memdb.values.element_count()) == 1);
                wassert(actual(memdb.values[0]->station.id) == memdb.stations[0]->id);
                wassert(actual(memdb.values[0]->levtr.level) == Level(1));
                wassert(actual(memdb.values[0]->levtr.trange) == Trange::instant());
                wassert(actual(memdb.values[0]->datetime) == Datetime(2013, 12, 15));
                wassert(actual(*(memdb.values[0]->var)) == var_1);
            }
        });
    }
} test("memdb_serialize");

}
