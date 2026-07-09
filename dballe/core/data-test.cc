#include "data.h"
#include "tests.h"

using namespace dballe::tests;
using namespace dballe;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_data");

void Tests::register_tests()
{

#if 0
add_method("comparisons", []() {
    core::Record rec;
    rec.station.id = -10;
    rec.station.coords.set(12.34567, 76.54321);
    rec.datetime.min.year = 1976;
    rec.obtain(WR_VAR(0, 20, 1)).set("456");
    rec.obtain(WR_VAR(0, 20, 3)).set("456");

    core::Record rec1;
    rec1 = rec;
    wassert(actual(rec == rec1).istrue());
    wassert(actual(rec1 == rec).istrue());
    wassert(actual(!(rec != rec1)).istrue());
    wassert(actual(!(rec1 != rec)).istrue());
    rec1.datetime.min.year = 1975;
    wassert(actual(rec != rec1).istrue());
    wassert(actual(rec1 != rec).istrue());
    wassert(actual(!(rec == rec1)).istrue());
    wassert(actual(!(rec1 == rec)).istrue());

    rec1 = rec;
    wassert(actual(rec == rec1).istrue());
    wassert(actual(rec1 == rec).istrue());
    rec1.datetime = DatetimeRange();
    wassert(actual(rec != rec1).istrue());
    wassert(actual(rec1 != rec).istrue());

    rec1 = rec;
    wassert(actual(rec == rec1).istrue());
    wassert(actual(rec1 == rec).istrue());
    rec1.obtain(WR_VAR(0, 20, 1)).set("45");
    wassert(actual(rec != rec1).istrue());
    wassert(actual(rec1 != rec).istrue());

    rec1 = rec;
    wassert(actual(rec == rec1).istrue());
    wassert(actual(rec1 == rec).istrue());
    rec1.unset_var(WR_VAR(0, 20, 1));
    wassert(actual(rec != rec1).istrue());
    wassert(actual(rec1 != rec).istrue());
});

add_method("extremes", []() {
    // Test querying extremes by Datetime
    core::Record rec;

    DatetimeRange dtr = wcallchecked(rec.get_datetimerange());
    wassert(actual(dtr.min.is_missing()).istrue());
    wassert(actual(dtr.max.is_missing()).istrue());

    static const int NA = MISSING_INT;

    wassert(rec.set(DatetimeRange(2010, 1, 1, 0, 0, 0, NA, NA, NA, NA, NA, NA)));
    dtr = wcallchecked(rec.get_datetimerange());
    wassert(actual(dtr.min) == Datetime(2010, 1, 1, 0, 0, 0));
    wassert(actual(dtr.max.is_missing()).istrue());

    wassert(rec.set(DatetimeRange(NA, NA, NA, NA, NA, NA, 2011, 2, 3, 4, 5, 6)));
    dtr = wcallchecked(rec.get_datetimerange());
    wassert(actual(dtr.min.is_missing()).istrue());
    wassert(actual(dtr.max) == Datetime(2011, 2, 3, 4, 5, 6));

    wassert(rec.set(DatetimeRange(2010, 1, 1, 0, 0, 0, 2011, 2, 3, 4, 5, 6)));
    dtr = wcallchecked(rec.get_datetimerange());
    wassert(actual(dtr.min) == Datetime(2010, 1, 1, 0, 0, 0));
    wassert(actual(dtr.max) == Datetime(2011, 2, 3, 4, 5, 6));
});
#endif
}

} // namespace
