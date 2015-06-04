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
        wassert(actual(Date(2013, 1, 1)) < Date(2014, 1, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 2, 1));
        wassert(actual(Date(2013, 1, 1)) < Date(2013, 1, 2));
        wassert(actual(Date(1945, 4, 25)) != Date(1945, 4, 26));
    }),
    Test("time", [](Fixture& f) {
        wassert(actual(Time(13, 1,  1)) <  Time(14, 1,  1));
        wassert(actual(Time(13, 1,  1)) <  Time(13, 2,  1));
        wassert(actual(Time(13, 1,  1)) <  Time(13, 1,  2));
        wassert(actual(Time(19, 4, 25)) != Time(19, 4, 26));
    }),
    Test("datetime", [](Fixture& f) {
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2014, 1, 1, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 2, 1, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 2, 0, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 1, 0, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 0, 1, 0));
        wassert(actual(Datetime(2013, 1, 1, 0, 0, 0)) < Datetime(2013, 1, 1, 0, 0, 1));
        wassert(actual(Datetime(1945, 4, 25, 8, 0, 0)) != Datetime(1945, 4, 26, 8, 0, 0));
    }),
    Test("datetime_ranges", [](Fixture& f) {
        Datetime missing;
        Datetime dt_2010(2010, 1, 1, 0, 0, 0);
        Datetime dt_2011(2011, 1, 1, 0, 0, 0);
        Datetime dt_2012(2012, 1, 1, 0, 0, 0);
        Datetime dt_2013(2013, 1, 1, 0, 0, 0);

        // Test equality
        wassert(actual(Datetime::range_equals(missing, missing, missing, missing)).istrue());
        wassert(actual(Datetime::range_equals(dt_2010, dt_2011, dt_2010, dt_2011)).istrue());
        wassert(actual(Datetime::range_equals(dt_2010, missing, missing, missing)).isfalse());
        wassert(actual(Datetime::range_equals(missing, dt_2010, missing, missing)).isfalse());
        wassert(actual(Datetime::range_equals(missing, missing, dt_2010, missing)).isfalse());
        wassert(actual(Datetime::range_equals(missing, missing, missing, dt_2010)).isfalse());
        wassert(actual(Datetime::range_equals(dt_2010, dt_2011, dt_2012, dt_2013)).isfalse());

        // Test contains
        wassert(actual(Datetime::range_contains(missing, missing, missing, missing)).istrue());
        wassert(actual(Datetime::range_contains(dt_2010, dt_2011, dt_2010, dt_2011)).istrue());
        wassert(actual(Datetime::range_contains(missing, missing, dt_2011, dt_2012)).istrue());
        wassert(actual(Datetime::range_contains(dt_2011, dt_2012, missing, missing)).isfalse());
        wassert(actual(Datetime::range_contains(dt_2010, dt_2013, dt_2011, dt_2012)).istrue());
        wassert(actual(Datetime::range_contains(dt_2010, dt_2012, dt_2011, dt_2013)).isfalse());
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
