#include "dballe/core/tests.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/record-access.h"
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
    wassert(actual(q.prio_min) == MISSING_INT);
    wassert(actual(q.prio_max) == MISSING_INT);
    wassert(actual(q.rep_memo) == "");
    wassert(actual(q.mobile) == MISSING_INT);
    wassert(actual(q.ident.is_missing()).istrue());
    wassert(actual(q.latrange) == LatRange());
    wassert(actual(q.lonrange) == LonRange());
    wassert(actual(q.datetime) == DatetimeRange());
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

add_method("all_set", []() {
    core::Record rec;
    record_seti(rec, "ana_id", 4);
    record_seti(rec, "priority", 1);
    record_setc(rec, "rep_memo", "foo");
    record_seti(rec, "mobile", 0);
    record_setc(rec, "ident", "bar");
    record_setd(rec, "lat", 44.123);
    record_setd(rec, "lon", 11.123);
    rec.set_datetime(Datetime(2000, 1, 2, 12, 30, 45));
    rec.set_level(Level(10, 11, 12, 13));
    rec.set_trange(Trange(20, 21, 22));
    record_setc(rec, "var", "B12101");
    record_setc(rec, "query", "best");
    record_setc(rec, "ana_filter", "B01001=1");
    record_setc(rec, "data_filter", "B12101>260");
    record_setc(rec, "attr_filter", "B33007>50");
    record_seti(rec, "limit", 100);
    record_seti(rec, "block", 16);
    record_seti(rec, "station", 404);

    core::Query q;
    q.set_from_record(rec);
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

add_method("prio", []() {
    core::Query q;
    q.set_from_test_string("priority=11");
    wassert(actual(q.prio_min) == 11);
    wassert(actual(q.prio_max) == 11);

    q.clear();
    q.set_from_test_string("priomin=12");
    wassert(actual(q.prio_min) == 12);
    wassert(actual(q.prio_max) == MISSING_INT);

    q.clear();
    q.set_from_test_string("priomax=12");
    wassert(actual(q.prio_min) == MISSING_INT);
    wassert(actual(q.prio_max) == 12);

    q.clear();
    q.set_from_test_string("priomin=11, priomax=22");
    wassert(actual(q.prio_min) == 11);
    wassert(actual(q.prio_max) == 22);

    q.clear();
    q.set_from_test_string("priomin=11, priomax=22, priority=16");
    wassert(actual(q.prio_min) == 16);
    wassert(actual(q.prio_max) == 16);
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
    try {
        q.set_from_test_string("lonmin=45.0");
        wassert(actual(false).istrue());
    } catch (std::exception& e) {
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

add_method("datetime", []() {
    core::Query q;
    wassert(q.set_from_test_string("year=2015"));
    wassert(actual(q.datetime) == DatetimeRange(2015,  1,  1,  0 , 0,  0, 2015, 12, 31, 23, 59, 59));

    q.clear();
    wassert(q.set_from_test_string("year=2015, monthmin=1, monthmax=2"));
    wassert(actual(q.datetime) == DatetimeRange(2015,  1,  1,  0 , 0,  0, 2015,  2, 28, 23, 59, 59));

    q.clear();
    auto e = wassert_throws(wreport::error_consistency, q.set_from_test_string("year=2015, monthmin=2, day=28"));
    wassert(actual(e.what()) == "day 28 given with no month");

    q.clear();
    e = wassert_throws(wreport::error_consistency, q.set_from_test_string("yearmin=2000, yearmax=2012, month=2, min=30"));
    wassert(actual(e.what()) == "minute 30 given with no hour");

    q.clear();
    wassert(q.set_from_test_string("yearmin=2010, yearmax=2012, year=2000, month=2, hour=12, min=30"));
    wassert(actual(q.datetime) == DatetimeRange(2000,  2,  1,  12, 30,  0, 2000,  2, 29, 12, 30, 59));

    q.clear();
    e = wassert_throws(wreport::error_consistency, q.set_from_test_string("yearmin=2010, yearmax=2012, year=2000, month=2, min=30"));
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
    wassert(actual(*q.varcodes.begin())  == WR_VAR(0,  1,   1));
    wassert(actual(*q.varcodes.rbegin()) == WR_VAR(0, 12, 101));

    q.clear();
    q.set_from_test_string("varlist=B12101,B01001, var=B12102");
    wassert(actual(q.varcodes.size()) == 1);
    wassert(actual(*q.varcodes.begin()) == WR_VAR(0, 12, 102));
});

add_method("modifiers", []() {
    wassert(actual(core::Query::parse_modifiers("best")) == DBA_DB_MODIFIER_BEST);
    wassert(actual(core::Query::parse_modifiers("details")) == DBA_DB_MODIFIER_SUMMARY_DETAILS);
    wassert(actual(core::Query::parse_modifiers("attrs")) == DBA_DB_MODIFIER_WITH_ATTRIBUTES);
    wassert(actual(core::Query::parse_modifiers("best,attrs")) == (DBA_DB_MODIFIER_BEST | DBA_DB_MODIFIER_WITH_ATTRIBUTES));
});

add_method("issue107", []() {
    core::Record rec;
    record_seti(rec, "month", 6);

    core::Query q;
    wassert_throws(wreport::error_consistency, q.set_from_record(rec));
});

}

}
