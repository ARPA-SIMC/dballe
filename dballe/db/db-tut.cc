/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "db/test-utils-db.h"
#include "db/querybuf.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace std;

namespace tut {

// Print all the results, returning the count of results printed
static int print_results(db::Cursor& cur)
{
    Record result;
    fprintf(stderr, "%d results:\n", (int)cur.remaining());
    int i;
    for (i = 0; cur.next(); ++i)
    {
        fprintf(stderr, " * Result %d:\n", i);
        cur.to_record(result);
        result.print(stderr);
    }
    return i;
}

struct db_shar : public dballe::tests::DB_test_base
{
    void test_reset();
    void test_insert();
    void test_ana_query();
    void test_misc_queries();
    void test_querybest();
    void test_deletion();
    void test_datetime_queries();
    void test_qc();
    void test_ana_queries();
    void test_vacuum();
    void test_attrs();
    void test_wrap_longitude();
    void test_invalid_sql_querybest();
    void test_ana_filter();
    void test_datetime_extremes();
    void test_bug_querybest();
    void test_bug_query_stations_by_level();
    void test_bug_query_levels_by_station();
    void test_query_stations();
    void test_summary_queries();
    void test_connect_leaks();

    void populate_for_station_queries()
    {
        // Start with an empty database
        db->reset();

        // Insert two stations
        insert.clear();
        insert.set_ana_context();
        insert.set(DBA_KEY_LAT, 12.34560);
        insert.set(DBA_KEY_LON, 76.54320);
        insert.set(DBA_KEY_MOBILE, 0);
        insert.set(DBA_KEY_REP_MEMO, "synop");
        insert.set(WR_VAR(0, 1,  1),  1); // Block
        insert.set(WR_VAR(0, 1,  2),  2); // Station
        insert.set(WR_VAR(0, 7, 30), 42.0); // Height
        db->insert(insert, false, true);
        insert.set(DBA_KEY_REP_MEMO, "metar");
        insert.set(WR_VAR(0, 7, 30), 50.0); // Height
        db->insert(insert, false, true);

        insert.clear();
        insert.set_ana_context();
        insert.set(DBA_KEY_REP_MEMO, "temp");
        insert.set(DBA_KEY_LAT, 23.45670);
        insert.set(DBA_KEY_LON, 65.43210);
        insert.set(DBA_KEY_MOBILE, 0);
        insert.set(WR_VAR(0, 1,  1),   3); // Block
        insert.set(WR_VAR(0, 1,  2),   4); // Station
        insert.set(WR_VAR(0, 7, 30), 100.0); // Height
        db->insert(insert, false, true);
        insert.set(DBA_KEY_REP_MEMO, "metar");
        insert.set(WR_VAR(0, 7, 30), 110.0); // Height
        db->insert(insert, false, true);

        // Insert measured temperatures
        insert.set(DBA_KEY_YEAR, 1945);
        insert.set(DBA_KEY_MONTH, 4);
        insert.set(DBA_KEY_DAY, 25);
        insert.set(DBA_KEY_HOUR, 8);
        insert.set(DBA_KEY_MIN, 0);
        insert.set(DBA_KEY_LEVELTYPE1, 10);
        insert.set(DBA_KEY_L1, 11);
        insert.set(DBA_KEY_LEVELTYPE2, 15);
        insert.set(DBA_KEY_L2, 22);
        insert.set(DBA_KEY_PINDICATOR, 20);
        insert.set(DBA_KEY_P1, 111);
        insert.set(DBA_KEY_P2, 122);

        insert.set(DBA_KEY_ANA_ID, 1);
        insert.set(WR_VAR(0, 12, 101), 290.0);
        db->insert(insert, false, false);

        insert.set(DBA_KEY_ANA_ID, 2);
        insert.set(WR_VAR(0, 12, 101), 300.0);
        db->insert(insert, false, false);
    }

