#include "db/test-utils-db.h"
#include <wibble/string.h>
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
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

struct DateHourFixture : public TestFixture
{
    DateHourFixture() : TestFixture(2)
    {
        TestStation st;
        st.lat = 12.34560;
        st.lon = 76.54320;

        for (unsigned i = 0; i < 2; ++i)
        {
            records[i].station = st;
            records[i].data.set("rep_memo", "synop");
            records[i].data.set(Level(10, 11, 15, 22));
            records[i].data.set(Trange(20, 111, 122));
            records[i].data.set(Datetime(2013, 10, 30, 11 + i));
            records[i].data.set("B12101", 11.5 + i);
        }
    }
};

struct DateDayFixture : public TestFixture
{
    DateDayFixture() : TestFixture(2)
    {
        TestStation st;
        st.lat = 12.34560;
        st.lon = 76.54320;

        for (unsigned i = 0; i < 2; ++i)
        {
            records[i].station = st;
            records[i].data.set("rep_memo", "synop");
            records[i].data.set(Level(10, 11, 15, 22));
            records[i].data.set(Trange(20, 111, 122));
            records[i].data.set(Datetime(2013, 10, 23 + i));
            records[i].data.set("B12101", 23.5 + i);
        }
    }
};

typedef dballe::tests::DBFixture Fixture;
typedef dballe::tests::db_test_group<Fixture> test_group;
typedef test_group::Test Test;

#define TRY_QUERY(qstring, expected_count) wassert(actual(*f.db).try_data_query(qstring, expected_count))

