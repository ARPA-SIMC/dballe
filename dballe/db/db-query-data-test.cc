#include "db/tests.h"
#include "db/v6/db.h"
#include "db/v7/db.h"
#include "db/v7/transaction.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

static inline core::Query query_exact(const Datetime& dt)
{
    core::Query query;
    query.datetime = DatetimeRange(dt, dt);
    return query;
}
static inline core::Query query_min(const Datetime& dt)
{
    core::Query query;
    query.datetime = DatetimeRange(dt, Datetime());
    return query;
}
static inline core::Query query_max(const Datetime& dt)
{
    core::Query query;
    query.datetime = DatetimeRange(Datetime(), dt);
    return query;
}
static inline core::Query query_minmax(const Datetime& min, const Datetime& max)
{
    core::Query query;
    query.datetime = DatetimeRange(min, max);
    return query;
}

struct DateHourDataSet : public TestDataSet
{
    DateHourDataSet()
    {
        DataValues d;
        d.info.coords = Coords(12.34560, 76.54320);
        d.info.report = "synop";
        d.info.level = Level(10, 11, 15, 22);
        d.info.trange = Trange(20, 111, 122);
        data["1"] = d;
        data["1"].info.datetime = Datetime(2013, 10, 30, 11);
        data["1"].values.set("B12101", 11.5);
        data["2"] = d;
        data["2"].info.datetime = Datetime(2013, 10, 30, 12);
        data["2"].values.set("B12101", 12.5);
    }
};

struct DateDayDataSet : public TestDataSet
{
    DateDayDataSet()
    {
        DataValues d;
        d.info.coords = Coords(12.34560, 76.54320);
        d.info.report = "synop";
        d.info.level = Level(10, 11, 15, 22);
        d.info.trange = Trange(20, 111, 122);
        data["1"] = d;
        data["1"].info.datetime = Datetime(2013, 10, 23);
        data["1"].values.set("B12101", 23.5);
        data["2"] = d;
        data["2"].info.datetime = Datetime(2013, 10, 24);
        data["2"].values.set("B12101", 24.5);
    }
};

#define TRY_QUERY(qstring, expected_count) wassert(actual(f.tr).try_data_query(qstring, expected_count))

template<typename DB>
class OldFixtureTests : public FixtureTestCase<TransactionFixture<DB, OldDballeTestDataSet>>
{
    typedef TransactionFixture<DB, OldDballeTestDataSet> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

template<typename DB>
class EmptyFixtureTests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};