    void populate_for_summary_queries()
    {
        // Start with an empty database
        db->reset();

        // Insert two stations
        insert.clear();
        insert.set_ana_context();
        insert.set(DBA_KEY_LAT, 12.34560);
        insert.set(DBA_KEY_LON, 76.54320);
        insert.set(DBA_KEY_MOBILE, 0);
        insert.set(DBA_KEY_REP_MEMO, "synop");
        insert.set(WR_VAR(0, 1,  1),  1); // Block
        insert.set(WR_VAR(0, 1,  2),  2); // Station
        insert.set(WR_VAR(0, 7, 30), 42.0); // Height
        db->insert(insert, false, true);
        insert.set(DBA_KEY_REP_MEMO, "metar");
        insert.set(WR_VAR(0, 7, 30), 50.0); // Height
        db->insert(insert, false, true);

        insert.clear();
        insert.set_ana_context();
        insert.set(DBA_KEY_REP_MEMO, "temp");
        insert.set(DBA_KEY_LAT, 23.45670);
        insert.set(DBA_KEY_LON, 65.43210);
        insert.set(DBA_KEY_MOBILE, 0);
        insert.set(WR_VAR(0, 1,  1),   3); // Block
        insert.set(WR_VAR(0, 1,  2),   4); // Station
        insert.set(WR_VAR(0, 7, 30), 100.0); // Height
        db->insert(insert, false, true);
        insert.set(DBA_KEY_REP_MEMO, "metar");
        insert.set(WR_VAR(0, 7, 30), 110.0); // Height
        db->insert(insert, false, true);

        // Insert measured temperatures
        insert.set(DBA_KEY_YEAR, 1945);
        insert.set(DBA_KEY_MONTH, 4);
        insert.set(DBA_KEY_DAY, 25);
        insert.set(DBA_KEY_HOUR, 8);
        insert.set(DBA_KEY_MIN, 0);
        insert.set(DBA_KEY_LEVELTYPE1, 10);
        insert.set(DBA_KEY_L1, 11);
        insert.set(DBA_KEY_LEVELTYPE2, 15);
        insert.set(DBA_KEY_L2, 22);
        insert.set(DBA_KEY_PINDICATOR, 20);
        insert.set(DBA_KEY_P1, 111);
        insert.set(DBA_KEY_P2, 122);

        insert.set(DBA_KEY_ANA_ID, 1);
        insert.set(WR_VAR(0, 12, 101), 290.0);
        insert.set(WR_VAR(0, 12, 103), 280.0);
        db->insert(insert, false, false);

        insert.set(DBA_KEY_ANA_ID, 2);
        insert.set(WR_VAR(0, 12, 101), 300.0);
        insert.set(WR_VAR(0, 12, 103), 290.0);
        db->insert(insert, false, false);

        insert.set(DBA_KEY_YEAR, 1945);
        insert.set(DBA_KEY_MONTH, 4);
        insert.set(DBA_KEY_DAY, 26);
        insert.set(DBA_KEY_HOUR, 8);
        insert.set(DBA_KEY_MIN, 0);

        insert.set(DBA_KEY_ANA_ID, 1);
        insert.set(WR_VAR(0, 12, 101), 291.0);
        insert.set(WR_VAR(0, 12, 103), 281.0);
        db->insert(insert, false, false);

        insert.set(DBA_KEY_ANA_ID, 2);
        insert.set(WR_VAR(0, 12, 101), 301.0);
        insert.set(WR_VAR(0, 12, 103), 291.0);
        db->insert(insert, false, false);
    }
};
TESTGRP(db);

template<> template<> void to::test<1>() { use_db(db::V5); test_reset(); }
template<> template<> void to::test<2>() { use_db(db::V6); test_reset(); }
void db_shar::test_reset()
{
    // Run twice to see if it is idempotent
    db->reset();
    db->reset();
}

template<> template<> void to::test<3>() { use_db(db::V5); test_insert(); }
template<> template<> void to::test<4>() { use_db(db::V6); test_insert(); }
void db_shar::test_insert()
{
    // Prepare a record to insert
    insert.clear();
    insert.add(sampleAna);
    insert.add(sampleBase);
    insert.add(sample0);
    insert.add(sample00);
    insert.add(sample01);

    // Check if adding a nonexisting station when not allowed causes an error
    try {
        db->insert(insert, false, false);
        ensure(false);
    } catch (error_consistency& e) {
        ensure_contains(e.what(), "insert a station entry when it is forbidden");
    }

    /* Insert the record */
    db->insert(insert, false, true);
    /* Check if duplicate updates are allowed by insert */
    db->insert(insert, true, false);
    /* Check if duplicate updates are trapped by insert_new */
    try {
        db->insert(insert, false, false);
        ensure(false);
    } catch (wreport::error& e) {
        // ensure_contains(e.what(), "uplicate");
    }
}

template<> template<> void to::test<5>() { use_db(db::V5); test_ana_query(); }
template<> template<> void to::test<6>() { use_db(db::V6); test_ana_query(); }
void db_shar::test_ana_query()
{
    populate_database();

    /*
       CHECKED(dba_ana_count(db, &count));
       fail_unless(count == 1);
       */
    query.clear();

    /* Iterate the anagraphic database */
    auto_ptr<db::Cursor> cur = db->query_stations(query);
    ensure_equals(cur->remaining(), 1);

    /* There should be an item */
    ensure(cur->next());
    cur->to_record(result);

    /* Check that the result matches */
    ensure(result.contains(sampleAna));
    //ensureTestRecEquals(result, extraAna);

    /* There should be only one item */
    ensure_equals(cur->remaining(), 0);

    ensure(!cur->next());
}

template<> template<> void to::test<7>() { use_db(V5); test_misc_queries(); }
template<> template<> void to::test<8>() { use_db(V6); test_misc_queries(); }
void db_shar::test_misc_queries()
{
    populate_database();

/* Try a query using a KEY query parameter */
#define TRY_QUERY(qstring, expected_count) do {\
        query.clear(); \
        query.set_from_string(qstring); \
        auto_ptr<db::Cursor> cur = db->query_data(query); \
        ensure_equals(cur->remaining(), expected_count); \
        int count; \
        if (0) count = print_results(*cur); \
        else for (count = 0; cur->next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)

/* Try a query using a longitude range */
#define TRY_QUERY2(lonmin, lonmax, expected_count) do {\
        query.clear(); \
        query.key(DBA_KEY_LONMIN).setd(lonmin); \
        query.key(DBA_KEY_LONMAX).setd(lonmax); \
        auto_ptr<db::Cursor> cur = db->query_data(query); \
        ensure_equals(cur->remaining(), expected_count); \
        int count; \
        if (0) count = print_results(*cur); \
        else for (count = 0; cur->next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)


    TRY_QUERY("ana_id=1", 4);
    TRY_QUERY("ana_id=2", 0);
    {
        query.clear();
        query.set_ana_context();
        auto_ptr<db::Cursor> cur = db->query_data(query);
        ensure_equals(cur->remaining(), 10);
    }
    //TRY_QUERY(DBA_KEY_YEAR, 1000, 10);
    TRY_QUERY("year=1001", 0);
    TRY_QUERY("yearmin=1999", 0);
    TRY_QUERY("yearmin=1945", 4);
    TRY_QUERY("yearmax=1944", 0);
    TRY_QUERY("yearmax=1945", 4);
    TRY_QUERY("yearmax=2030", 4);
    TRY_QUERY("year=1944", 0);
    TRY_QUERY("year=1945", 4);
    TRY_QUERY("year=1946", 0);
    TRY_QUERY("B01001=1", 4);
    TRY_QUERY("B01001=2", 0);
    TRY_QUERY("B01002=52", 4);
    TRY_QUERY("B01002=53", 0);
    TRY_QUERY("ana_filter=block=1", 4);
    TRY_QUERY("ana_filter=B01001=1", 4);
    TRY_QUERY("ana_filter=block>1", 0);
    TRY_QUERY("ana_filter=B01001>1", 0);
    TRY_QUERY("ana_filter=block<=1", 4);
    TRY_QUERY("ana_filter=B01001<=1", 4);
    TRY_QUERY("ana_filter=0<=B01001<=2", 4);
    TRY_QUERY("ana_filter=1<=B01001<=1", 4);
    TRY_QUERY("ana_filter=2<=B01001<=4", 0);
    TRY_QUERY("data_filter=B01011=DB-All.e!", 4);
    TRY_QUERY("data_filter=B01012<300", 0);
    TRY_QUERY("data_filter=B01012<=300", 4);
    TRY_QUERY("data_filter=B01012=300", 4);
    TRY_QUERY("data_filter=B01012>=300", 4);
    TRY_QUERY("data_filter=B01012>300", 4);
    TRY_QUERY("data_filter=B01012<400", 4);
    TRY_QUERY("data_filter=B01012<=400", 4);

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
    TRY_QUERY("latmin=11.0", 4);
    TRY_QUERY("latmin=12.34560", 4);
    TRY_QUERY("latmin=13.0", 0);
    TRY_QUERY("latmax=11.0", 0);
    TRY_QUERY("latmax=12.34560", 4);
    TRY_QUERY("latmax=13.0", 4);
    TRY_QUERY2(75., 77., 4);
    TRY_QUERY2(76.54320, 76.54320, 4);
    TRY_QUERY2(76.54330, 77., 0);
    TRY_QUERY2(77., 76.54330, 4);
    TRY_QUERY2(77., 76.54320, 4);
    TRY_QUERY2(77., -10, 0);
    TRY_QUERY2(0., 360., 4);
    TRY_QUERY2(-180., 180., 4);
    TRY_QUERY("mobile=0", 4);
    TRY_QUERY("mobile=1", 0);
    //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
    TRY_QUERY("pindicator=20", 4);
    TRY_QUERY("pindicator=21", 0);
    TRY_QUERY("p1=111", 4);
    TRY_QUERY("p1=112", 0);
    TRY_QUERY("p2=121", 0);
    TRY_QUERY("p2=122", 2);
    TRY_QUERY("p2=123", 2);
    TRY_QUERY("leveltype1=10", 4);
    TRY_QUERY("leveltype1=11", 0);
    TRY_QUERY("leveltype2=15", 4);
    TRY_QUERY("leveltype2=16", 0);
    TRY_QUERY("l1=11", 4);
    TRY_QUERY("l1=12", 0);
    TRY_QUERY("l2=22", 4);
    TRY_QUERY("l2=23", 0);
    TRY_QUERY("var=B01011", 2);
    TRY_QUERY("var=B01012", 2);
    TRY_QUERY("var=B01013", 0);
    TRY_QUERY("rep_cod=1", 2);
    TRY_QUERY("rep_cod=2", 2);
    TRY_QUERY("rep_cod=3", 0);
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
    TRY_QUERY("context_id=1", 1);
    TRY_QUERY("context_id=11", 1);

#undef TRY_QUERY
#undef TRY_QUERY2
}

template<> template<> void to::test< 9>() { use_db(V5); test_querybest(); }
template<> template<> void to::test<10>() { use_db(V6); test_querybest(); }
void db_shar::test_querybest()
{
    populate_database();

    //if (db->server_type == ORACLE || db->server_type == POSTGRES)
    //      return;

    // Prepare a query
    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    query.set(DBA_KEY_QUERY, "best");

    // Make the query
    auto_ptr<db::Cursor> cur = db->query_data(query);

    ensure_equals(cur->remaining(), 4);

    // There should be four items
    ensure(cur->next());
    ensure_equals(cur->remaining(), 3);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 2);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 1);
    ensure(cur->next());
    ensure_equals(cur->remaining(), 0);

    // Now there should not be anything anymore
    ensure(!cur->next());
}

