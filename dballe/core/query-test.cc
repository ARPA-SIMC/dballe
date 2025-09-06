#include "dballe/core/query.h"
#include "dballe/core/tests.h"
#include <stdexcept>

using namespace std;
using namespace dballe;
using namespace dballe::tests;
using namespace wreport;

namespace {

class Tests : public TestCase
{
    using TestCase::TestCase;

    void register_tests() override;
} test("core_query");

void Tests::register_tests()
{

    add_method("all_unset", []() {
        core::Query q;
        wassert(actual(q.ana_id) == MISSING_INT);
        wassert(actual(q.priomin) == MISSING_INT);
        wassert(actual(q.priomax) == MISSING_INT);
        wassert(actual(q.report) == "");
        wassert(actual(q.mobile) == MISSING_INT);
        wassert(actual(q.ident.is_missing()).istrue());
        wassert(actual(q.latrange) == LatRange());
        wassert(actual(q.lonrange) == LonRange());
        wassert(actual(q.dtrange) == DatetimeRange());
        wassert(actual(q.level) == Level());
        wassert(actual(q.trange) == Trange());
        wassert(actual(q.varcodes.size()) == 0);
        wassert(actual(q.query) == "");
        wassert(actual(q.ana_filter) == "");
        wassert(actual(q.data_filter) == "");
        wassert(actual(q.attr_filter) == "");
        wassert(actual(q.limit) == MISSING_INT);
        wassert(actual(q.block) == MISSING_INT);
        wassert(actual(q.station) == MISSING_INT);
    });

#if 0
add_method("all_set", []() {
    core::Query q;
    q.setf("ana_id", "4");
    q.setf("priority", "1");
    q.setf("rep_memo", "foo");
    q.setf("mobile", "0");
    q.setf("ident", "bar");
    q.setf("lat", "44.123");
    q.setf("lon", "11.123");
    q.datetime.min = q.datetime.max = Datetime(2000, 1, 2, 12, 30, 45);
    q.level = Level(10, 11, 12, 13);
    q.trange = Trange(20, 21, 22);
    q.setf("var", "B12101");
    q.setf("query", "best");
    q.setf("ana_filter", "B01001=1");
    q.setf("data_filter", "B12101>260");
    q.setf("attr_filter", "B33007>50");
    q.setf("limit", "100");
    q.setf("block", "16");
    q.setf("station", "404");

    wassert(actual(q.ana_id) == 4);
    wassert(actual(q.prio_min) == 1);
    wassert(actual(q.prio_max) == 1);
    wassert(actual(q.rep_memo) == "foo");
    wassert(actual(q.mobile) == 0);
    wassert(actual(q.ident.is_missing()).isfalse());
    wassert(actual(q.ident.get()) == "bar");
    wassert(actual(q.latrange) == LatRange(44.123, 44.123));
    wassert(actual(q.lonrange) == LonRange(11.123, 11.123));
    wassert(actual(q.datetime) == DatetimeRange(2000, 1, 2, 12, 30, 45, 2000, 1, 2, 12, 30, 45));
    wassert(actual(q.level) == Level(10, 11, 12, 13));
    wassert(actual(q.trange) == Trange(20, 21, 22));
    wassert(actual(q.varcodes.size()) == 1);
    wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));
    wassert(actual(q.query) == "best");
    wassert(actual(q.ana_filter) == "B01001=1");
    wassert(actual(q.data_filter) == "B12101>260");
    wassert(actual(q.attr_filter) == "B33007>50");
    wassert(actual(q.limit) == 100);
    wassert(actual(q.block) == 16);
    wassert(actual(q.station) == 404);
});
#endif

    add_method("prio", []() {
        core::Query q;
        q.set_from_test_string("priority=11");
        wassert(actual(q.priomin) == 11);
        wassert(actual(q.priomax) == 11);

        q.clear();
        q.set_from_test_string("priomin=12");
        wassert(actual(q.priomin) == 12);
        wassert(actual(q.priomax) == MISSING_INT);

        q.clear();
        q.set_from_test_string("priomax=12");
        wassert(actual(q.priomin) == MISSING_INT);
        wassert(actual(q.priomax) == 12);

        q.clear();
        q.set_from_test_string("priomin=11, priomax=22");
        wassert(actual(q.priomin) == 11);
        wassert(actual(q.priomax) == 22);

        q.clear();
        q.set_from_test_string("priomin=11, priomax=22, priority=16");
        wassert(actual(q.priomin) == 16);
        wassert(actual(q.priomax) == 16);
    });

    add_method("lat", []() {
        core::Query q;
        q.set_from_test_string("lat=45.0");
        wassert(actual(q.latrange) == LatRange(45.0, 45.0));

        q.clear();
        q.set_from_test_string("latmin=45.0");
        wassert(actual(q.latrange) == LatRange(45.0, LatRange::DMAX));

        q.clear();
        q.set_from_test_string("latmax=45.0");
        wassert(actual(q.latrange) == LatRange(LatRange::DMIN, 45.0));

        q.clear();
        q.set_from_test_string("latmin=40.0, latmax=50.0");
        wassert(actual(q.latrange) == LatRange(40.0, 50.0));

        q.clear();
        q.set_from_test_string("latmin=40.0, latmax=50.0, lat=42.0");
        wassert(actual(q.latrange) == LatRange(42.0, 42.0));
    });

    add_method("lon", []() {
        core::Query q;
        q.set_from_test_string("lon=45.0");
        wassert(actual(q.lonrange) == LonRange(45.0, 45.0));

        q.clear();
        try
        {
            q.set_from_test_string("lonmin=45.0");
            wassert(actual(false).istrue());
        }
        catch (std::exception& e)
        {
            wassert(actual(e.what()).contains("open ended range"));
        }
        wassert(actual(q.lonrange) == LonRange());

        q.clear();
        q.set_from_test_string("lonmin=40.0, lonmax=50.0");
        wassert(actual(q.lonrange) == LonRange(40.0, 50.0));

        q.clear();
        q.set_from_test_string("lonmin=40.0, lonmax=50.0, lon=42.0");
        wassert(actual(q.lonrange) == LonRange(42.0, 42.0));
    });

    add_method("lonrange", []() {
        // See issue #132: setting lonmin/lonmax now normalizes at each set
        core::Query q;
        q.set_from_test_string("lonmin=0, lonmax=360.0");
        wassert(actual(q.lonrange) == LonRange(0.0, 0.0));
    });

    add_method("dtrange", []() {
        core::Query q;
        wassert(q.set_from_test_string("year=2015"));
        wassert(actual(q.dtrange) ==
                DatetimeRange(2015, 1, 1, 0, 0, 0, 2015, 12, 31, 23, 59, 59));

        q.clear();
        wassert(q.set_from_test_string("year=2015, monthmin=1, monthmax=2"));
        wassert(actual(q.dtrange) ==
                DatetimeRange(2015, 1, 1, 0, 0, 0, 2015, 2, 28, 23, 59, 59));

        q.clear();
        auto e = wassert_throws(
            wreport::error_consistency,
            q.set_from_test_string("year=2015, monthmin=2, day=28"));
        wassert(actual(e.what()) == "day 28 given with no month");

        q.clear();
        e = wassert_throws(wreport::error_consistency,
                           q.set_from_test_string(
                               "yearmin=2000, yearmax=2012, month=2, min=30"));
        wassert(actual(e.what()) == "minute 30 given with no hour");

        q.clear();
        wassert(q.set_from_test_string(
            "yearmin=2010, yearmax=2012, year=2000, month=2, hour=12, min=30"));
        wassert(actual(q.dtrange) ==
                DatetimeRange(2000, 2, 1, 12, 30, 0, 2000, 2, 29, 12, 30, 59));

        q.clear();
        e = wassert_throws(
            wreport::error_consistency,
            q.set_from_test_string(
                "yearmin=2010, yearmax=2012, year=2000, month=2, min=30"));
        wassert(actual(e.what()) == "minute 30 given with no hour");
    });

    add_method("varlist", []() {
        core::Query q;
        q.set_from_test_string("var=B12101");
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 101));

        q.clear();
        q.set_from_test_string("varlist=B12101,B01001");
        wassert(actual(q.varcodes.size()) == 2);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 1, 1));
        wassert(actual(*q.varcodes.rbegin()) == WR_VAR(0, 12, 101));

        q.clear();
        q.set_from_test_string("varlist=B12101,B01001, var=B12102");
        wassert(actual(q.varcodes.size()) == 1);
        wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 102));
    });

    add_method("modifiers", []() {
        wassert(actual(core::Query::parse_modifiers("best")) ==
                DBA_DB_MODIFIER_BEST);
        wassert(actual(core::Query::parse_modifiers("details")) ==
                DBA_DB_MODIFIER_SUMMARY_DETAILS);
        wassert(actual(core::Query::parse_modifiers("attrs")) ==
                DBA_DB_MODIFIER_WITH_ATTRIBUTES);
        wassert(actual(core::Query::parse_modifiers("best,attrs")) ==
                (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_WITH_ATTRIBUTES));
        wassert(actual(core::Query::parse_modifiers("last")) ==
                DBA_DB_MODIFIER_LAST);
    });

    add_method("issue107", []() {
        core::Query q;
        wassert_throws(wreport::error_consistency,
                       q.set_from_test_string("month=6"));
    });

    add_method("setf_invalid_int", [] {
        core::Query q;
        wassert_throws(wreport::error_domain, q.set_from_string("priomin=1Kg"));
        wassert_throws(wreport::error_domain,
                       q.set_from_string("priomin=1 Kg"));
        wassert_throws(wreport::error_domain,
                       q.set_from_string("leveltype2=1Km"));
        wassert_throws(wreport::error_domain, q.set_from_string("year=2025ce"));
        wassert_throws(wreport::error_domain,
                       q.set_from_string("month=january"));
        wassert_throws(wreport::error_domain, q.set_from_string("lat=45°"));
        wassert_throws(wreport::error_domain, q.set_from_string("latmax=45°"));
        wassert_throws(wreport::error_domain, q.set_from_string("latmin=45°"));
        wassert_throws(wreport::error_domain, q.set_from_string("lon=11°"));
        wassert_throws(wreport::error_domain, q.set_from_string("lonmax=11°"));
        wassert_throws(wreport::error_domain, q.set_from_string("lonmin=11°"));
        // Spaces in the end are ok
        q.set_from_string("priomin=1   ");
    });

    add_method("setf_unset", []() {
        core::Query q;

        q.set_from_string("priority=-");
        wassert(actual(q.priomin) == MISSING_INT);
        wassert(actual(q.priomax) == MISSING_INT);

        q.set_from_test_string("priomin=1, priomax=-");
        wassert(actual(q.priomin) == 1);
        wassert(actual(q.priomax) == MISSING_INT);

        q.set_from_test_string("priomin=-, priomax=1");
        wassert(actual(q.priomin) == MISSING_INT);
        wassert(actual(q.priomax) == 1);

        q.set_from_string("rep_memo=-");
        wassert(actual(q.report) == std::string());
        q.set_from_string("report=-");
        wassert(actual(q.report) == std::string());

        q.set_from_string("ana_id=-");
        wassert(actual(q.ana_id) == MISSING_INT);

        q.set_from_string("mobile=-");
        wassert(actual(q.mobile) == MISSING_INT);

        q.set_from_string("ident=-");
        wassert(actual(q.ident.is_missing()).istrue());

        q.set_from_string("lat=-");
        wassert(actual(q.latrange.is_missing()).istrue());

        q.set_from_string("lon=-");
        wassert(actual(q.lonrange.is_missing()).istrue());

        wassert(q.set_from_test_string("latmin=1, latmax=-"));
        wassert(actual(q.latrange.imin) == Coords::lat_to_int(1));
        wassert(actual(q.latrange.imax) == LatRange::IMAX);
        wassert(q.set_from_test_string("latmin=-, latmax=1"));
        wassert(actual(q.latrange.imin) == LatRange::IMIN);
        wassert(actual(q.latrange.imax) == Coords::lat_to_int(1));

        wassert(q.set_from_test_string("lonmin=-, lonmax=-"));
        wassert(actual(q.lonrange.imin) == MISSING_INT);
        wassert(actual(q.lonrange.imax) == MISSING_INT);

        q.set_from_string("year=-");
        wassert(actual(q.dtrange.min.year) == 0xffff);
        wassert(actual(q.dtrange.max.year) == 0xffff);
        q.set_from_string("month=-");
        wassert(actual(q.dtrange.min.month) == 0xff);
        wassert(actual(q.dtrange.max.month) == 0xff);
        q.set_from_string("day=-");
        wassert(actual(q.dtrange.min.day) == 0xff);
        wassert(actual(q.dtrange.max.day) == 0xff);
        q.set_from_string("hour=-");
        wassert(actual(q.dtrange.min.hour) == 0xff);
        wassert(actual(q.dtrange.max.hour) == 0xff);
        q.set_from_string("min=-");
        wassert(actual(q.dtrange.min.minute) == 0xff);
        wassert(actual(q.dtrange.max.minute) == 0xff);
        q.set_from_string("sec=-");
        wassert(actual(q.dtrange.min.second) == 0xff);
        wassert(actual(q.dtrange.max.second) == 0xff);

        // TODO: use actual_int from newer wreport/wobble
        wassert(q.set_from_test_string("yearmin=1, yearmax=-"));
        wassert(actual((int)q.dtrange.min.year) == 1);
        wassert(actual((int)q.dtrange.max.year) == 0xffff);
        wassert(q.set_from_test_string("yearmin=-, yearmax=1"));
        wassert(actual((int)q.dtrange.min.year) == 0xffff);
        wassert(actual((int)q.dtrange.max.year) == 1);
        wassert(q.set_from_test_string("year=2025, monthmin=1, monthmax=-"));
        wassert(actual((int)q.dtrange.min.month) == 1);
        wassert(actual((int)q.dtrange.max.month) == 12);
        wassert(q.set_from_test_string("year=2025, monthmin=-, monthmax=1"));
        wassert(actual((int)q.dtrange.min.month) == 1);
        wassert(actual((int)q.dtrange.max.month) == 1);
        wassert(
            q.set_from_test_string("year=2025, month=9, daymin=-, daymax=15"));
        wassert(actual((int)q.dtrange.min.day) == 1);
        wassert(actual((int)q.dtrange.max.day) == 15);
        wassert(
            q.set_from_test_string("year=2025, month=9, daymin=15, daymax=-"));
        wassert(actual((int)q.dtrange.min.day) == 15);
        wassert(actual((int)q.dtrange.max.day) == 30);
        wassert(q.set_from_test_string(
            "year=2025, month=9, day=15, hourmin=-, hourmax=15"));
        wassert(actual((int)q.dtrange.min.hour) == 0);
        wassert(actual((int)q.dtrange.max.hour) == 15);
        wassert(q.set_from_test_string(
            "year=2025, month=9, day=15, hourmin=15, hourmax=-"));
        wassert(actual((int)q.dtrange.min.hour) == 15);
        wassert(actual((int)q.dtrange.max.hour) == 23);
        wassert(q.set_from_test_string(
            "year=2025, month=9, day=15, hour=12, minumin=-, minumax=30"));
        wassert(actual((int)q.dtrange.min.minute) == 0);
        wassert(actual((int)q.dtrange.max.minute) == 30);
        wassert(q.set_from_test_string(
            "year=2025, month=9, day=15, hour=12, minumin=30, minumax=-"));
        wassert(actual((int)q.dtrange.min.minute) == 30);
        wassert(actual((int)q.dtrange.max.minute) == 59);
        wassert(q.set_from_test_string("year=2025, month=9, day=15, hour=12, "
                                       "min=30, secmin=-, secmax=30"));
        wassert(actual((int)q.dtrange.min.second) == 0);
        wassert(actual((int)q.dtrange.max.second) == 30);
        wassert(q.set_from_test_string("year=2025, month=9, day=15, hour=12, "
                                       "min=30, secmin=30, secmax=-"));
        wassert(actual((int)q.dtrange.min.second) == 30);
        wassert(actual((int)q.dtrange.max.second) == 59);

        q.set_from_string("leveltype1=-");
        wassert(actual(q.level.ltype1) == MISSING_INT);
        q.set_from_string("l1=-");
        wassert(actual(q.level.l1) == REQUIRED_MISSING_INT);
        q.set_from_string("leveltype2=-");
        wassert(actual(q.level.ltype2) == REQUIRED_MISSING_INT);
        q.set_from_string("l2=-");
        wassert(actual(q.level.l2) == REQUIRED_MISSING_INT);

        q.set_from_string("pindicator=-");
        wassert(actual(q.trange.pind) == MISSING_INT);
        q.set_from_string("p1=-");
        wassert(actual(q.trange.p1) == MISSING_INT);
        q.set_from_string("p2=-");
        wassert(actual(q.trange.p2) == MISSING_INT);

        q.set_from_string("var=-");
        wassert(actual(q.varcodes.empty()).istrue());
        q.set_from_string("varlist=-");
        wassert(actual(q.varcodes.empty()).istrue());

        q.set_from_string("query=-");
        wassert(actual(q.query) == std::string());
        q.set_from_string("ana_filter=-");
        wassert(actual(q.ana_filter) == std::string());
        q.set_from_string("data_filter=-");
        wassert(actual(q.data_filter) == std::string());
        q.set_from_string("attr_filter=-");
        wassert(actual(q.attr_filter) == std::string());

        q.set_from_string("limit=-");
        wassert(actual(q.limit) == MISSING_INT);

        q.set_from_string("block=-");
        wassert(actual(q.block) == MISSING_INT);
        q.set_from_string("station=-");
        wassert(actual(q.station) == MISSING_INT);
    });
}

} // namespace
