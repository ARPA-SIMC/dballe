#include "tests.h"
#include "values.h"
#include "record.h"
#include <cstring>

using namespace std;
using namespace dballe::tests;
using namespace dballe;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_values");

void Tests::register_tests()
{

add_method("codec", []() {
    Values vals;
    // Integer variable
    vals.set(newvar(WR_VAR(0, 1, 2), 123));
    // Floating point variable
    vals.set(newvar(WR_VAR(0, 12, 101), 280.23));
    // Text variable
    vals.set(newvar(WR_VAR(0, 1, 19), "Test string value"));

    vector<uint8_t> encoded = vals.encode();
    wassert(actual(encoded.size()) == (14 + strlen("Test string value") + 1));

    Values vals1;
    Values::decode(encoded, [&](std::unique_ptr<wreport::Var> var) { vals1.set(move(var)); });

    wassert(actual(*vals1[WR_VAR(0, 1, 2)].var) == *vals[WR_VAR(0, 1, 2)].var);
    wassert(actual(*vals1[WR_VAR(0, 12, 101)].var) == *vals[WR_VAR(0, 12, 101)].var);
    wassert(actual(*vals1[WR_VAR(0, 1, 19)].var) == *vals[WR_VAR(0, 1, 19)].var);
});

add_method("values", []() {
    // Set station by ana_id
    {
        core::Record rec;
        rec.station.id = 1;
        rec.set(Level(1));
        rec.set(Trange::instant());
        rec.set(Datetime(2018, 7, 1));
        DataValues vals;
        vals.set_from_record(rec);
    }

    // Set station by station data
    {
        core::Record rec;
        rec.station.report = "test";
        rec.set(Coords(44.5, 11.5));
        rec.set(Level(1));
        rec.set(Trange::instant());
        rec.set(Datetime(2018, 7, 1));
        DataValues vals;
        vals.set_from_record(rec);
    }
});

}

}