template<> template<> void to::test<11>() { use_db(V5); test_deletion(); }
template<> template<> void to::test<12>() { use_db(V6); test_deletion(); }
void db_shar::test_deletion()
{
    populate_database();

    // 4 items to begin with
    query.clear();
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 4);
    cur->discard_rest();

    query.clear();
    query.set(DBA_KEY_YEARMIN, 1945);
    query.set(DBA_KEY_MONTHMIN, 4);
    query.set(DBA_KEY_DAYMIN, 25);
    query.set(DBA_KEY_HOURMIN, 8);
    query.set(DBA_KEY_MINUMIN, 10);
    db->remove(query);

    // 2 remaining after remove
    query.clear();
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    cur->discard_rest();

    // Did it remove the right ones?
    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    ensure(cur->next());
    cur->to_record(result);
    ensure(result.contains(sampleAna));
    ensure(result.contains(sampleBase));

    Varcode last_code = 0;
    for (unsigned i = 0; i < 2; ++i)
    {
        // Check that varcodes repeat
        if (last_code != 0)
            ensure(last_code != result.vars()[0]->code());
        last_code = result.vars()[0]->code();

        switch (last_code)
        {
            case WR_VAR(0, 1, 11):
                ensure(result.contains(sample00));
                break;
            case WR_VAR(0, 1, 12):
                ensure(result.contains(sample01));
                break;
            default:
                ensure(false);
        }

        if (i == 0)
        {
            /* The item should have two data in it */
            ensure(cur->next());
            cur->to_record(result);
        } else {
            ensure(!cur->next());
        }
    }
}