std::vector<Test> tests {
    Test("ana_id", [](Fixture& f) {
        OldDballeTestFixture oldf;
        wruntest(f.populate_database, oldf);
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
        if (f.db->format() == MEM)
            TRY_QUERY(str::fmtf("ana_id=%d", oldf.dataset0.ana_id), 2);
        else
            TRY_QUERY(str::fmtf("ana_id=%d", oldf.dataset0.ana_id), 4);
        TRY_QUERY("ana_id=4242", 0);
    }),
    Test("ana_context", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // Query data in station context
        core::Query query;
        unique_ptr<db::Cursor> cur = f.db->query_station_data(query);
        ensure_equals(cur->remaining(), 5);
    }),
    Test("year", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
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
    }),
    Test("block_station", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
        const int all = (f.db->format() == MEM ? 2 : 4);
        // Block and station queries
        TRY_QUERY("B01001=1", all);
        TRY_QUERY("B01001=2", 0);
        TRY_QUERY("B01002=52", all);
        TRY_QUERY("B01002=53", 0);
    }),
    Test("ana_filter", [](Fixture& f) {
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
        const int all = (f.db->format() == MEM ? 2 : 4);
        wruntest(f.populate<OldDballeTestFixture>);
        // ana_filter queries
        TRY_QUERY("ana_filter=block=1", all);
        TRY_QUERY("ana_filter=B01001=1", all);
        TRY_QUERY("ana_filter=block>1", 0);
        TRY_QUERY("ana_filter=B01001>1", 0);
        TRY_QUERY("ana_filter=block<=1", all);
        TRY_QUERY("ana_filter=B01001<=1", all);
        TRY_QUERY("ana_filter=0<=B01001<=2", all);
        TRY_QUERY("ana_filter=1<=B01001<=1", all);
        TRY_QUERY("ana_filter=2<=B01001<=4", 0);
    }),
    Test("data_filter", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // data_filter queries
        TRY_QUERY("data_filter=B01011=DB-All.e!", 1);
        TRY_QUERY("data_filter=B01012<300", 0);
        TRY_QUERY("data_filter=B01012<=300", 1);
        TRY_QUERY("data_filter=B01012=300", 1);
        TRY_QUERY("data_filter=B01012>=300", 2);
        TRY_QUERY("data_filter=B01012>300", 1);
        TRY_QUERY("data_filter=B01012<400", 1);
        TRY_QUERY("data_filter=B01012<=400", 2);
    }),
    Test("latlon", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
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
    }),
    Test("mobile", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // fixed/mobile queries
        TRY_QUERY("mobile=0", 4);
        TRY_QUERY("mobile=1", 0);
    }),
    // ident queries
    // FIXME: we currently have no mobile station data in the samples
    //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
    Test("timerange", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // timerange queries
        TRY_QUERY("pindicator=20", 4);
        TRY_QUERY("pindicator=21", 0);
        TRY_QUERY("p1=111", 4);
        TRY_QUERY("p1=112", 0);
        TRY_QUERY("p2=121", 0);
        TRY_QUERY("p2=122", 2);
        TRY_QUERY("p2=123", 2);
    }),
    Test("level", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // level queries
        TRY_QUERY("leveltype1=10", 4);
        TRY_QUERY("leveltype1=11", 0);
        TRY_QUERY("leveltype2=15", 4);
        TRY_QUERY("leveltype2=16", 0);
        TRY_QUERY("l1=11", 4);
        TRY_QUERY("l1=12", 0);
        TRY_QUERY("l2=22", 4);
        TRY_QUERY("l2=23", 0);
    }),
    Test("varcode", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // varcode queries
        TRY_QUERY("var=B01011", 2);
        TRY_QUERY("var=B01012", 2);
        TRY_QUERY("var=B01013", 0);
    }),
    Test("report", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // report queries
        TRY_QUERY("rep_memo=synop", 2);
        TRY_QUERY("rep_memo=metar", 2);
        TRY_QUERY("rep_memo=temp", 0);
    }),
    Test("priority", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
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
    }),
    Test("context_id", [](Fixture& f) {
        wruntest(f.populate<OldDballeTestFixture>);
        // get a valid data id
        core::Query q;
        q.varcodes.insert(WR_VAR(0, 1, 11));
        core::Record res;
        auto cur = f.db->query_data(q);
        while (cur->next())
            cur->to_record(res);
        char valid_query[100];
        snprintf(valid_query, 100, "context_id=%d", res.enq("context_id", MISSING_INT));

        // context ID queries
        TRY_QUERY(valid_query, 1);
        TRY_QUERY("context_id=1234567", 0);
    }),
    Test("datetime1", [](Fixture& f) {
        // Check datetime queries, with data that only differs by its hour
        wruntest(f.populate<DateHourFixture>);
        auto& db = *f.db;

        // Valid hours: 11 and 12

        // Exact match
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 30, 10)), 0));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 30, 11)), 1));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 30, 12)), 1));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 30, 13)), 0));

        // Datemin match
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 30, 10)), 2));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 30, 11)), 2));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 30, 12)), 1));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 30, 13)), 0));

        // Datemax match
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 30, 13)), 2));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 30, 12)), 2));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 30, 11)), 1));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 30, 10)), 0));

        // Date min-max match
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30, 10), Datetime(2013, 10, 30, 13)), 2));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30, 11), Datetime(2013, 10, 30, 12)), 2));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30, 10), Datetime(2013, 10, 30, 11)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30, 12), Datetime(2013, 10, 30, 13)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30,  9), Datetime(2013, 10, 30, 10)), 0));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 30, 13), Datetime(2013, 10, 30, 14)), 0));
    }),
    Test("datetime2", [](Fixture& f) {
        // Check datetime queries, with data that only differs by its day
        wruntest(f.populate<DateDayFixture>);
        auto& db = *f.db;

        // Valid days: 23 and 24

        // Exact match
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 22)), 0));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 23)), 1));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 24)), 1));
        wassert(actual(db).try_data_query(query_exact(Datetime(2013, 10, 25)), 0));

        // Datemin match
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 22)), 2));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 23)), 2));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 24)), 1));
        wassert(actual(db).try_data_query(query_min(Datetime(2013, 10, 25)), 0));

        // Datemax match
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 25)), 2));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 24)), 2));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 23)), 1));
        wassert(actual(db).try_data_query(query_max(Datetime(2013, 10, 22)), 0));

        // Date min-max match
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 22), Datetime(2013, 10, 25)), 2));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 23), Datetime(2013, 10, 24)), 2));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 23), Datetime(2013, 10, 23)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 24), Datetime(2013, 10, 24)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 22), Datetime(2013, 10, 23)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 24), Datetime(2013, 10, 25)), 1));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 21), Datetime(2013, 10, 22)), 0));
        wassert(actual(db).try_data_query(query_minmax(Datetime(2013, 10, 25), Datetime(2013, 10, 26)), 0));
    }),
};

test_group tg1("db_query_data_mem", nullptr, db::MEM, tests);
test_group tg2("db_query_data_v6_sqlite", "SQLITE", db::V6, tests);
#ifdef HAVE_ODBC
test_group tg4("db_query_data_v6_odbc", "ODBC", db::V6, tests);
#endif
#ifdef HAVE_LIBPQ
test_group tg6("db_query_data_v6_postgresql", "POSTGRESQL", db::V6, tests);
#endif
#ifdef HAVE_MYSQL
test_group tg8("db_query_data_v6_mysql", "MYSQL", db::V6, tests);
#endif

}
