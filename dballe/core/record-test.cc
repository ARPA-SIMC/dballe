#include "core/tests.h"
#include "core/record.h"

using namespace dballe::tests;
using namespace dballe;
using namespace std;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override
    {
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


        add_method("keyword_name", []() {
            // Keyword name resolution
            using namespace dballe::core;
            wassert(actual(core::Record::keyword_byname("cippo") == DBA_KEY_ERROR).istrue());
            wassert(actual(core::Record::keyword_byname("zzzip") == DBA_KEY_ERROR).istrue());

            wassert(actual(core::Record::keyword_byname("ana_id") == DBA_KEY_ANA_ID).istrue());
            wassert(actual(core::Record::keyword_byname_len("ana_idi", 6) == DBA_KEY_ANA_ID).istrue());
            wassert(actual(core::Record::keyword_info(DBA_KEY_ANA_ID)->desc) == "Station database ID");

            wassert(actual(core::Record::keyword_byname("yearmin") == DBA_KEY_YEARMIN).istrue());
            wassert(actual(core::Record::keyword_info(DBA_KEY_YEARMIN)->desc) == "Year or minimum year queried");

            wassert(actual(core::Record::keyword_byname("lat") == DBA_KEY_LAT).istrue());
            wassert(actual(core::Record::keyword_info(DBA_KEY_LAT)->desc) == "Latitude");

            wassert(actual(core::Record::keyword_byname("lon") == DBA_KEY_LON).istrue());
            wassert(actual(core::Record::keyword_info(DBA_KEY_LON)->desc) == "Longitude");
        });
        add_method("get_set", []() {
            // Check that things don't exist at the beginning
            core::Record rec;
            wassert(actual(rec.get("ana_id")).isfalse());
            wassert(actual(rec.get("lat")).isfalse());
            wassert(actual(rec.get("B20001")).isfalse());
            wassert(actual(rec.get("B20003")).isfalse());

            // Set various things
            rec.set("ana_id", -10);
            rec.set("lat", 1234567);
            rec.set("lon", 76.54321);
            rec.set("yearmin", "1976");
            rec.set("B20001", "456");
            rec.set("B20003", "456");

            // Check that they now exist
            wassert(actual(rec.get("ana_id")).istrue());
            wassert(actual(rec.get("lat")).istrue());
            wassert(actual(rec.get("B20001")).istrue());
            wassert(actual(rec.get("B20003")).istrue());

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
            wassert(actual(rec.get("lat")).isfalse());

            // See if unset works for variables
            rec.unset("B20001");
            wassert(actual(rec.get("B20001")).isfalse());

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
            wassert(actual(rec.get("lat")).isfalse());
            wassert(actual(rec.get("B20003")).isfalse());

            rec.clear();
            wassert(actual(rec.get("lat")).isfalse());
            wassert(actual(rec.get("B20003")).isfalse());
        });
        add_method("ident", []() {
            // This used to cause a segfault
            core::Record rec;
            rec.setc("ident", "nosort");

            core::Record rec1;
            rec = rec;
            rec.setc("ident", "nosort");
        });
        add_method("comparisons", []() {
            core::Record rec;
            rec.set("ana_id", -10);
            rec.set("lat", 1234567);
            rec.set("lon", 76.54321);
            rec.set("yearmin", "1976");
            rec.set("B20001", "456");
            rec.set("B20003", "456");

            core::Record rec1;
            rec1 = rec;
            wassert(actual(rec == rec1).istrue());
            wassert(actual(rec1 == rec).istrue());
            wassert(actual(!(rec != rec1)).istrue());
            wassert(actual(!(rec1 != rec)).istrue());
            rec1.seti("yearmin", 1975);
            wassert(actual(rec != rec1).istrue());
            wassert(actual(rec1 != rec).istrue());
            wassert(actual(!(rec == rec1)).istrue());
            wassert(actual(!(rec1 == rec)).istrue());

            rec1 = rec;
            wassert(actual(rec == rec1).istrue());
            wassert(actual(rec1 == rec).istrue());
            rec1.unset("yearmin");
            wassert(actual(rec != rec1).istrue());
            wassert(actual(rec1 != rec).istrue());

            rec1 = rec;
            wassert(actual(rec == rec1).istrue());
            wassert(actual(rec1 == rec).istrue());
            rec1.set("B20001", "45");
            wassert(actual(rec != rec1).istrue());
            wassert(actual(rec1 != rec).istrue());

            rec1 = rec;
            wassert(actual(rec == rec1).istrue());
            wassert(actual(rec1 == rec).istrue());
            rec1.unset("B20001");
            wassert(actual(rec != rec1).istrue());
            wassert(actual(rec1 != rec).istrue());
        });
        add_method("get_set_level", []() {
            core::Record rec;
            wassert(actual(rec.get_level()) == Level());

            rec.set("leveltype1", 1);
            rec.set("l1", 0);
            rec.set("leveltype2", 2);
            rec.set("l2", 3);
            wassert(actual(rec.get_level()) == Level(1, 0, 2, 3));

            rec.set(Level(9, 8));
            wassert(actual(rec.enq("leveltype1", 0)) == 9);
            wassert(actual(rec.enq("l1", 0)) == 8);
            wassert(actual(rec.enq("leveltype2", 0)) == 0);
            wassert(actual(rec.enq("l2", 0)) == 0);
            wassert(actual(rec.get_level()) == Level(9, 8));
        });
        add_method("get_set_trange", []() {
            core::Record rec;
            wassert(actual(rec.get_trange()) == Trange());

            rec.set("pindicator", 11);
            rec.set("p1", 22);
            rec.set("p2", 33);
            wassert(actual(rec.get_trange()) == Trange(11, 22, 33));

            rec.set(Trange(7, 6));
            wassert(actual(rec.enq("pindicator", 0)) == 7);
            wassert(actual(rec.enq("p1", 0)) == 6);
            wassert(actual(rec.enq("p2", 0)) == 0);
            wassert(actual(rec.get_trange()) == Trange(7, 6));
        });
        add_method("get_set_datetime", []() {
            core::Record rec;
            rec.set(Datetime(2013, 11, 1, 12, 0, 0));
            wassert(actual(rec.get_datetime()) == Datetime(2013, 11, 1, 12));

            rec.set(Datetime(2012, 5, 15, 17, 30, 30));
            Datetime dt = rec.get_datetime();
            wassert(actual(dt) == Datetime(2012, 5, 15, 17, 30, 30));
        });
        add_method("iter", []() {
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
    }
} test("core_record");

}
