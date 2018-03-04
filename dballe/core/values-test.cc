#include "core/tests.h"
#include "core/values.h"
#include "core/record.h"
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

add_method("station", []() {
    Station st;
    st.ana_id = 1;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    core::Record rec;
    st.to_record(rec);

    wassert_true(rec.get("ana_id"));
    wassert(actual(*rec.get("ana_id")) == 1);
    wassert_true(rec.get("rep_memo"));
    wassert(actual(*rec.get("rep_memo")) == "testreport");
    wassert_true(rec.get("lat"));
    wassert(actual(*rec.get("lat")) == 1150000);
    wassert_true(rec.get("lon"));
    wassert(actual(*rec.get("lon")) == 4250000);
    wassert_true(rec.get("mobile"));
    wassert(actual(*rec.get("mobile")) == 1);
    wassert_true(rec.get("ident"));
    wassert(actual(*rec.get("ident")) == "testident");

    Station st1(rec);
    wassert(actual(st) == st1);

    st = Station();
    st.report = "testreport1";
    st.coords = Coords(11.6, 42.6);
    st.to_record(rec);

    wassert_false(rec.get("ana_id"));
    wassert_true(rec.get("rep_memo"));
    wassert(actual(*rec.get("rep_memo")) == "testreport1");
    wassert_true(rec.get("lat"));
    wassert(actual(*rec.get("lat")) == 1160000);
    wassert_true(rec.get("lon"));
    wassert(actual(*rec.get("lon")) == 4260000);
    wassert_true(rec.get("mobile"));
    wassert(actual(*rec.get("mobile")) == 0);
    wassert_false(rec.get("ident"));
});

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

}

}
