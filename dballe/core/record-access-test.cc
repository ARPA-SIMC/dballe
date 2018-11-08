#include "tests.h"
#include "record.h"
#include "record-access.h"

using namespace dballe::tests;
using namespace dballe;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_record_access");

void Tests::register_tests()
{

add_method("get_set", []() {
    using namespace dballe::core;

    // Check that things don't exist at the beginning
    core::Record rec;
    wassert_false(record_isset(rec, "ana_id"));
    wassert_false(record_isset(rec, "lat"));
    wassert_false(record_isset(rec, "lon"));
    wassert_false(record_isset(rec, "yearmin"));
    wassert_false(record_isset(rec, "monthmin"));
    wassert_false(record_isset(rec, "B20001"));
    wassert_false(record_isset(rec, "B20003"));

    // Set various things
    record_seti(rec, "ana_id", -10);
    record_seti(rec, "lat", 1234567);
    record_setd(rec, "lon", 76.54321);
    record_setc(rec, "yearmin", "1976");
    record_setc(rec, "B20001", "456");
    record_setc(rec, "B20003", "456");

    // Check that they now exist
    wassert_true(record_isset(rec, "ana_id"));
    wassert_true(record_isset(rec, "lat"));
    wassert_true(record_isset(rec, "lon"));
    wassert_true(record_isset(rec, "yearmin"));
    wassert_true(record_isset(rec, "B20001"));
    wassert_true(record_isset(rec, "B20003"));

    // Check that they have the right value
    wassert(actual(record_enqi(rec, "ana_id", MISSING_INT)) == -10);
    wassert(actual(record_enqd(rec, "ana_id", 0)) == -10.0);
    wassert(actual(record_enqi(rec, "lon", MISSING_INT)) == 7654321);
    wassert(actual(record_enqd(rec, "lon", 0)) == 76.54321);
    wassert(actual(record_enqs(rec, "lon", "")) == "7654321");
    wassert(actual(record_enqd(rec, "lat", 0)) == 12.34567);
    wassert(actual(record_enqd(rec, "yearmin", 0)) == 1976.0);
    wassert(actual(record_enqd(rec, "B20001", 0)) == 4560.0);
    wassert(actual(record_enqd(rec, "B20003", 0)) == 456);

    // See if unset works for keywords
    record_unset(rec, "lat");
    wassert_false(record_isset(rec, "lat"));

    // See if unset works for variables
    record_unset(rec, "B20001");
    wassert_false(record_isset(rec, "B20001"));

    /* fprintf(stderr, "IVAL: %d\n", ival); */
    /* fprintf(stderr, "DVAL: %f\n", fval); */
    /*
    {
        int i = 7654321;
        double f = (double)i / 100000;
        fprintf(stderr, "I: %d, F: %f\n", i, f);
    }
    */

    // See if clear clears
    rec.clear();
    wassert_false(record_isset(rec, "lat"));
    wassert_false(record_isset(rec, "yearmin"));
    wassert_false(record_isset(rec, "B20003"));

    rec.clear();
    wassert_false(record_isset(rec, "lat"));
    wassert_false(record_isset(rec, "yearmin"));
    wassert_false(record_isset(rec, "B20003"));
});

add_method("ident", []() {
    // This used to cause a segfault
    core::Record rec;
    record_setc(rec, "ident", "nosort");
    rec = rec;
    record_setc(rec, "ident", "nosort");
});

add_method("level", []() {
    core::Record rec;
    wassert(actual(rec.get_level()) == Level());

    rec.level = Level(1, 0, 2, 3);
    wassert(actual(rec.get_level()) == Level(1, 0, 2, 3));

    rec.set(Level(9, 8));
    wassert(actual(record_enqi(rec, "leveltype1", 0)) == 9);
    wassert(actual(record_enqi(rec, "l1", 0)) == 8);
    wassert(actual(record_enqi(rec, "leveltype2", 0)) == 0);
    wassert(actual(record_enqi(rec, "l2", 0)) == 0);
    wassert(actual(rec.get_level()) == Level(9, 8));

    record_seti(rec, "leveltype1", 1);
    record_setf(rec, "l1", "-");
    wassert(actual(rec.get_level()) == Level(1));
});

add_method("trange", []() {
    core::Record rec;
    wassert(actual(rec.get_trange()) == Trange());

    rec.set(Trange(7, 6));
    wassert(actual(rec.get_trange()) == Trange(7, 6));
    wassert(actual(record_enqi(rec, "pindicator", 0)) == 7);
    wassert(actual(record_enqi(rec, "p1", 0)) == 6);
    wassert(actual(record_enqi(rec, "p2", 0)) == 0);

    record_seti(rec, "pindicator", 11);
    record_seti(rec, "p1", 22);
    record_seti(rec, "p2", 33);
    wassert(actual(rec.get_trange()) == Trange(11, 22, 33));
});

add_method("datetime", []() {
    core::Record rec;
    rec.set(Datetime(2013, 11, 1, 12, 0, 0));
    wassert(actual(rec.get_datetime()) == Datetime(2013, 11, 1, 12));

    rec.set(Datetime(2012, 5, 15, 17, 30, 30));
    Datetime dt = rec.get_datetime();
    wassert(actual(dt) == Datetime(2012, 5, 15, 17, 30, 30));

    rec.clear();
    record_seti(rec, "yearmin", 1976);
    wassert_true(rec.get_datetime().is_missing());
    wassert(actual(rec.get_datetimerange()) == DatetimeRange(1976, 1, 1, 0, 0, 0, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT, MISSING_INT));

    record_seti(rec, "yearmax", 1976);
    wassert(actual(rec.get_datetime()) == Datetime(1976, 1, 1, 0, 0, 0));
    wassert(actual(rec.get_datetimerange()) == DatetimeRange(1976, 1, 1, 0, 0, 0, 1976, 12, 31, 23, 59, 59));
});

add_method("station", []() {
    Station st;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    core::Record rec;
    rec.set(st);

    wassert_false(record_isset(rec, "ana_id"));
    wassert_true(record_isset(rec, "rep_memo"));
    wassert(actual(record_enqs(rec, "rep_memo", "")) == "testreport");
    wassert_true(record_isset(rec, "lat"));
    wassert(actual(record_enqi(rec, "lat", MISSING_INT)) == 1150000);
    wassert_true(record_isset(rec, "lon"));
    wassert(actual(record_enqi(rec, "lon", MISSING_INT)) == 4250000);
    wassert_true(record_isset(rec, "mobile"));
    wassert(actual(record_enqi(rec, "mobile", MISSING_INT)) == 1);
    wassert_true(record_isset(rec, "ident"));
    wassert(actual(record_enqs(rec, "ident", "")) == "testident");

    Station st1 = rec.get_station();
    wassert(actual(st) == st1);

    st = Station();
    st.report = "testreport1";
    st.coords = Coords(11.6, 42.6);
    rec.set(st);

    wassert_false(record_isset(rec, "ana_id"));
    wassert_true(record_isset(rec, "rep_memo"));
    wassert(actual(record_enqs(rec, "rep_memo", "")) == "testreport1");
    wassert_true(record_isset(rec, "lat"));
    wassert(actual(record_enqi(rec, "lat", MISSING_INT)) == 1160000);
    wassert_true(record_isset(rec, "lon"));
    wassert(actual(record_enqi(rec, "lon", MISSING_INT)) == 4260000);
    wassert_true(record_isset(rec, "mobile"));
    wassert(actual(record_enqi(rec, "mobile", MISSING_INT)) == 0);
    wassert_false(record_isset(rec, "ident"));
});

add_method("dbstation", []() {
    DBStation st;
    st.id = 1;
    st.report = "testreport";
    st.coords = Coords(11.5, 42.5);
    st.ident = "testident";

    core::Record rec;
    rec.set(st);

    wassert_true(record_isset(rec, "ana_id"));
    wassert(actual(record_enqi(rec, "ana_id", MISSING_INT)) == 1);
    wassert_true(record_isset(rec, "rep_memo"));
    wassert(actual(record_enqs(rec, "rep_memo", "")) == "testreport");
    wassert_true(record_isset(rec, "lat"));
    wassert(actual(record_enqi(rec, "lat", MISSING_INT)) == 1150000);
    wassert_true(record_isset(rec, "lon"));
    wassert(actual(record_enqi(rec, "lon", MISSING_INT)) == 4250000);
    wassert_true(record_isset(rec, "mobile"));
    wassert(actual(record_enqi(rec, "mobile", MISSING_INT)) == 1);
    wassert_true(record_isset(rec, "ident"));
    wassert(actual(record_enqs(rec, "ident", "")) == "testident");

    DBStation st1 = rec.get_dbstation();
    wassert(actual(st) == st1);

    st = DBStation();
    st.report = "testreport1";
    st.coords = Coords(11.6, 42.6);
    rec.set(st);

    wassert_false(record_isset(rec, "ana_id"));
    wassert_true(record_isset(rec, "rep_memo"));
    wassert(actual(record_enqs(rec, "rep_memo", "")) == "testreport1");
    wassert_true(record_isset(rec, "lat"));
    wassert(actual(record_enqi(rec, "lat", MISSING_INT)) == 1160000);
    wassert_true(record_isset(rec, "lon"));
    wassert(actual(record_enqi(rec, "lon", MISSING_INT)) == 4260000);
    wassert_true(record_isset(rec, "mobile"));
    wassert(actual(record_enqi(rec, "mobile", MISSING_INT)) == 0);
    wassert_false(record_isset(rec, "ident"));
});


}

}
