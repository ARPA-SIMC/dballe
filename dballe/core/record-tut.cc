#include "core/test-utils-core.h"
#include "core/record.h"

using namespace wibble::tests;
using namespace dballe;
using namespace std;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

#define fail_unless_int_is(keyvar, param, value) do { \
		int found; \
		int val; \
		CHECKED(dba_record_##keyvar##_enqi(rec, param, &val, &found)); \
		ensure_equals(found, 1); \
		ensure_equals(val, value); \
	} while (0)

#define fail_unless_double_is(keyvar, param, value) do { \
		int found; \
		double val; \
		CHECKED(dba_record_##keyvar##_enqd(rec, param, &val, &found)); \
		ensure_equals(found, 1); \
		ensure_equals(val, value); \
	} while (0)

#define fail_unless_char_is(keyvar, param, value) do { \
		const char* val; \
		CHECKED(dba_record_##keyvar##_enqc(rec, param, &val)); \
		ensure(val != NULL); \
		ensure_equals(string(val), string(value)); \
	} while (0)


std::vector<Test> tests {
    Test("keyword_name", [](Fixture& f) {
        // Keyword name resolution
        using namespace dballe::core;
        ensure(core::Record::keyword_byname("cippo") == DBA_KEY_ERROR);
        ensure(core::Record::keyword_byname("zzzip") == DBA_KEY_ERROR);

        ensure(core::Record::keyword_byname("ana_id") == DBA_KEY_ANA_ID);
        ensure(core::Record::keyword_byname_len("ana_idi", 6) == DBA_KEY_ANA_ID);
        wassert(actual(core::Record::keyword_info(DBA_KEY_ANA_ID)->desc) == "Station database ID");

        ensure(core::Record::keyword_byname("yearmin") == DBA_KEY_YEARMIN);
        wassert(actual(core::Record::keyword_info(DBA_KEY_YEARMIN)->desc) == "Year or minimum year queried");

        ensure(core::Record::keyword_byname("lat") == DBA_KEY_LAT);
        wassert(actual(core::Record::keyword_info(DBA_KEY_LAT)->desc) == "Latitude");

        ensure(core::Record::keyword_byname("lon") == DBA_KEY_LON);
        wassert(actual(core::Record::keyword_info(DBA_KEY_LON)->desc) == "Longitude");
    }),
    Test("get_set", [](Fixture& f) {
        // Check that things don't exist at the beginning
        core::Record rec;
        ensure(rec.get("ana_id") == NULL);
        ensure(rec.get("lat") == NULL);
        ensure(rec.get("B20001") == NULL);
        ensure(rec.get("B20003") == NULL);

        // Set various things
        rec.set("ana_id", -10);
        rec.set("lat", 1234567);
        rec.set("lon", 76.54321);
        rec.set("yearmin", "1976");
        rec.obtain(WR_VAR(0, 20, 1)).setc("456");
        rec.obtain(WR_VAR(0, 20, 3)).setc("456");

        // Check that they now exist
        ensure(rec.get("ana_id") != NULL);
        ensure(rec.get("lat") != NULL);
        ensure(rec.get("B20001") != NULL);
        ensure(rec.get("B20003") != NULL);

        // Check that they have the right value
        wassert(actual(rec.get("ana_id")->enqi()) == -10);
        wassert(actual(rec.get("ana_id")->enqd()) == -10.0);
        wassert(actual(rec.get("lon")->enqi()) == 7654321);
        wassert(actual(rec.get("lon")->enqd()) == 76.54321);
        wassert(actual(rec.get("lon")->enqc()) == "7654321");
        wassert(actual(rec.get("lat")->enqd()) == 12.34567);
        wassert(actual(rec.get("yearmin")->enqd()) == 1976.0);
        wassert(actual(rec.get("B20001")->enqd()) == 4560.0);
        wassert(actual(rec.get("B20003")->enqd()) == 456);

        // See if unset works for keywords
        rec.unset("lat");
        ensure(rec.get("lat") == NULL);

        // See if unset works for variables
        rec.unset("B20001");
        ensure(rec.get("B20001") == NULL);

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
        ensure(rec.get("lat") == NULL);
        ensure(rec.get("B20003") == NULL);

        rec.clear();
        ensure(rec.get("lat") == NULL);
        ensure(rec.get("B20003") == NULL);
    }),
    Test("ident", [](Fixture& f) {
        // This used to cause a segfault
        core::Record rec;
        rec.setc("ident", "nosort");

        core::Record rec1;
        rec = rec;
        rec.setc("ident", "nosort");
    }),
    Test("comparisons", [](Fixture& f) {
        core::Record rec;
        rec.set("ana_id", -10);
        rec.set("lat", 1234567);
        rec.set("lon", 76.54321);
        rec.set("yearmin", "1976");
        rec.obtain(WR_VAR(0, 20, 1)).setc("456");
        rec.obtain(WR_VAR(0, 20, 3)).setc("456");

        core::Record rec1;
        rec1 = rec;
        ensure(rec == rec1);
        ensure(rec1 == rec);
        ensure(!(rec != rec1));
        ensure(!(rec1 != rec));
        rec1.seti("yearmin", 1975);
        ensure(rec != rec1);
        ensure(rec1 != rec);
        ensure(!(rec == rec1));
        ensure(!(rec1 == rec));

        rec1 = rec;
        ensure(rec == rec1);
        ensure(rec1 == rec);
        rec1.unset("yearmin");
        ensure(rec != rec1);
        ensure(rec1 != rec);

        rec1 = rec;
        ensure(rec == rec1);
        ensure(rec1 == rec);
        rec1.obtain(WR_VAR(0, 20, 1)).setc("45");
        ensure(rec != rec1);
        ensure(rec1 != rec);

        rec1 = rec;
        ensure(rec == rec1);
        ensure(rec1 == rec);
        rec1.unset("B20001");
        ensure(rec != rec1);
        ensure(rec1 != rec);
    }),
    Test("get_set_level", [](Fixture& f) {
        core::Record rec;
        ensure_equals(rec.get_level(), Level());

        rec.set("leveltype1", 1);
        rec.set("l1", 0);
        rec.set("leveltype2", 2);
        rec.set("l2", 3);
        ensure_equals(rec.get_level(), Level(1, 0, 2, 3));

        rec.set(Level(9, 8));
        ensure_equals(rec.enq("leveltype1", 0), 9);
        ensure_equals(rec.enq("l1", 0), 8);
        ensure_equals(rec.enq("leveltype2", 0), 0);
        ensure_equals(rec.enq("l2", 0), 0);
        ensure_equals(rec.get_level(), Level(9, 8));
    }),
    Test("get_set_trange", [](Fixture& f) {
        core::Record rec;
        ensure_equals(rec.get_trange(), Trange());

        rec.set("pindicator", 11);
        rec.set("p1", 22);
        rec.set("p2", 33);
        ensure_equals(rec.get_trange(), Trange(11, 22, 33));

        rec.set(Trange(7, 6));
        ensure_equals(rec.enq("pindicator", 0), 7);
        ensure_equals(rec.enq("p1", 0), 6);
        ensure_equals(rec.enq("p2", 0), 0);
        ensure_equals(rec.get_trange(), Trange(7, 6));
    }),
    Test("get_set_datetime", [](Fixture& f) {
        core::Record rec;
        rec.set(Datetime(2013, 11, 1, 12, 0, 0));
        wassert(actual(rec.get_datetime()) == Datetime(2013, 11, 1, 12));

        rec.set(Datetime(2012, 5, 15, 17, 30, 30));
        Datetime dt = rec.get_datetime();
        wassert(actual(dt) == Datetime(2012, 5, 15, 17, 30, 30));
    }),
    Test("iter", [](Fixture& f) {
        // Test iteration
        using namespace dballe::core;
        core::Record rec;
        rec.set("priority", 1);
        rec.set(Datetime(2013, 11, 1, 12, 0, 0));
        rec.set("var_related", "B12123");

        unsigned count = 0;
        bool res = rec.iter_keys([&count](dba_keyword k, const wreport::Var& v) {
            ++count;
            return true;
        });

        wassert(actual(res).istrue());
        wassert(actual(count) == 8);
    }),
    Test("extremes", [](Fixture& f) {
        // Test querying extremes by Datetime
        core::Record rec;

        Datetime dtmin, dtmax;

        rec.parse_date_extremes(dtmin, dtmax);
        wassert(actual(dtmin.is_missing()).istrue());
        wassert(actual(dtmax.is_missing()).istrue());

        rec.setmin(Datetime(2010, 1, 1, 0, 0, 0));
        rec.parse_date_extremes(dtmin, dtmax);
        wassert(actual(dtmin) == Datetime(2010, 1, 1, 0, 0, 0));
        wassert(actual(dtmax.is_missing()).istrue());

        rec.setmax(Datetime(2011, 2, 3, 4, 5, 6));
        rec.parse_date_extremes(dtmin, dtmax);
        wassert(actual(dtmin) == Datetime(2010, 1, 1, 0, 0, 0));
        wassert(actual(dtmax) == Datetime(2011, 2, 3, 4, 5, 6));
    }),
};

test_group newtg("dballe_core_record", tests);

}