template<> template<> void to::test<13>() { use_db(V5); test_datetime_queries(); }
template<> template<> void to::test<14>() { use_db(V6); test_datetime_queries(); }
void db_shar::test_datetime_queries()
{
    /* Prepare test data */
    Record base, a, b;

    base.set(DBA_KEY_LAT, 12.0);
    base.set(DBA_KEY_LON, 48.0);
    base.set(DBA_KEY_MOBILE, 0);

    /*
       base.set(DBA_KEY_HEIGHT, 42);
       base.set(DBA_KEY_HEIGHTBARO, 234);
       base.set(DBA_KEY_BLOCK, 1);
       base.set(DBA_KEY_STATION, 52);
       base.set(DBA_KEY_NAME, "Cippo Lippo");
       */

    base.set(DBA_KEY_LEVELTYPE1, 1);
    base.set(DBA_KEY_L1, 0);
    base.set(DBA_KEY_LEVELTYPE2, 1);
    base.set(DBA_KEY_L2, 0);
    base.set(DBA_KEY_PINDICATOR, 1);
    base.set(DBA_KEY_P1, 0);
    base.set(DBA_KEY_P2, 0);

    base.set(DBA_KEY_REP_COD, 1);
    base.set(DBA_KEY_PRIORITY, 101);

    base.set(WR_VAR(0, 1, 12), 500);

    base.set(DBA_KEY_YEAR, 2006);
    base.set(DBA_KEY_MONTH, 5);
    base.set(DBA_KEY_DAY, 15);
    base.set(DBA_KEY_HOUR, 12);
    base.set(DBA_KEY_MIN, 30);
    base.set(DBA_KEY_SEC, 0);

#define WANTRESULT(ab) do { \
    auto_ptr<db::Cursor> cur = db->query_data(query); \
    ensure_equals(cur->remaining(), 1); \
    ensure(cur->next()); \
    cur->to_record(result); \
    ensure_equals(cur->remaining(), 0); \
    ensure_varcode_equals(result.vars()[0]->code(), WR_VAR(0, 1, 12)); \
    ensure(result.contains(ab)); \
    cur->discard_rest(); \
} while(0)

    /* Year */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2005);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEARMIN, 2006);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEARMAX, 2005);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    WANTRESULT(b);


    /* Month */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 4);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTHMIN, 5);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTHMAX, 4);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    WANTRESULT(b);

    /* Day */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 2);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAYMIN, 3);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAYMAX, 2);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    WANTRESULT(b);

    /* Hour */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 3);
    a.set(DBA_KEY_HOUR, 12);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    b.set(DBA_KEY_HOUR, 13);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOURMIN, 13);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOURMAX, 12);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 13);
    WANTRESULT(b);

    /* Minute */
    db->reset();

    insert.clear();
    a = base;
    a.set(DBA_KEY_YEAR, 2006);
    a.set(DBA_KEY_MONTH, 5);
    a.set(DBA_KEY_DAY, 3);
    a.set(DBA_KEY_HOUR, 12);
    a.set(DBA_KEY_MIN, 29);
    insert.add(a);
    db->insert(insert, false, true);

    insert.clear();
    b = base;
    b.set(DBA_KEY_YEAR, 2006);
    b.set(DBA_KEY_MONTH, 5);
    b.set(DBA_KEY_DAY, 3);
    b.set(DBA_KEY_HOUR, 12);
    b.set(DBA_KEY_MIN, 30);
    insert.add(b);
    db->insert(insert, false, false);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MINUMIN, 30);
    WANTRESULT(b);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MINUMAX, 29);
    WANTRESULT(a);

    query.clear();
    query.set(DBA_KEY_YEAR, 2006);
    query.set(DBA_KEY_MONTH, 5);
    query.set(DBA_KEY_DAY, 3);
    query.set(DBA_KEY_HOUR, 12);
    query.set(DBA_KEY_MIN, 30);
    WANTRESULT(b);
}

