#include "core/test-utils-core.h"
#include "types.h"

using namespace std;
using namespace wibble::tests;
using namespace dballe;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("date", [](Fixture& f) {
        // Constructor boundary checks
        wassert(actual(Date(2015, 0, 0)) == Date(2015, 1, 1));
        wassert(actual(Date(2015, 15, 100)) == Date(2015, 12, 31));
        wassert(actual(Date(2004, 2, 30)) == Date(2004, 2, 29));

        wassert(actual(Date(2013, 1, 1)) < Date(2014, 1, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 2, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 1, 2));
        wassert(actual(Date(1945, 4, 25)) != Date(1945, 4, 26));
    }),
    Test("time", [](Fixture& f) {
        // Constructor boundary checks
        wassert(actual(Time( 0, 0, 0)) == Time(0, 0, 0));
        wassert(actual(Time(24, 0, 0)) == Time(23, 0, 0));
        wassert(actual(Time(99, 99, 99)) == Time(23, 59, 59));

        wassert(actual(Time(13, 1,  1)) <  Time(14, 1,  1));
        wassert(actual(Time(13, 1,  1)) <  Time(13, 2,  1));
        wassert(actual(Time(13, 1,  1)) <  Time(13, 1,  2));
        wassert(actual(Time(19, 4, 25)) != Time(19, 4, 26));
    }),
    Test("datetime", [](Fixture& f) {
        // Constructor boundary checks
        wassert(actual(Datetime(2015,  0,  0,  0,  0,  0)) == Datetime(2015,  1,  1,  0,  0,  0));
        wassert(actual(Datetime(2015, 15, 99, 99, 99, 99)) == Datetime(2015, 12, 31, 23, 59, 59));
        wassert(actual(Datetime(2004,  2, 30,  0,  0,  0)) == Datetime(2004,  2, 29,  0,  0,  0));
        wassert(actual(Datetime(2015,  6, 10,  0,  0,  0)) == Datetime(2015,  6, 10,  0,  0,  0));
        wassert(actual(Datetime(2015,  6, 10, 24,  0,  0)) == Datetime(2015,  6, 10, 23,  0,  0));
        wassert(actual(Datetime(2015,  6, 10, 99, 99, 99)) == Datetime(2015,  6, 10, 23, 59, 59));

        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2014, 1, 1, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 2, 1, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 2, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 1, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 0, 1, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 0, 0, 1));
        wassert(actual(Datetime(1945, 4, 25, 8, 0, 0)) != Datetime(1945, 4, 26, 8, 0, 0));
    }),
    Test("datetime_jdays", [](Fixture& f) {
        // Test Date to/from julian days conversion
        Date d(2015, 4, 25);
        wassert(actual(d.to_julian()) == 2457138);

        d.from_julian(2457138);
        wassert(actual(d.year) == 2015);
        wassert(actual(d.month) == 4);
        wassert(actual(d.day) == 25);
    }),
    Test("datetimerange", [](Fixture& f) {
        Datetime missing;
        Datetime dt_2010(2010, 1, 1, 0, 0, 0);
        Datetime dt_2011(2011, 1, 1, 0, 0, 0);
        Datetime dt_2012(2012, 1, 1, 0, 0, 0);
        Datetime dt_2013(2013, 1, 1, 0, 0, 0);

        // Test equality
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(missing, missing)).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2011) == DatetimeRange(dt_2010, dt_2011)).istrue());
        wassert(actual(DatetimeRange(dt_2010, missing) == DatetimeRange(missing, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, dt_2010) == DatetimeRange(missing, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(dt_2010, missing)).isfalse());
        wassert(actual(DatetimeRange(missing, missing) == DatetimeRange(missing, dt_2010)).isfalse());
        wassert(actual(DatetimeRange(dt_2010, dt_2011) == DatetimeRange(dt_2012, dt_2013)).isfalse());

        // Test contains
        wassert(actual(DatetimeRange(missing, missing).contains(DatetimeRange(missing, missing))).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2011).contains(DatetimeRange(dt_2010, dt_2011))).istrue());
        wassert(actual(DatetimeRange(missing, missing).contains(DatetimeRange(dt_2011, dt_2012))).istrue());
        wassert(actual(DatetimeRange(dt_2011, dt_2012).contains(DatetimeRange(missing, missing))).isfalse());
        wassert(actual(DatetimeRange(dt_2010, dt_2013).contains(DatetimeRange(dt_2011, dt_2012))).istrue());
        wassert(actual(DatetimeRange(dt_2010, dt_2012).contains(DatetimeRange(dt_2011, dt_2013))).isfalse());
        wassert(actual(DatetimeRange(missing, dt_2010).contains(DatetimeRange(dt_2011, missing))).isfalse());
    }),
    Test("latrange", [](Fixture& f) {
        double dmin, dmax;
        LatRange lr;
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == LatRange::IMIN);
        wassert(actual(lr.imax) == LatRange::IMAX);
        wassert(actual(lr.dmin()) == LatRange::DMIN);
        wassert(actual(lr.dmax()) == LatRange::DMAX);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == LatRange::DMIN);
        wassert(actual(dmax) == LatRange::DMAX);
        wassert(actual(lr.contains(0)).istrue());

        lr = LatRange(40.0, 50.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 40.0);
        wassert(actual(dmax) == 50.0);
        wassert(actual(lr.contains(39.9)).isfalse());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).istrue());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).isfalse());
        wassert(actual(lr.contains(4500000)).istrue());
        wassert(actual(lr.contains(5500000)).isfalse());

        lr.set(-10.0, 10.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 1000000);
        wassert(actual(lr) == LatRange(-10.0, 10.0));

        lr.set(4000000, 5000000);
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        wassert(actual(lr) == LatRange(40.0, 50.0));
    }),
    Test("lonrange", [](Fixture& f) {
        double dmin, dmax;
        LonRange lr;
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);
        wassert(actual(lr.dmin()) == -180.0);
        wassert(actual(lr.dmax()) ==  180.0);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == -180.0);
        wassert(actual(dmax) == 180.0);
        wassert(actual(lr.contains(0)).istrue());

        lr = LonRange(-18000000, 18000000);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr = LonRange(-180.0, 180.0);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr.set(-18000000, 18000000);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr.set(-180.0, 180.0);
        wassert(actual(lr.is_missing()).istrue());
        wassert(actual(lr.imin) == MISSING_INT);
        wassert(actual(lr.imax) == MISSING_INT);

        lr = LonRange(40.0, 50.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 4000000);
        wassert(actual(lr.imax) == 5000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 40.0);
        wassert(actual(dmax) == 50.0);
        wassert(actual(lr.contains(39.9)).isfalse());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).istrue());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).isfalse());
        wassert(actual(lr.contains(4500000)).istrue());
        wassert(actual(lr.contains(5500000)).isfalse());

        lr = LonRange(50.0, 40.0);
        wassert(actual(lr.is_missing()).isfalse());
        wassert(actual(lr.imin) == 5000000);
        wassert(actual(lr.imax) == 4000000);
        lr.get(dmin, dmax);
        wassert(actual(dmin) == 50.0);
        wassert(actual(dmax) == 40.0);
        wassert(actual(lr.contains(39.9)).istrue());
        wassert(actual(lr.contains(40.0)).istrue());
        wassert(actual(lr.contains(45.0)).isfalse());
        wassert(actual(lr.contains(50.0)).istrue());
        wassert(actual(lr.contains(50.1)).istrue());
        wassert(actual(lr.contains(4500000)).isfalse());
        wassert(actual(lr.contains(5500000)).istrue());

        lr.set(-10.0, 10.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 1000000);
        wassert(actual(lr) == LonRange(-10.0, 10.0));

        lr.set(350.0, 360.0);
        wassert(actual(lr.imin) == -1000000);
        wassert(actual(lr.imax) == 0);
        wassert(actual(lr) == LonRange(-10.0, 0.0));
    }),
    Test("level_descs", [](Fixture& f) {
        // Try to get descriptions for all the layers
        for (int i = 0; i < 261; ++i)
        {
            Level(i).describe();
            Level(i, 0).describe();
            Level(i, MISSING_INT, i, MISSING_INT).describe();
            Level(i, 0, i, 0).describe();
        }
    }),
    Test("trange_descs", [](Fixture& f) {
        // Try to get descriptions for all the time ranges
        for (int i = 0; i < 256; ++i)
        {
            Trange(i).describe();
            Trange(i, 0).describe();
            Trange(i, 0, 0).describe();
        }
    }),
    Test("known_descs", [](Fixture& f) {
        // Verify some well-known descriptions
        wassert(actual(Level().describe()) == "Information about the station that generated the data");
        wassert(actual(Level(103, 2000).describe()) == "2.000m above ground");
        wassert(actual(Level(103, 2000, 103, 4000).describe()) ==
                "Layer from [2.000m above ground] to [4.000m above ground]");
        wassert(actual(Trange(254, 86400).describe()) ==
                "Forecast at t+1d, instantaneous value");
        wassert(actual(Trange(2, 0, 43200).describe()) == "Maximum over 12h at forecast time 0");
        wassert(actual(Trange(3, 194400, 43200).describe()) == "Minimum over 12h at forecast time 2d 6h");
    }),
};

test_group newtg("dballe_types", tests);

}