OldFixtureTests<V6DB> tg1("db_query_data1_v6_sqlite", "SQLITE");
OldFixtureTests<V7DB> tg2("db_query_data1_v7_sqlite", "SQLITE");
EmptyFixtureTests<V6DB> tg3("db_query_data2_v6_sqlite", "SQLITE");
EmptyFixtureTests<V7DB> tg4("db_query_data2_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
OldFixtureTests<V6DB> tg5("db_query_data1_v6_postgresql", "POSTGRESQL");
OldFixtureTests<V7DB> tg6("db_query_data1_v7_postgresql", "POSTGRESQL");
EmptyFixtureTests<V6DB> tg7("db_query_data2_v6_postgresql", "POSTGRESQL");
EmptyFixtureTests<V7DB> tg8("db_query_data2_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
OldFixtureTests<V6DB> tg9("db_query_data1_v6_mysql", "MYSQL");
OldFixtureTests<V7DB> tga("db_query_data1_v7_mysql", "MYSQL");
EmptyFixtureTests<V6DB> tgb("db_query_data2_v6_mysql", "MYSQL");
EmptyFixtureTests<V7DB> tgc("db_query_data2_v7_mysql", "MYSQL");
#endif

template<typename DB>
void OldFixtureTests<DB>::register_tests()
{
this->add_method("ana_id", [](Fixture& f) {
    char query[20];
    snprintf(query, 20, "ana_id=%d", f.test_data.data["synop"].info.id);
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
    if (f.db->format() != V6)
        TRY_QUERY(query, 2);
    else
        TRY_QUERY(query, 4);
    TRY_QUERY("ana_id=4242", 0);
});
this->add_method("ana_context", [](Fixture& f) {
    // Query data in station context
    core::Query query;
    unique_ptr<db::Cursor> cur = f.tr->query_station_data(query);
    wassert(actual(cur->remaining()) == 10);
});
this->add_method("year", [](Fixture& f) {
    // Datetime queries
    TRY_QUERY("year=1001", 0);
    TRY_QUERY("yearmin=1999", 0);
    TRY_QUERY("yearmin=1945", 4);
    TRY_QUERY("yearmax=1944", 0);
    TRY_QUERY("yearmax=1945", 4);
    TRY_QUERY("yearmax=2030", 4);
    TRY_QUERY("year=1944", 0);
    TRY_QUERY("year=1945", 4);
    TRY_QUERY("year=1946", 0);
    /*
    TRY_QUERY(i, DBA_KEY_MONTHMIN, 1);
    TRY_QUERY(i, DBA_KEY_MONTHMAX, 12);
    TRY_QUERY(i, DBA_KEY_MONTH, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_DAYMIN, 1);
    TRY_QUERY(i, DBA_KEY_DAYMAX, 12);
    TRY_QUERY(i, DBA_KEY_DAY, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_HOURMIN, 1);
    TRY_QUERY(i, DBA_KEY_HOURMAX, 12);
    TRY_QUERY(i, DBA_KEY_HOUR, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_MINUMIN, 1);
    TRY_QUERY(i, DBA_KEY_MINUMAX, 12);
    TRY_QUERY(i, DBA_KEY_MIN, 5);
    */
    /*
    TRY_QUERY(i, DBA_KEY_SECMIN, 1);
    TRY_QUERY(i, DBA_KEY_SECMAX, 12);
    TRY_QUERY(i, DBA_KEY_SEC, 5);
    */
});
this->add_method("block_station", [](Fixture& f) {
    // Block and station queries
    TRY_QUERY("B01001=1", 4);
    TRY_QUERY("B01001=2", 0);
    TRY_QUERY("B01002=52", 4);
    TRY_QUERY("B01002=53", 0);
});
this->add_method("ana_filter", [](Fixture& f) {
    // ana_filter queries
    TRY_QUERY("ana_filter=block=1", 4);
    TRY_QUERY("ana_filter=B01001=1", 4);
    TRY_QUERY("ana_filter=block>1", 0);
    TRY_QUERY("ana_filter=B01001>1", 0);
    TRY_QUERY("ana_filter=block<=1", 4);
    TRY_QUERY("ana_filter=B01001<=1", 4);
    TRY_QUERY("ana_filter=0<=B01001<=2", 4);
    TRY_QUERY("ana_filter=1<=B01001<=1", 4);
    TRY_QUERY("ana_filter=2<=B01001<=4", 0);
});
this->add_method("data_filter", [](Fixture& f) {
    // data_filter queries
    TRY_QUERY("data_filter=B01011=DB-All.e!", 1);
    TRY_QUERY("data_filter=B01012<300", 0);
    TRY_QUERY("data_filter=B01012<=300", 1);
    TRY_QUERY("data_filter=B01012=300", 1);
    TRY_QUERY("data_filter=B01012>=300", 2);
    TRY_QUERY("data_filter=B01012>300", 1);
    TRY_QUERY("data_filter=B01012<400", 1);
    TRY_QUERY("data_filter=B01012<=400", 2);
});
this->add_method("latlon", [](Fixture& f) {
    // latitude/longitude queries
    TRY_QUERY("latmin=11.0", 4);
    TRY_QUERY("latmin=12.34560", 4);
    TRY_QUERY("latmin=13.0", 0);
    TRY_QUERY("latmax=11.0", 0);
    TRY_QUERY("latmax=12.34560", 4);
    TRY_QUERY("latmax=13.0", 4);
    TRY_QUERY("latmin=0, latmax=20", 4);
    TRY_QUERY("latmin=-90, latmax=20", 4);
    TRY_QUERY("latmin=-90, latmax=0", 0);
    TRY_QUERY("latmin=10, latmax=90", 4);
    TRY_QUERY("latmin=45, latmax=90", 0);
    TRY_QUERY("latmin=-90, latmax=90", 4);
    TRY_QUERY("lonmin=75, lonmax=77", 4);
    TRY_QUERY("lonmin=76.54320, lonmax=76.54320", 4);
    TRY_QUERY("lonmin=76.54330, lonmax=77.", 0);
    TRY_QUERY("lonmin=77., lonmax=76.54330", 4);
    TRY_QUERY("lonmin=77., lonmax=76.54320", 4);
    TRY_QUERY("lonmin=77., lonmax=-10", 0);
    TRY_QUERY("lonmin=0., lonmax=360.", 4);
    TRY_QUERY("lonmin=-180., lonmax=180.", 4);
});
this->add_method("mobile", [](Fixture& f) {
    // fixed/mobile queries
    TRY_QUERY("mobile=0", 4);
    TRY_QUERY("mobile=1", 0);
});
// ident queries
// FIXME: we currently have no mobile station data in the samples
//TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
this->add_method("timerange", [](Fixture& f) {
    // timerange queries
    TRY_QUERY("pindicator=20", 4);
    TRY_QUERY("pindicator=21", 0);
    TRY_QUERY("p1=111", 4);
    TRY_QUERY("p1=112", 0);
    TRY_QUERY("p2=121", 0);
    TRY_QUERY("p2=122", 2);
    TRY_QUERY("p2=123", 2);
});
this->add_method("level", [](Fixture& f) {
    // level queries
    TRY_QUERY("leveltype1=10", 4);
    TRY_QUERY("leveltype1=11", 0);
    TRY_QUERY("leveltype2=15", 4);
    TRY_QUERY("leveltype2=16", 0);
    TRY_QUERY("l1=11", 4);
    TRY_QUERY("l1=12", 0);
    TRY_QUERY("l2=22", 4);
    TRY_QUERY("l2=23", 0);
});
this->add_method("varcode", [](Fixture& f) {
    // varcode queries
    TRY_QUERY("var=B01011", 2);
    TRY_QUERY("var=B01012", 2);
    TRY_QUERY("var=B01013", 0);
});
this->add_method("report", [](Fixture& f) {
    // report queries
    TRY_QUERY("rep_memo=synop", 2);
    TRY_QUERY("rep_memo=metar", 2);
    TRY_QUERY("rep_memo=temp", 0);
});
this->add_method("priority", [](Fixture& f) {
    // report priority queries
    TRY_QUERY("priority=101", 2);
    TRY_QUERY("priority=81", 2);
    TRY_QUERY("priority=102", 0);
    TRY_QUERY("priomin=70", 4);
    TRY_QUERY("priomin=80", 4);
    TRY_QUERY("priomin=90", 2);
    TRY_QUERY("priomin=100", 2);
    TRY_QUERY("priomin=110", 0);
    TRY_QUERY("priomax=70", 0);
    TRY_QUERY("priomax=81", 2);
    TRY_QUERY("priomax=100", 2);
    TRY_QUERY("priomax=101", 4);
    TRY_QUERY("priomax=110", 4);
});

}

template<typename DB>
void EmptyFixtureTests<DB>::register_tests()
{

this->add_method("datetime1", [](Fixture& f) {
    // Check datetime queries, with data that only differs by its hour
    DateHourDataSet test_data;
    wassert(f.populate(test_data));

    // Valid hours: 11 and 12

    // Exact match
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 30, 10)), 0));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 30, 11)), 1));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 30, 12)), 1));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 30, 13)), 0));

    // Datemin match
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 30, 10)), 2));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 30, 11)), 2));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 30, 12)), 1));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 30, 13)), 0));

    // Datemax match
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 30, 13)), 2));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 30, 12)), 2));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 30, 11)), 1));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 30, 10)), 0));

    // Date min-max match
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30, 10), Datetime(2013, 10, 30, 13)), 2));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30, 11), Datetime(2013, 10, 30, 12)), 2));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30, 10), Datetime(2013, 10, 30, 11)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30, 12), Datetime(2013, 10, 30, 13)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30,  9), Datetime(2013, 10, 30, 10)), 0));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 30, 13), Datetime(2013, 10, 30, 14)), 0));
});
this->add_method("datetime2", [](Fixture& f) {
    // Check datetime queries, with data that only differs by its day
    DateDayDataSet test_data;
    wassert(f.populate(test_data));

    // Valid days: 23 and 24

    // Exact match
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 22)), 0));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 23)), 1));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 24)), 1));
    wassert(actual(f.tr).try_data_query(query_exact(Datetime(2013, 10, 25)), 0));

    // Datemin match
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 22)), 2));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 23)), 2));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 24)), 1));
    wassert(actual(f.tr).try_data_query(query_min(Datetime(2013, 10, 25)), 0));

    // Datemax match
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 25)), 2));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 24)), 2));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 23)), 1));
    wassert(actual(f.tr).try_data_query(query_max(Datetime(2013, 10, 22)), 0));

    // Date min-max match
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 22), Datetime(2013, 10, 25)), 2));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 23), Datetime(2013, 10, 24)), 2));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 23), Datetime(2013, 10, 23)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 24), Datetime(2013, 10, 24)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 22), Datetime(2013, 10, 23)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 24), Datetime(2013, 10, 25)), 1));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 21), Datetime(2013, 10, 22)), 0));
    wassert(actual(f.tr).try_data_query(query_minmax(Datetime(2013, 10, 25), Datetime(2013, 10, 26)), 0));
});
this->add_method("query_ordering", [](Fixture& f) {
    auto insert = [&](const char* str) {
        core::Record rec;
        rec.set_from_test_string(str);
        DataValues vals(rec);
        wassert(f.tr->insert_data(vals, false, true));
        return vals;
    };
    auto vals01 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=synop, B12101=280.15");
    auto vals02 = insert("lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=synop, B12101=280.15");
    auto vals03 = insert("lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=synop, B12101=280.15");
    auto vals04 = insert("lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=synop, B12101=280.15");
    auto vals05 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=synop, B12101=280.15");
    auto vals06 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=metar, B12101=280.15");
    auto vals07 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=synop, B12103=280.15");

    auto cur = f.tr->query_data(core::Query());
    wassert(actual(cur->remaining()) == 7);

    core::Record test;
    switch (DB::format)
    {
        case V6:
            // v7: coords, ident, datetime, level, trange, report, code
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals01)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals07)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12103=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals06)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=b, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals05)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals04)); // lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals03)); // lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals02)); // lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            break;
        case V7:
            // v7: ana_id(coords, ident, report), datetime, level, trange, code
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals01)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals07)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12103=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals05)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals04)); // lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals03)); // lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals02)); // lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
            wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals06)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=b, B12101=280.15
            break;
        default: error_unimplemented::throwf("cannot run this test on a database of format %d", (int)DB::format);
    }
});
}

}