template<> template<> void to::test<15>() { use_db(V5); test_qc(); }
template<> template<> void to::test<16>() { use_db(V6); test_qc(); }
void db_shar::test_qc()
{
    populate_database();

    query.clear();
    query.set(DBA_KEY_LATMIN, 1000000);
    auto_ptr<db::Cursor> cur = db->query_data(query);

    // Move the cursor to B01011
    bool found = false;
    while (cur->next())
    {
        cur->to_record(result);
        if (result.vars()[0]->code() == WR_VAR(0, 1, 11))
        {
            cur->discard_rest();
            found = true;
            break;
        }
    }
    ensure(found);

    int context_id = cur->attr_reference_id();

    // Insert new attributes about this report
    qc.clear();
    qc.set(WR_VAR(0, 33, 2), 2);
    qc.set(WR_VAR(0, 33, 3), 5);
    qc.set(WR_VAR(0, 33, 5), 33);
    db->attr_insert(context_id, WR_VAR(0, 1, 11), qc);

    // Query back the data
    qc.clear();
    vector<Varcode> codes;
    ensure_equals(db->query_attrs(context_id, WR_VAR(0, 1, 11), codes, qc), 3);

    const Var* attr = qc.var_peek(WR_VAR(0, 33, 2));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 2);

    attr = qc.var_peek(WR_VAR(0, 33, 3));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 5);

    attr = qc.var_peek(WR_VAR(0, 33, 5));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 33);

    // Delete a couple of items
    codes.push_back(WR_VAR(0, 33, 2));
    codes.push_back(WR_VAR(0, 33, 5));
    db->attr_remove(context_id, WR_VAR(0, 1, 11), codes);

    // Deleting non-existing items should not fail.  Also try creating a
    // query with just one item
    codes.clear();
    codes.push_back(WR_VAR(0, 33, 2));
    db->attr_remove(context_id, WR_VAR(0, 1, 11), codes);

    /* Query back the data */
    qc.clear();
    codes.clear();
    codes.push_back(WR_VAR(0, 33, 2));
    codes.push_back(WR_VAR(0, 33, 3));
    codes.push_back(WR_VAR(0, 33, 5));
    ensure_equals(db->query_attrs(context_id, WR_VAR(0, 1, 11), codes, qc), 1);

    ensure(qc.var_peek(WR_VAR(0, 33, 2)) == NULL);
    ensure(qc.var_peek(WR_VAR(0, 33, 5)) == NULL);
    attr = qc.var_peek(WR_VAR(0, 33, 3));
    ensure(attr != NULL);
    ensure_equals(attr->enqi(), 5);
    /*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
}

template<> template<> void to::test<17>() { use_db(V5); test_ana_queries(); }
template<> template<> void to::test<18>() { use_db(V6); test_ana_queries(); }
void db_shar::test_ana_queries()
{
    populate_database();

    query.clear();
    query.set(DBA_KEY_REP_COD, 1);

    auto_ptr<db::Cursor> cur = db->query_stations(query);
    ensure_equals(cur->remaining(), 1);

    ensure(cur->next());
    ensure(!cur->next());
}

template<> template<> void to::test<19>() { use_db(V5); test_vacuum(); }
template<> template<> void to::test<20>() { use_db(V6); test_vacuum(); }
void db_shar::test_vacuum()
{
    db->vacuum();
}

template<> template<> void to::test<21>() { use_db(V5); test_attrs(); }
template<> template<> void to::test<22>() { use_db(V6); test_attrs(); }
void db_shar::test_attrs()
{
    // Insert a data record
    insert.clear();
    insert.add(sampleAna);
    insert.add(sampleBase);
    insert.add(sample0);
    insert.add(sample00);
    insert.add(sample01);
    db->insert(insert, false, true);

    qc.clear();
    qc.set(WR_VAR(0,  1,  7),  1);
    qc.set(WR_VAR(0,  2, 48),  2);
    qc.set(WR_VAR(0,  5, 40),  3);
    qc.set(WR_VAR(0,  5, 41),  4);
    qc.set(WR_VAR(0,  5, 43),  5);
    qc.set(WR_VAR(0, 33, 32),  6);
    qc.set(WR_VAR(0,  7, 24),  7);
    qc.set(WR_VAR(0,  5, 21),  8);
    qc.set(WR_VAR(0,  7, 25),  9);
    qc.set(WR_VAR(0,  5, 22), 10);

    db->attr_insert(WR_VAR(0, 1, 11), qc, false);

    qc.clear();
    vector<Varcode> codes;
    int count = db->query_attrs(1, WR_VAR(0, 1, 11), codes, qc);
    ensure_equals(count, 10);

    // Check that all the attributes come out
    const vector<Var*> vars = qc.vars();
    ensure_equals(vars.size(), 10);
    ensure_varcode_equals(vars[0]->code(), WR_VAR(0,   1,  7)); ensure_var_equals(*vars[0],  1);
    ensure_varcode_equals(vars[1]->code(), WR_VAR(0,   2, 48)); ensure_var_equals(*vars[1],  2);
    ensure_varcode_equals(vars[2]->code(), WR_VAR(0,   5, 21)); ensure_var_equals(*vars[2],  8);
    ensure_varcode_equals(vars[3]->code(), WR_VAR(0,   5, 22)); ensure_var_equals(*vars[3], 10);
    ensure_varcode_equals(vars[4]->code(), WR_VAR(0,   5, 40)); ensure_var_equals(*vars[4],  3);
    ensure_varcode_equals(vars[5]->code(), WR_VAR(0,   5, 41)); ensure_var_equals(*vars[5],  4);
    ensure_varcode_equals(vars[6]->code(), WR_VAR(0,   5, 43)); ensure_var_equals(*vars[6],  5);
    ensure_varcode_equals(vars[7]->code(), WR_VAR(0,   7, 24)); ensure_var_equals(*vars[7],  7);
    ensure_varcode_equals(vars[8]->code(), WR_VAR(0,   7, 25)); ensure_var_equals(*vars[8],  9);
    ensure_varcode_equals(vars[9]->code(), WR_VAR(0,  33, 32)); ensure_var_equals(*vars[9],  6);
}

template<> template<> void to::test<23>() { use_db(V5); test_wrap_longitude(); }
template<> template<> void to::test<24>() { use_db(V6); test_wrap_longitude(); }
void db_shar::test_wrap_longitude()
{
    // Insert a data record
    insert.clear();
    insert.add(sampleAna);
    insert.add(sampleBase);
    insert.add(sample0);
    insert.add(sample00);
    insert.add(sample01);
    db->insert(insert, false, true);

    query.clear();
    query.set(DBA_KEY_LATMIN, 10.0);
    query.set(DBA_KEY_LATMAX, 15.0);
    query.set(DBA_KEY_LONMIN, 70.0);
    query.set(DBA_KEY_LONMAX, -160.0);

    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 2);
    cur->discard_rest();
}

template<> template<> void to::test<25>() { use_db(V5); test_invalid_sql_querybest(); }
template<> template<> void to::test<26>() { use_db(V6); test_invalid_sql_querybest(); }
void db_shar::test_invalid_sql_querybest()
{
// Reproduce a querybest scenario which produced invalid SQL
    populate_database();
    // SELECT pa.lat, pa.lon, pa.ident,
    //        d.datetime, d.id_report, d.id_var, d.value,
    //        ri.prio, pa.id, d.id, d.id_lev_tr
    //   FROM data d
    //   JOIN station pa ON d.id_station = pa.id
    //   JOIN repinfo ri ON ri.id=d.id_report
    //  WHERE pa.lat>=? AND pa.lat<=? AND pa.lon>=? AND pa.lon<=? AND pa.ident IS NULL
    //    AND d.datetime=?
    //    AND d.id_var IN (1822)
    //    AND ri.prio=(SELECT MAX(sri.prio) FROM repinfo sri JOIN data sd ON sri.id=sd.id_report WHERE sd.id_station=d.id_station AND sd.id_lev_tr=d.id_lev_tr AND sd.datetime=d.datetime AND sd.id_var=d.id_var)
    //    AND d.id_lev_tr IS NULLORDER BY d.id_station, d.datetime
    Record rec;
    rec.set(DBA_KEY_YEAR,  1000);
    rec.set(DBA_KEY_MONTH,    1);
    rec.set(DBA_KEY_DAY,      1);
    rec.set(DBA_KEY_HOUR,     0);
    rec.set(DBA_KEY_MIN,     0);
    rec.set(DBA_KEY_QUERY,    "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    while (cur->next())
    {
    }
}

template<> template<> void to::test<27>() { use_db(V5); test_ana_filter(); }
template<> template<> void to::test<28>() { use_db(V6); test_ana_filter(); }
void db_shar::test_ana_filter()
{
    // Test numeric comparisons in ana_filter
    populate_database();

    query.clear();
    query.set(DBA_KEY_REP_COD, 2);
    query.set(DBA_KEY_VAR, "B01011");
    auto_ptr<db::Cursor> cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);

    // Move the cursor to B01011
    cur->next();
    int context_id = cur->attr_reference_id();
    cur->discard_rest();

    // Insert new attributes about this report
    qc.clear();
    qc.set(WR_VAR(0, 1, 1), 50);
    qc.set(WR_VAR(0, 1, 8), "50");
    db->attr_insert(context_id, WR_VAR(0, 1, 11), qc);

    // Try queries filtered by numeric attributes
    query.clear();
    query.set(DBA_KEY_REP_COD, 2);
    query.set(DBA_KEY_VAR, "B01011");

    query.set(DBA_KEY_ATTR_FILTER, "B01001=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<51");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01001<8");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 0);
    cur->discard_rest();

    // Try queries filtered by string attributes
    query.clear();
    query.set(DBA_KEY_REP_COD, 2);
    query.set(DBA_KEY_VAR, "B01011");

    query.set(DBA_KEY_ATTR_FILTER, "B01008=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<=50");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<8");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 1);
    cur->discard_rest();

    query.set(DBA_KEY_ATTR_FILTER, "B01008<100");
    cur = db->query_data(query);
    ensure_equals(cur->remaining(), 0);
    cur->discard_rest();
}

template<> template<> void to::test<29>() { use_db(V5); test_datetime_extremes(); }
template<> template<> void to::test<30>() { use_db(V6); test_datetime_extremes(); }
void db_shar::test_datetime_extremes()
{
#warning temporary disabled
#if 0
    populate_database();

    // All DB
    query.clear();
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), 1945);
    ensure_equals(result.get("monthmin", -1),    4);
    ensure_equals(result.get("daymin",   -1),   25);
    ensure_equals(result.get("hourmin",  -1),    8);
    ensure_equals(result.get("minumin",  -1),    0);
    ensure_equals(result.get("secmin",   -1),    0);

    ensure_equals(result.get("yearmax",  -1), 1945);
    ensure_equals(result.get("monthmax", -1),    4);
    ensure_equals(result.get("daymax",   -1),   25);
    ensure_equals(result.get("hourmax",  -1),    8);
    ensure_equals(result.get("minumax",  -1),   30);
    ensure_equals(result.get("secmax",   -1),    0);

    // Subset of the DB
    query.clear();
    query.set("pindicator", 20);
    query.set("p1", 111);
    query.set("p2", 122);
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), 1945);
    ensure_equals(result.get("monthmin", -1),    4);
    ensure_equals(result.get("daymin",   -1),   25);
    ensure_equals(result.get("hourmin",  -1),    8);
    ensure_equals(result.get("minumin",  -1),    0);
    ensure_equals(result.get("secmin",   -1),    0);

    ensure_equals(result.get("yearmax",  -1), 1945);
    ensure_equals(result.get("monthmax", -1),    4);
    ensure_equals(result.get("daymax",   -1),   25);
    ensure_equals(result.get("hourmax",  -1),    8);
    ensure_equals(result.get("minumax",  -1),    0);
    ensure_equals(result.get("secmax",   -1),    0);

    // No matches
    query.clear();
    query.set("pindicator", 1);
    db->query_datetime_extremes(query, result);

    ensure_equals(result.get("yearmin",  -1), -1);
    ensure_equals(result.get("monthmin", -1), -1);
    ensure_equals(result.get("daymin",   -1), -1);
    ensure_equals(result.get("hourmin",  -1), -1);
    ensure_equals(result.get("minumin",  -1), -1);
    ensure_equals(result.get("secmin",   -1), -1);

    ensure_equals(result.get("yearmax",  -1), -1);
    ensure_equals(result.get("monthmax", -1), -1);
    ensure_equals(result.get("daymax",   -1), -1);
    ensure_equals(result.get("hourmax",  -1), -1);
    ensure_equals(result.get("minumax",  -1), -1);
    ensure_equals(result.get("secmax",   -1), -1);
#endif
}

template<> template<> void to::test<31>() { use_db(V5); test_bug_querybest(); }
template<> template<> void to::test<32>() { use_db(V6); test_bug_querybest(); }
void db_shar::test_bug_querybest()
{
    // Reproduce a querybest scenario which produced always the same data record

    // Import lots
    const char** files = dballe::tests::bufr_files;
    for (int i = 0; files[i] != NULL; i++)
    {
        std::auto_ptr<Msgs> inmsgs = read_msgs(files[i], BUFR);
        Msg& msg = *(*inmsgs)[0];
        db->import_msg(msg, NULL, DBA_IMPORT_ATTRS | DBA_IMPORT_FULL_PSEUDOANA | DBA_IMPORT_DATETIME_ATTRS | DBA_IMPORT_OVERWRITE);
    }

    // Query all with best
    Record rec;
    rec.set(DBA_KEY_VAR,   "B12101");
    rec.set(DBA_KEY_QUERY, "best");
    auto_ptr<db::Cursor> cur = db->query_data(rec);
    unsigned orig_count = cur->remaining();
    unsigned count = 0;
    int id_data = 0;
    unsigned id_data_changes = 0;
    while (cur->next())
    {
        ++count;
        if (cur->attr_reference_id() != id_data)
        {
            id_data = cur->attr_reference_id();
            ++id_data_changes;
        }
    }

    ensure(count > 1);
    ensure_equals(id_data_changes, count);
    ensure_equals(count, orig_count);
}

template<> template<> void to::test<33>() { use_db(V5); test_bug_query_stations_by_level(); }
template<> template<> void to::test<34>() { use_db(V6); test_bug_query_stations_by_level(); }
void db_shar::test_bug_query_stations_by_level()
{
    // Reproduce a query that generated invalid SQL on V6
    populate_database();

    // All DB
    query.clear();
    query.set(DBA_KEY_LEVELTYPE1, 103);
    query.set(DBA_KEY_L1, 2000);
    db->query_stations(query);
}

template<> template<> void to::test<35>() { use_db(V5); test_bug_query_levels_by_station(); }
template<> template<> void to::test<36>() { use_db(V6); test_bug_query_levels_by_station(); }
void db_shar::test_bug_query_levels_by_station()
{
    // Reproduce a query that generated invalid SQL on V6
    populate_database();

    // All DB
    query.clear();
    query.set(DBA_KEY_ANA_ID, 1);
    //db->query_levels(query);
    //db->query_tranges(query);
#warning currently disabled
}

template<> template<> void to::test<37>() { use_db(V5); test_query_stations(); }
template<> template<> void to::test<38>() { use_db(V6); test_query_stations(); }
void db_shar::test_query_stations()
{
/* Try a query using a KEY query parameter */
#define TRY_QUERY(param, value, expected_count) do {\
        query.clear(); \
        query.set(param, value); \
        auto_ptr<db::Cursor> cur = db->query_stations(query); \
        ensure_equals(cur->remaining(), expected_count); \
        int count; \
        if (0) count = print_results(*cur); \
        else for (count = 0; cur->next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)

