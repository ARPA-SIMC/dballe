#include "db/tests.h"
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

#define TRY_QUERY(qstring, expected_count) wassert(actual(*f.db).try_data_query(qstring, expected_count))

class Tests : public FixtureTestCase<DBFixture>
{
    using FixtureTestCase::FixtureTestCase;

    void register_tests() override
    {
        add_method("ana_id", [](Fixture& f) {
            OldDballeTestDataSet oldf;
            wassert(f.populate_database(oldf));
            char query[20];
            snprintf(query, 20, "ana_id=%d", oldf.data["synop"].info.ana_id);
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
            if (f.db->format() != V6)
                TRY_QUERY(query, 2);
            else
                TRY_QUERY(query, 4);
            TRY_QUERY("ana_id=4242", 0);
        });
        add_method("ana_context", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
            // Query data in station context
            core::Query query;
            unique_ptr<db::Cursor> cur = f.db->query_station_data(query);
            wassert(actual(cur->remaining()) == 10);
        });
        add_method("year", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
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
        add_method("block_station", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
            const int all = (f.db->format() == MEM ? 4 : 4);
            // Block and station queries
            TRY_QUERY("B01001=1", all);
            TRY_QUERY("B01001=2", 0);
            TRY_QUERY("B01002=52", all);
            TRY_QUERY("B01002=53", 0);
        });
        add_method("ana_filter", [](Fixture& f) {
#warning FIXME: change after testing if we can move to report-in-station behaviour or not
            const int all = (f.db->format() == MEM ? 4 : 4);
            wassert(f.populate<OldDballeTestDataSet>());
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
        });
        add_method("data_filter", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
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
        add_method("latlon", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
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
        add_method("mobile", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
            // fixed/mobile queries
            TRY_QUERY("mobile=0", 4);
            TRY_QUERY("mobile=1", 0);
        });
        // ident queries
        // FIXME: we currently have no mobile station data in the samples
        //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
        add_method("timerange", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
            // timerange queries
            TRY_QUERY("pindicator=20", 4);
            TRY_QUERY("pindicator=21", 0);
            TRY_QUERY("p1=111", 4);
            TRY_QUERY("p1=112", 0);
            TRY_QUERY("p2=121", 0);
            TRY_QUERY("p2=122", 2);
            TRY_QUERY("p2=123", 2);
        });
        add_method("level", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
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
        add_method("varcode", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
            // varcode queries
            TRY_QUERY("var=B01011", 2);
            TRY_QUERY("var=B01012", 2);
            TRY_QUERY("var=B01013", 0);
        });
        add_method("report", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
            // report queries
            TRY_QUERY("rep_memo=synop", 2);
            TRY_QUERY("rep_memo=metar", 2);
            TRY_QUERY("rep_memo=temp", 0);
        });
        add_method("priority", [](Fixture& f) {
            wassert(f.populate<OldDballeTestDataSet>());
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
        add_method("datetime1", [](Fixture& f) {
            // Check datetime queries, with data that only differs by its hour
            wassert(f.populate<DateHourDataSet>());
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
        });
        add_method("datetime2", [](Fixture& f) {
            // Check datetime queries, with data that only differs by its day
            wassert(f.populate<DateDayDataSet>());
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
        });
        add_method("query_ordering", [](Fixture& f) {
            auto& db = *f.db;
            auto insert = [&](const char* str) {
                core::Record rec;
                rec.set_from_test_string(str);
                DataValues vals(rec);
                db.insert_data(vals, false, true);
                return vals;
            };
            auto vals01 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15");
            auto vals02 = insert("lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15");
            auto vals03 = insert("lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15");
            auto vals04 = insert("lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=a, B12101=280.15");
            auto vals05 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=a, B12101=280.15");
            auto vals06 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=b, B12101=280.15");
            auto vals07 = insert("lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12103=280.15");

            auto cur = db.query_data(core::Query());
            wassert(actual(cur->remaining()) == 7);

            core::Record test;
            switch (db.format())
            {
                case V7:
                case MEM:
                    // mem: coords, ident, datetime, level, trange, code, report
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals01)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals07)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12103=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals06)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=b, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals05)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals04)); // lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals03)); // lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals02)); // lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    break;
                case V6:
                    // V6: ana_id, datetime, level, trange, report, var
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals01)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals07)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12103=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals06)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=b, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals05)); // lat=1, lon=1, year=2000, leveltype1=1, pindicator=2, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals04)); // lat=1, lon=1, year=2000, leveltype1=2, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals03)); // lat=1, lon=1, year=2001, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    wassert(actual(cur->next())); wassert(actual(cur).data_matches(vals02)); // lat=2, lon=1, year=2000, leveltype1=1, pindicator=1, rep_memo=a, B12101=280.15
                    // cur->to_record(test); test.print(stderr);
                    break;
                default: error_unimplemented::throwf("cannot run this test on a database of format %d", (int)db.format());
            }
        });
    }
};

Tests tg1("db_query_data_mem", nullptr, db::MEM);
Tests tg2("db_query_data_v6_sqlite", "SQLITE", db::V6);
#ifdef HAVE_ODBC
Tests tg4("db_query_data_v6_odbc", "ODBC", db::V6);
#endif
#ifdef HAVE_LIBPQ
Tests tg6("db_query_data_v6_postgresql", "POSTGRESQL", db::V6);
#endif
#ifdef HAVE_MYSQL
Tests tg8("db_query_data_v6_mysql", "MYSQL", db::V6);
#endif
Tests tg9("db_query_data_v7_sqlite", "SQLITE", db::V7);

}