/* Try a query using a longitude range */
#define TRY_QUERY2(lonmin, lonmax, expected_count) do {\
        query.clear(); \
        query.key(DBA_KEY_LONMIN).setd(lonmin); \
        query.key(DBA_KEY_LONMAX).setd(lonmax); \
        auto_ptr<db::Cursor> cur = db->query_stations(query); \
        ensure_equals(cur->remaining(), expected_count); \
        int count; \
        if (0) count = print_results(*cur); \
        else for (count = 0; cur->next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)

    populate_for_station_queries();

    TRY_QUERY(DBA_KEY_ANA_ID, "1", 1);
    TRY_QUERY(DBA_KEY_ANA_ID, "2", 1);
    TRY_QUERY(DBA_KEY_LAT, 12.00000, 0);
    TRY_QUERY(DBA_KEY_LAT, 12.34560, 1);
    TRY_QUERY(DBA_KEY_LAT, 23.45670, 1);
    TRY_QUERY(DBA_KEY_LATMIN, 12.00000, 2);
    TRY_QUERY(DBA_KEY_LATMIN, 12.34560, 2);
    TRY_QUERY(DBA_KEY_LATMIN, 12.34570, 1);
    TRY_QUERY(DBA_KEY_LATMIN, 23.45670, 1);
    TRY_QUERY(DBA_KEY_LATMIN, 23.45680, 0);
    TRY_QUERY(DBA_KEY_LATMAX, 12.00000, 0);
    TRY_QUERY(DBA_KEY_LATMAX, 12.34560, 1);
    TRY_QUERY(DBA_KEY_LATMAX, 12.34570, 1);
    TRY_QUERY(DBA_KEY_LATMAX, 23.45670, 2);
    TRY_QUERY(DBA_KEY_LATMAX, 23.45680, 2);
    TRY_QUERY(DBA_KEY_LON, 76.00000, 0);
    TRY_QUERY(DBA_KEY_LON, 76.54320, 1);
    TRY_QUERY(DBA_KEY_LON, 65.43210, 1);
    TRY_QUERY2(10., 20., 0);
    TRY_QUERY2(76.54320, 76.54320, 1);
    TRY_QUERY2(76.54320, 77., 1);
    TRY_QUERY2(76.54330, 77., 0);
    TRY_QUERY2(60., 77., 2);
    TRY_QUERY2(77., 76.54310, 1);
    TRY_QUERY2(77., 76.54320, 2);
    TRY_QUERY2(77., -10, 0);
    TRY_QUERY(DBA_KEY_MOBILE, 0, 2);
    TRY_QUERY(DBA_KEY_MOBILE, 1, 0);
    TRY_QUERY(WR_VAR(0, 1, 1), 1, 1);
    TRY_QUERY(WR_VAR(0, 1, 1), 2, 0);
    TRY_QUERY(WR_VAR(0, 1, 1), 3, 1);
    TRY_QUERY(WR_VAR(0, 1, 1), 4, 0);
    TRY_QUERY(WR_VAR(0, 1, 2), 1, 0);
    TRY_QUERY(WR_VAR(0, 1, 2), 2, 1);
    TRY_QUERY(WR_VAR(0, 1, 2), 3, 0);
    TRY_QUERY(WR_VAR(0, 1, 2), 4, 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block=1", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block=2", 0);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block=3", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block>=1", 2);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030=42", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030=50", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030=100", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030=110", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030=120", 0);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030>50", 1);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B07030>=50", 2);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "50<=B07030<=100", 2);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101=290", 1);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101=300", 1);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<300", 1);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<=300", 2);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101>=300", 1);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101>300", 0);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<400", 2);
#undef TRY_QUERY
#undef TRY_QUERY2
}

template<> template<> void to::test<39>() { use_db(V5); test_summary_queries(); }
template<> template<> void to::test<40>() { use_db(V6); test_summary_queries(); }
void db_shar::test_summary_queries()
{
    populate_for_summary_queries();

/* Try a query using a KEY query parameter */
#define TRY_QUERY(param, value, expected_count) do {\
        query.clear(); \
        query.set(param, value); \
        auto_ptr<db::Cursor> cur = db->query_summary(query); \
        ensure_equals(cur->remaining(), 0); \
        int count = cur->test_iterate(); \
        ensure_equals(count, expected_count); \
} while (0)

/* Try a query using a longitude range */
#define TRY_QUERY2(lonmin, lonmax, expected_count) do {\
        query.clear(); \
        query.key(DBA_KEY_LONMIN).setd(lonmin); \
        query.key(DBA_KEY_LONMAX).setd(lonmax); \
        auto_ptr<db::Cursor> cur = db->query_summary(query); \
        ensure_equals(cur->remaining(), 0); \
        int count = cur->test_iterate(); \
        ensure_equals(count, expected_count); \
} while (0)

    TRY_QUERY(DBA_KEY_ANA_ID, 1, 5);
    TRY_QUERY(DBA_KEY_ANA_ID, 2, 5);
    TRY_QUERY(DBA_KEY_ANA_ID, 3, 0);
    {
        query.clear();
        query.set_ana_context();
        auto_ptr<db::Cursor> cur = db->query_summary(query);
        ensure_equals(cur->test_iterate(), 12);
    }
    //TRY_QUERY(DBA_KEY_YEAR, 1000, 10);
    TRY_QUERY(DBA_KEY_YEAR, 1001, 0);
    TRY_QUERY(DBA_KEY_YEARMIN, 1999, 0);
    TRY_QUERY(DBA_KEY_YEARMIN, 1945, 10);
    TRY_QUERY(DBA_KEY_YEARMAX, 1944, 0);
    TRY_QUERY(DBA_KEY_YEARMAX, 1945, 10);
    TRY_QUERY(DBA_KEY_YEARMAX, 2030, 10);
    TRY_QUERY(DBA_KEY_YEAR, 1944, 0);
    TRY_QUERY(DBA_KEY_YEAR, 1945, 10);
    TRY_QUERY(DBA_KEY_YEAR, 1946, 0);
    TRY_QUERY(WR_VAR(0, 1, 1), 1, 5);
    TRY_QUERY(WR_VAR(0, 1, 1), 2, 0);
    TRY_QUERY(WR_VAR(0, 1, 2), 3, 0);
    TRY_QUERY(WR_VAR(0, 1, 2), 4, 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block=1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001=1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block>1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001>1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "block<=1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001>3", 0);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001>=3", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001<=1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "0<=B01001<=2", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "1<=B01001<=1", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "2<=B01001<=4", 5);
    TRY_QUERY(DBA_KEY_ANA_FILTER, "4<=B01001<=6", 0);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<300.0", 5);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<=300.0", 10);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101=300.0", 5);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101>=300,0", 5);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101>300.0", 5);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<400.0", 10);
    TRY_QUERY(DBA_KEY_DATA_FILTER, "B12101<=400.0", 10);

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
    TRY_QUERY(DBA_KEY_LATMIN, 11.0, 10);
    TRY_QUERY(DBA_KEY_LATMIN, 12.34560, 10);
    TRY_QUERY(DBA_KEY_LATMIN, 13.0, 5);
    TRY_QUERY(DBA_KEY_LATMAX, 11.0, 0);
    TRY_QUERY(DBA_KEY_LATMAX, 12.34560, 5);
    TRY_QUERY(DBA_KEY_LATMAX, 13.0, 5);
    TRY_QUERY2(75., 77., 5);
    TRY_QUERY2(76.54320, 76.54320, 5);
    TRY_QUERY2(76.54330, 77., 0);
    TRY_QUERY2(77., 76.54310, 5);
    TRY_QUERY2(77., 76.54320, 10);
    TRY_QUERY2(77., -10, 0);
    TRY_QUERY(DBA_KEY_MOBILE, 0, 10);
    TRY_QUERY(DBA_KEY_MOBILE, 1, 0);
    //TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
    TRY_QUERY(DBA_KEY_PINDICATOR, 20, 10);
    TRY_QUERY(DBA_KEY_PINDICATOR, 21, 0);
    TRY_QUERY(DBA_KEY_P1, 111, 10);
    TRY_QUERY(DBA_KEY_P1, 112, 0);
    TRY_QUERY(DBA_KEY_P2, 121, 0);
    TRY_QUERY(DBA_KEY_P2, 122, 10);
    TRY_QUERY(DBA_KEY_P2, 123, 0);
    TRY_QUERY(DBA_KEY_LEVELTYPE1, 10, 10);
    TRY_QUERY(DBA_KEY_LEVELTYPE1, 11, 0);
    TRY_QUERY(DBA_KEY_LEVELTYPE2, 15, 10);
    TRY_QUERY(DBA_KEY_LEVELTYPE2, 16, 0);
    TRY_QUERY(DBA_KEY_L1, 11, 10);
    TRY_QUERY(DBA_KEY_L1, 12, 0);
    TRY_QUERY(DBA_KEY_L2, 22, 10);
    TRY_QUERY(DBA_KEY_L2, 23, 0);
    TRY_QUERY(DBA_KEY_VAR, "B01001", 2);
    TRY_QUERY(DBA_KEY_VAR, "B12101", 2);
    TRY_QUERY(DBA_KEY_VAR, "B12102", 0);
    TRY_QUERY(DBA_KEY_REP_COD, 1, 0);
    TRY_QUERY(DBA_KEY_REP_COD, 2, 10);
    TRY_QUERY(DBA_KEY_REP_COD, 3, 0);
    TRY_QUERY(DBA_KEY_PRIORITY, 101, 0);
    TRY_QUERY(DBA_KEY_PRIORITY, 81, 10);
    TRY_QUERY(DBA_KEY_PRIORITY, 102, 0);
    TRY_QUERY(DBA_KEY_PRIOMIN, 70, 10);
    TRY_QUERY(DBA_KEY_PRIOMIN, 80, 10);
    TRY_QUERY(DBA_KEY_PRIOMIN, 90, 0);
    TRY_QUERY(DBA_KEY_PRIOMAX, 70, 0);
    TRY_QUERY(DBA_KEY_PRIOMAX, 81, 10);
    TRY_QUERY(DBA_KEY_PRIOMAX, 100, 10);
    TRY_QUERY(DBA_KEY_CONTEXT_ID, 1, 1);
    TRY_QUERY(DBA_KEY_CONTEXT_ID, 11, 1);
#undef TRY_QUERY
#undef TRY_QUERY2
}

template<> template<> void to::test<41>() { use_db(V5); test_connect_leaks(); }
template<> template<> void to::test<42>() { use_db(V6); test_connect_leaks(); }
void db_shar::test_connect_leaks()
{
    insert.clear();
    insert.set_ana_context();
    insert.set(DBA_KEY_LAT, 12.34560);
    insert.set(DBA_KEY_LON, 76.54320);
    insert.set(DBA_KEY_MOBILE, 0);
    insert.set(DBA_KEY_REP_MEMO, "synop");
    insert.set(WR_VAR(0, 7, 30), 42.0); // Height

    // Assume a max open file limit of 1100
    for (unsigned i = 0; i < 1100; ++i)
    {
        std::auto_ptr<DB> db = DB::connect_test();
        db->insert(insert, false, true);
    }
}

}

/* vim:set ts=4 sw=4: */

