/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <test-utils-db.h>
#include <dballe/db/querybuf.h>
#include <dballe/db/db.h>
#include <dballe/db/cursor.h>
#include <dballe/db/internals.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace tut {

// Print all the results, returning the count of results printed
static int print_results(db::Cursor& cur)
{
        Record result;
        fprintf(stderr, "Query: %s\n%d results:\n", cur.sql_query.c_str(), (int)cur.count);
        int i;
        for (i = 0; cur.next(); ++i)
        {
                fprintf(stderr, " * Result %d:\n", i);
                cur.to_record(result);
                result.print(stderr);
        }
        return i;
}

struct db_shar : public dballe::tests::db_test
{
	// Records with test data
	Record sampleAna;
	Record extraAna;
	Record sampleBase;
	Record sample0;
	Record sample00;
	Record sample01;
	Record sample1;
	Record sample10;
	Record sample11;

	// Work records
	Record insert;
	Record query;
	Record result;
	Record qc;

	db_shar()
//		: insert(NULL), query(NULL), result(NULL), qc(NULL)
	{
		if (!has_db()) return;

		// Common data (ana)
		sampleAna.set(DBA_KEY_LAT, 12.34560);
		sampleAna.set(DBA_KEY_LON, 76.54320);
		sampleAna.set(DBA_KEY_MOBILE, 0);

		// Extra ana info
		extraAna.set(WR_VAR(0, 7,  1), 42);		// Height
		extraAna.set(WR_VAR(0, 7, 31), 234);		// Heightbaro
		extraAna.set(WR_VAR(0, 1,  1), 1);			// Block
		extraAna.set(WR_VAR(0, 1,  2), 52);		// Station
		extraAna.set(WR_VAR(0, 1, 19), "Cippo Lippo");	// Name

		// Common data
		sampleBase.set(DBA_KEY_YEAR, 1945);
		sampleBase.set(DBA_KEY_MONTH, 4);
		sampleBase.set(DBA_KEY_DAY, 25);
		sampleBase.set(DBA_KEY_HOUR, 8);
		sampleBase.set(DBA_KEY_LEVELTYPE1, 10);
		sampleBase.set(DBA_KEY_L1, 11);
		sampleBase.set(DBA_KEY_LEVELTYPE2, 15);
		sampleBase.set(DBA_KEY_L2, 22);
		sampleBase.set(DBA_KEY_PINDICATOR, 20);
		sampleBase.set(DBA_KEY_P1, 111);

		// Specific data
		sample0.set(DBA_KEY_MIN, 0);
		sample0.set(DBA_KEY_P2, 122);
		sample0.set(DBA_KEY_REP_COD, 1);
		sample0.set(DBA_KEY_PRIORITY, 101);

		sample00.set(WR_VAR(0, 1, 11), "DB-All.e!");
		sample01.set(WR_VAR(0, 1, 12), 300);

		sample1.set(DBA_KEY_MIN, 30);
		sample1.set(DBA_KEY_P2, 123);
		sample1.set(DBA_KEY_REP_COD, 2);
		sample1.set(DBA_KEY_PRIORITY, 81);

		sample10.set(WR_VAR(0, 1, 11), "Arpa-Sim!");
		sample11.set(WR_VAR(0, 1, 12), 400);

		/*
static struct test_data tdata3_patch[] = {
	{ "mobile", "1" },
	{ "ident", "Cippo" },
};
		*/
	}

	~db_shar()
	{
	}

	void populate_database();
};
TESTGRP(db);

void db_shar::populate_database()
{
        /* Start with an empty database */
        db->reset();

        /* Insert the ana station */
        insert.clear();
        insert.set_ana_context();
        insert.key(DBA_KEY_REP_MEMO).setc("synop");
        insert.add(sampleAna);
        insert.add(extraAna);
        /* Insert the anagraphical record */
        db->insert(insert, false, true);

        /* Insert the ana info also for rep_cod 2 */
        insert.key(DBA_KEY_REP_MEMO).setc("metar");
        insert.unset(DBA_KEY_CONTEXT_ID);
        db->insert(insert, false, true);

        // Insert a record
        insert.clear();
        insert.add(sampleAna);
        insert.add(sampleBase);
        insert.add(sample0);
        insert.add(sample00);
        insert.add(sample01);
        db->insert(insert, false, false);

        // Insert another record (similar but not the same)
        insert.clear();
        insert.add(sampleAna);
        insert.add(sampleBase);
        insert.add(sample1);
        insert.add(sample10);
        insert.add(sample11);
        db->insert(insert, false, false);
}

// Ensure that reset will work on an empty database
template<> template<>
void to::test<1>()
{
	use_db();

	db->delete_tables();
	db->reset();
	// Run twice to see if it is idempotent
	db->reset();
}

// Test insert
template<> template<>
void to::test<2>()
{
        use_db();

        db->reset();

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
        } catch (db::error_odbc& e) {
                ensure_contains(e.what(), "uplicate");
        }

}

// Test ana_query
template<> template<>
void to::test<3>()
{
        use_db();

        populate_database();

        db::Cursor cursor(*db);

        /*
        CHECKED(dba_ana_count(db, &count));
        fail_unless(count == 1);
        */
        query.clear();

        /* Iterate the anagraphic database */
        ensure_equals(cursor.query_stations(query), 1);

        /* There should be an item */
        ensure(cursor.next());
        cursor.to_record(result);

        /* Check that the result matches */
        ensure(result.contains(sampleAna));
        //ensureTestRecEquals(result, extraAna);

        /* There should be only one item */
        ensure_equals(cursor.remaining(), 0);

        ensure(!cursor.next());
}

// Try many possible queries
template<> template<>
void to::test<4>()
{
        use_db();

        populate_database();

/* Try a query using a KEY query parameter */
#define TRY_QUERY(param, value, expected_count) do {\
        query.clear(); \
        db::Cursor cursor(*db); \
        query.set(param, value); \
        ensure_equals(cursor.query_data(query), expected_count); \
        int count; \
        if (0) count = print_results(cursor); \
        else for (count = 0; cursor.next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)

/* Try a query using a longitude range */
#define TRY_QUERY2(lonmin, lonmax, expected_count) do {\
        query.clear(); \
        db::Cursor cursor(*db); \
        query.key(DBA_KEY_LONMIN).setd(lonmin); \
        query.key(DBA_KEY_LONMAX).setd(lonmax); \
        ensure_equals(cursor.query_data(query), expected_count); \
        int count; \
        if (0) count = print_results(cursor); \
        else for (count = 0; cursor.next(); ++count) ; \
        ensure_equals(count, expected_count); \
} while (0)


	TRY_QUERY(DBA_KEY_ANA_ID, "1", 4);
	TRY_QUERY(DBA_KEY_ANA_ID, "2", 0);
	TRY_QUERY(DBA_KEY_YEAR, 1000, 10);
	TRY_QUERY(DBA_KEY_YEAR, 1001, 0);
	TRY_QUERY(DBA_KEY_YEARMIN, 1999, 0);
	TRY_QUERY(DBA_KEY_YEARMIN, 1945, 4);
	TRY_QUERY(DBA_KEY_YEARMAX, 1944, 0);
	TRY_QUERY(DBA_KEY_YEARMAX, 1945, 4);
	TRY_QUERY(DBA_KEY_YEARMAX, 2030, 4);
	TRY_QUERY(DBA_KEY_YEAR, 1944, 0);
	TRY_QUERY(DBA_KEY_YEAR, 1945, 4);
	TRY_QUERY(DBA_KEY_YEAR, 1946, 0);
	TRY_QUERY(WR_VAR(0, 1, 1), 1, 4);
	TRY_QUERY(WR_VAR(0, 1, 1), 2, 0);
	TRY_QUERY(WR_VAR(0, 1, 2), 52, 4);
	TRY_QUERY(WR_VAR(0, 1, 2), 53, 0);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "block=1", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001=1", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "block>1", 0);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001>1", 0);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "block<=1", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "B01001<=1", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "0<=B01001<=2", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "1<=B01001<=1", 4);
	TRY_QUERY(DBA_KEY_ANA_FILTER, "2<=B01001<=4", 0);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01011=DB-All.e!", 2);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01012=300", 2);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01012>=300", 4);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01012>300", 2);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01012<400", 2);
	TRY_QUERY(DBA_KEY_DATA_FILTER, "B01012<=400", 4);

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
	TRY_QUERY(DBA_KEY_LATMIN, 11.0, 4);
	TRY_QUERY(DBA_KEY_LATMIN, 12.34560, 4);
	TRY_QUERY(DBA_KEY_LATMIN, 13.0, 0);
	TRY_QUERY(DBA_KEY_LATMAX, 11.0, 0);
	TRY_QUERY(DBA_KEY_LATMAX, 12.34560, 4);
	TRY_QUERY(DBA_KEY_LATMAX, 13.0, 4);
	TRY_QUERY2(75., 77., 4);
	TRY_QUERY2(76.54320, 76.54320, 4);
	TRY_QUERY2(76.54330, 77., 0);
	TRY_QUERY2(77., 76.54330, 4);
	TRY_QUERY2(77., 76.54320, 4);
	TRY_QUERY2(77., -10, 0);
	TRY_QUERY(DBA_KEY_MOBILE, 0, 4);
	TRY_QUERY(DBA_KEY_MOBILE, 1, 0);
	//TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
	TRY_QUERY(DBA_KEY_PINDICATOR, 20, 4);
	TRY_QUERY(DBA_KEY_PINDICATOR, 21, 0);
	TRY_QUERY(DBA_KEY_P1, 111, 4);
	TRY_QUERY(DBA_KEY_P1, 112, 0);
	TRY_QUERY(DBA_KEY_P2, 121, 0);
	TRY_QUERY(DBA_KEY_P2, 122, 2);
	TRY_QUERY(DBA_KEY_P2, 123, 2);
	TRY_QUERY(DBA_KEY_LEVELTYPE1, 10, 4);
	TRY_QUERY(DBA_KEY_LEVELTYPE1, 11, 0);
	TRY_QUERY(DBA_KEY_LEVELTYPE2, 15, 4);
	TRY_QUERY(DBA_KEY_LEVELTYPE2, 16, 0);
	TRY_QUERY(DBA_KEY_L1, 11, 4);
	TRY_QUERY(DBA_KEY_L1, 12, 0);
	TRY_QUERY(DBA_KEY_L2, 22, 4);
	TRY_QUERY(DBA_KEY_L2, 23, 0);
	TRY_QUERY(DBA_KEY_VAR, "B01011", 2);
	TRY_QUERY(DBA_KEY_VAR, "B01012", 2);
	TRY_QUERY(DBA_KEY_VAR, "B01013", 0);
	TRY_QUERY(DBA_KEY_REP_COD, 1, 2);
	TRY_QUERY(DBA_KEY_REP_COD, 2, 2);
	TRY_QUERY(DBA_KEY_REP_COD, 3, 0);
	TRY_QUERY(DBA_KEY_PRIORITY, 101, 2);
	TRY_QUERY(DBA_KEY_PRIORITY, 81, 2);
	TRY_QUERY(DBA_KEY_PRIORITY, 102, 0);
	TRY_QUERY(DBA_KEY_PRIOMIN, 70, 4);
	TRY_QUERY(DBA_KEY_PRIOMIN, 80, 4);
	TRY_QUERY(DBA_KEY_PRIOMIN, 90, 2);
	TRY_QUERY(DBA_KEY_PRIOMIN, 100, 2);
	TRY_QUERY(DBA_KEY_PRIOMIN, 110, 0);
	TRY_QUERY(DBA_KEY_PRIOMAX, 70, 0);
	TRY_QUERY(DBA_KEY_PRIOMAX, 81, 2);
	TRY_QUERY(DBA_KEY_PRIOMAX, 100, 2);
	TRY_QUERY(DBA_KEY_PRIOMAX, 101, 4);
	TRY_QUERY(DBA_KEY_PRIOMAX, 110, 4);
}

// Try a query checking all the steps
template<> template<>
void to::test<5>()
{
        use_db();
        populate_database();

        db::Cursor cursor(*db);

        // Prepare a query
        query.clear();
        query.set(DBA_KEY_LATMIN, 10.0);

        // Make the query
        ensure_equals(cursor.query_data(query), 4);

        // There should be at least one item
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 3);
        cursor.to_record(result);

        /* Check that the results match */
        ensure(result.contains(sampleAna));
        ensure(result.contains(sampleBase));
        ensure(result.contains(sample0));

        // result.print(stderr);
        // exit(0);

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample00));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample01));

        // The item should have two data in it
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 2);
        cursor.to_record(result);

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample00));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample01));

        // There should be also another item
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 1);
        cursor.to_record(result);

        // Check that the results matches
        ensure(result.contains(sampleAna));
        ensure(result.contains(sampleBase));
        ensure(result.contains(sample1));

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample10));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample11));

        // And finally the last item
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 0);
        cursor.to_record(result);

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample10));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample11));

        // Now there should not be anything anymore
        ensure_equals(cursor.remaining(), 0);
        ensure(!cursor.next());
}

// Try a query for best value
template<> template<>
void to::test<6>()
{
        use_db();
        populate_database();

        //if (db->server_type == ORACLE || db->server_type == POSTGRES)
        //      return;

        db::Cursor cursor(*db);


        // Prepare a query
        query.clear();
        query.set(DBA_KEY_LATMIN, 1000000);
        query.set(DBA_KEY_QUERY, "best");

        // Make the query
        ensure_equals(cursor.query_data(query), 4);

        // There should be four items
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 3);
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 2);
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 1);
        ensure(cursor.next());
        ensure_equals(cursor.remaining(), 0);

        // Now there should not be anything anymore
        ensure(!cursor.next());
}

// Check if deletion works
template<> template<>
void to::test<7>()
{
        use_db();
        populate_database();

        db::Cursor cursor(*db);

        // 4 items to begin with
        query.clear();
        ensure_equals(cursor.query_data(query), 4);

        query.clear();
        query.set(DBA_KEY_YEARMIN, 1945);
        query.set(DBA_KEY_MONTHMIN, 4);
        query.set(DBA_KEY_DAYMIN, 25);
        query.set(DBA_KEY_HOURMIN, 8);
        query.set(DBA_KEY_MINUMIN, 10);
        db->remove(query);

        // 2 remaining after remove
        query.clear();
        ensure_equals(cursor.query_data(query), 2);

        // Did it remove the right ones?
        query.clear();
        query.set(DBA_KEY_LATMIN, 1000000);
        ensure_equals(cursor.query_data(query), 2);
        ensure(cursor.next());
        cursor.to_record(result);
        ensure(result.contains(sampleAna));
        ensure(result.contains(sampleBase));

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample00));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample01));

        /* The item should have two data in it */
        ensure(cursor.next());
        cursor.to_record(result);

        ensure(cursor.out_idvar == WR_VAR(0, 1, 11) || cursor.out_idvar == WR_VAR(0, 1, 12));
        if (cursor.out_idvar == WR_VAR(0, 1, 11))
                ensure(result.contains(sample00));
        if (cursor.out_idvar == WR_VAR(0, 1, 12))
                ensure(result.contains(sample01));

        ensure(!cursor.next());
}

/* Test datetime queries */
template<> template<>
void to::test<8>()
{
        use_db();

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
        db::Cursor cursor(*db); \
        ensure_equals(cursor.query_data(query), 1); \
        ensure(cursor.next()); \
        cursor.to_record(result); \
        ensure_equals(cursor.count, 0); \
        ensure_varcode_equals(cursor.out_idvar, WR_VAR(0, 1, 12)); \
        ensure(result.contains(ab)); \
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

#if 0
// Test working with QC data
template<> template<>
void to::test<9>()
{
        use_db();
        populate_database();

	{
		int count, has_data;
		int context;
		dba_db_cursor cursor;
		int val;
		int qc_count;
		int found;

		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_LATMIN, 1000000));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		do {
			CHECKED(dba_db_cursor_next(cursor, &has_data));
			gen_ensure(has_data);
			/* fprintf(stderr, "%d B%02d%03d\n", count, WR_VAR_X(var), WR_VAR_Y(var)); */
		} while (cursor->count && cursor->out_idvar != WR_VAR(0, 1, 11));
		gen_ensure(cursor->out_idvar == WR_VAR(0, 1, 11));
		context = cursor->out_context_id;
		dba_db_cursor_delete(cursor);

		/* Insert new QC data about this report */
		dba_record_clear(qc);
		CHECKED(dba_record_var_seti(qc, WR_VAR(0, 33, 2), 2));
		CHECKED(dba_record_var_seti(qc, WR_VAR(0, 33, 3), 5));
		CHECKED(dba_record_var_seti(qc, WR_VAR(0, 33, 5), 33));
		CHECKED(dba_db_qc_insert(db, context, WR_VAR(0, 1, 11), qc));

		/* Query back the data */
		dba_record_clear(qc);
		CHECKED(dba_db_qc_query(db, context, WR_VAR(0, 1, 11), NULL, 0, qc, &qc_count));

		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 2), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 2);
		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 3), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 5);
		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 5), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 33);

		/* Delete a couple of items */
		{
			dba_varcode todel[] = {WR_VAR(0, 33, 2), WR_VAR(0, 33, 5)};
			CHECKED(dba_db_qc_remove(db, context, WR_VAR(0, 1, 11), todel, 2));
		}
		/* Deleting non-existing items should not fail.  Also try creating a
		 * query with just on item */
		{
			dba_varcode todel[] = {WR_VAR(0, 33, 2)};
			CHECKED(dba_db_qc_remove(db, context, WR_VAR(0, 1, 11), todel, 1));
		}

		/* Query back the data */
		dba_record_clear(qc);
		{
			dba_varcode toget[] = { WR_VAR(0, 33, 2), WR_VAR(0, 33, 3), WR_VAR(0, 33, 5) };
			CHECKED(dba_db_qc_query(db, context, WR_VAR(0, 1, 11), toget, 3, qc, &qc_count));
		}

		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 2), &val, &found));
		gen_ensure_equals(found, 0);
		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 3), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure(val == 5);
		CHECKED(dba_record_var_enqi(qc, WR_VAR(0, 33, 5), &val, &found));
		gen_ensure_equals(found, 0);
	}

	/*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
}

/* Test ana queries */
template<> template<>
void to::test<10>()
{
	use_db();

	reset_database();

	int count, has_data;
	dba_db_cursor cursor;

	dba_record_clear(query);
	CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, 1));
	CHECKED(dba_db_ana_query(db, query, &cursor, &count));
	//cerr << dba_querybuf_get(cursor->query) << endl;

	gen_ensure_equals(count, 1);
	CHECKED(dba_db_cursor_next(cursor, &has_data));
	gen_ensure(has_data);
	CHECKED(dba_db_cursor_next(cursor, &has_data));
	gen_ensure(!has_data);
	dba_db_cursor_delete(cursor);
}

/* Run a search for orphan elements */
template<> template<>
void to::test<11>()
{
	use_db();

	CHECKED(dba_db_remove_orphans(db));
}

/* Insert some attributes and try to read them again */
template<> template<>
void to::test<12>()
{
	use_db();

	/* Start with an empty database */
	CHECKED(dba_db_reset(db, 0));

	/* Fill in data for the first record */
	dba_record_clear(insert);
	sampleAna.copyTestDataToRecord(insert);
	sampleBase.copyTestDataToRecord(insert);
	sample0.copyTestDataToRecord(insert);
	sample00.copyTestDataToRecord(insert);
	sample01.copyTestDataToRecord(insert);

	/* Insert the record */
	int anaid, contextid;
	CHECKED(dba_db_insert(db, insert, 0, 1, &anaid, &contextid));

	gen_ensure_equals(anaid, 1);
	gen_ensure_equals(contextid, 1);

	dba_record_clear(qc);
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  1,  7),  1));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  2, 48),  2));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  5, 40),  3));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  5, 41),  4));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  5, 43),  5));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0, 33, 32),  6));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  7, 24),  7));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  5, 21),  8));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  7, 25),  9));
	CHECKED(dba_record_var_seti(qc, WR_VAR(0,  5, 22), 10));

	CHECKED(dba_db_qc_insert_new(db, contextid, WR_VAR(0, 1, 11), qc));

	dba_record_clear(qc);
	int count;
	CHECKED(dba_db_qc_query(db, contextid, WR_VAR(0, 1, 11), NULL, 0, qc, &count));

	std::map<dba_varcode, std::string> values;
	values.insert(make_pair(WR_VAR(0, 1, 7), std::string("1")));
	values.insert(make_pair(WR_VAR(0, 2, 48), std::string("2")));
	values.insert(make_pair(WR_VAR(0,  1,  7), std::string("1")));
	values.insert(make_pair(WR_VAR(0,  2, 48), std::string("2")));
	values.insert(make_pair(WR_VAR(0,  5, 40), std::string("3")));
	values.insert(make_pair(WR_VAR(0,  5, 41), std::string("4")));
	values.insert(make_pair(WR_VAR(0,  5, 43), std::string("5")));
	values.insert(make_pair(WR_VAR(0, 33, 32), std::string("6")));
	values.insert(make_pair(WR_VAR(0,  7, 24), std::string("7")));
	values.insert(make_pair(WR_VAR(0,  5, 21), std::string("8")));
	values.insert(make_pair(WR_VAR(0,  7, 25), std::string("9")));
	values.insert(make_pair(WR_VAR(0,  5, 22), std::string("10")));

	// Check that all the attributes come out
	gen_ensure_equals(count, (int)values.size());
	for (dba_record_cursor cur = dba_record_iterate_first(qc);
			cur != NULL; cur = dba_record_iterate_next(qc, cur))
	{
		dba_var var = dba_record_cursor_variable(cur);
		std::map<dba_varcode, std::string>::iterator v
			= values.find(dba_var_code(var));
		if (v == values.end())
		{
			char buf[20];
			snprintf(buf, 20, "Attr B%02d%03d not found",
					WR_VAR_X(dba_var_code(var)),
					WR_VAR_Y(dba_var_code(var)));
			ensure(false, buf);
		} else {
			gen_ensure_equals(string(dba_var_value(var)), v->second);
			values.erase(v);
		}
	}
}

/* Query using lonmin > latmax */
template<> template<>
void to::test<13>()
{
	use_db();

	/* Start with an empty database */
	CHECKED(dba_db_reset(db, 0));

	/* Fill in data for the first record */
	dba_record_clear(insert);
	sampleAna.copyTestDataToRecord(insert);
	sampleBase.copyTestDataToRecord(insert);
	sample0.copyTestDataToRecord(insert);
	sample00.copyTestDataToRecord(insert);
	sample01.copyTestDataToRecord(insert);

	/* Insert the record */
	int anaid, contextid;
	CHECKED(dba_db_insert(db, insert, 0, 1, &anaid, &contextid));

	gen_ensure_equals(anaid, 1);
	gen_ensure_equals(contextid, 1);

	dba_record_clear(query);
	CHECKED(dba_record_key_setd(query, DBA_KEY_LATMIN, 10.0));
	CHECKED(dba_record_key_setd(query, DBA_KEY_LATMAX, 15.0));
	CHECKED(dba_record_key_setd(query, DBA_KEY_LONMIN, 70.0));
	CHECKED(dba_record_key_setd(query, DBA_KEY_LONMAX, -160.0));

	int count;
	dba_db_cursor cursor;
	CHECKED(dba_db_query(db, query, &cursor, &count));
	gen_ensure_equals(count, 2);
}

/* This query caused problems */
template<> template<>
void to::test<14>()
{
	use_db();

	reset_database();

	dba_record_clear(query);
	CHECKED(dba_record_key_setc(query, DBA_KEY_ANA_FILTER, "B07001>1"));

	int has_data;
	dba_db_cursor cur;

	/* Allocate a new cursor */
	CHECKED(dba_db_cursor_create(db, &cur));

	/* Perform the query, limited to level values */
	CHECKED(dba_db_cursor_query(cur, query, DBA_DB_WANT_ANA_ID, 0));

	gen_ensure_equals(dba_db_cursor_remaining(cur), 2);
	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(has_data);
	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(has_data);
	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(!has_data);
	dba_db_cursor_delete(cur);
}

/* Insert with undef leveltype2 and l2 */
template<> template<>
void to::test<15>()
{
	use_db();

	reset_database();

	dba_record_clear(insert);

	sampleAna.copyTestDataToRecord(insert);
	sampleBase.copyTestDataToRecord(insert);
	sample0.copyTestDataToRecord(insert);
	sample01.copyTestDataToRecord(insert);

	dba_record_key_seti(insert, DBA_KEY_LEVELTYPE1, 44);
	dba_record_key_seti(insert, DBA_KEY_L1, 55);
	dba_record_key_unset(insert, DBA_KEY_LEVELTYPE2);
	dba_record_key_unset(insert, DBA_KEY_L2);

	/* Insert the record */
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	dba_record_clear(query);
	CHECKED(dba_record_key_seti(query, DBA_KEY_LEVELTYPE1, 44));
	CHECKED(dba_record_key_seti(query, DBA_KEY_L1, 55));

	int has_data;
	dba_db_cursor cur;

	/* Allocate a new cursor */
	CHECKED(dba_db_cursor_create(db, &cur));

	/* Perform the query, limited to level values */
	CHECKED(dba_db_cursor_query(cur, query, DBA_DB_WANT_VAR_VALUE | DBA_DB_WANT_LEVEL, 0));

	gen_ensure_equals(dba_db_cursor_remaining(cur), 1);
	CHECKED(dba_db_cursor_next(cur, &has_data));

	dba_record_clear(result);
	CHECKED(dba_db_cursor_to_record(cur, result));

	int val, found;
	CHECKED(dba_record_key_enqi(result, DBA_KEY_LEVELTYPE1, &val, &found));
	gen_ensure(found);
	gen_ensure_equals(val, 44);
	CHECKED(dba_record_key_enqi(result, DBA_KEY_L1, &val, &found));
	gen_ensure(found);
	gen_ensure_equals(val, 55);
	CHECKED(dba_record_key_enqi(result, DBA_KEY_LEVELTYPE2, &val, &found));
	gen_ensure(found);
	gen_ensure_equals(val, 0);
	CHECKED(dba_record_key_enqi(result, DBA_KEY_L2, &val, &found));
	gen_ensure(found);
	gen_ensure_equals(val, 0);

	gen_ensure(has_data);
	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(!has_data);
	dba_db_cursor_delete(cur);
}

/* Query with undef leveltype2 and l2 */
template<> template<>
void to::test<16>()
{
	use_db();

	reset_database();

	dba_record_clear(query);
	CHECKED(dba_record_key_seti(query, DBA_KEY_LEVELTYPE1, 10));
	CHECKED(dba_record_key_seti(query, DBA_KEY_L1, 11));

	dba_db_cursor cur;

	/* Allocate a new cursor */
	CHECKED(dba_db_cursor_create(db, &cur));

	/* Perform the query, limited to level values */
	CHECKED(dba_db_cursor_query(cur, query, DBA_DB_WANT_VAR_VALUE, 0));

	gen_ensure_equals(dba_db_cursor_remaining(cur), 4);
	dba_db_cursor_delete(cur);
}

/* Query with an incorrect attr_filter */
template<> template<>
void to::test<17>()
{
	use_db();

	reset_database();

	dba_record_clear(query);
	CHECKED(dba_record_key_setc(query, DBA_KEY_ATTR_FILTER, "B12001"));

	dba_db_cursor cur;

	/* Allocate a new cursor */
	CHECKED(dba_db_cursor_create(db, &cur));

	/* Perform the query, limited to level values */
	gen_ensure(dba_db_cursor_query(cur, query, DBA_DB_WANT_VAR_VALUE, 0) != 0);
	dba_error_ok();

	dba_db_cursor_delete(cur);
}

/* Test querying priomax together with query=best */
template<> template<>
void to::test<18>()
{
	use_db();

	/* Start with an empty database */
	CHECKED(dba_db_reset(db, 0));

	/* Prepare the common parts of some data */
	dba_record_clear(insert);
	CHECKED(dba_record_key_seti(insert, DBA_KEY_LAT, 1));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_LON, 1));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_LEVELTYPE1, 1));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_L1, 0));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_PINDICATOR, 254));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_P1, 0));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_P2, 0));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_YEAR, 2009));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_MONTH, 11));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_DAY, 11));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_HOUR, 0));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_MIN, 0));
	CHECKED(dba_record_key_seti(insert, DBA_KEY_SEC, 0));

	//  1,synop,synop,101,oss,0
	//  2,metar,metar,81,oss,0
	//  3,temp,sounding,98,oss,2
	//  4,pilot,wind profile,80,oss,2
	//  9,buoy,buoy,50,oss,31
	// 10,ship,synop ship,99,oss,1
	// 11,tempship,temp ship,100,oss,2
	// 12,airep,airep,82,oss,4
	// 13,amdar,amdar,97,oss,4
	// 14,acars,acars,96,oss,4
	// 42,pollution,pollution,199,oss,8
	// 200,satellite,NOAA satellites,41,oss,255
	// 255,generic,generic data,1000,?,255

	static int rep_cods[] = { 1, 2, 3, 4, 9, 10, 11, 12, 13, 14, 42, 200, 255, -1 };

	for (int* i = rep_cods; *i != -1; ++i)
	{
		CHECKED(dba_record_key_seti(insert, DBA_KEY_REP_COD, *i));
		CHECKED(dba_record_var_seti(insert, WR_VAR(0, 12, 101), *i));
		int paid;
		CHECKED(dba_db_insert(db, insert, 0, 1, &paid, NULL));
	}

	int count, has_data;
	dba_db_cursor cur;
	int repcod, found;

	// Query with querybest only
	CHECKED(dba_db_cursor_create(db, &cur));

	dba_record_clear(query);
	CHECKED(dba_record_key_setc(query, DBA_KEY_QUERY, "best"));
	CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2009));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 11));
	CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 11));
	CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 0));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MIN, 0));
	CHECKED(dba_record_key_seti(query, DBA_KEY_SEC, 0));
	CHECKED(dba_record_key_setc(query, DBA_KEY_VAR, "B12101"));
	CHECKED(dba_db_cursor_query(cur, query, DBA_DB_WANT_REPCOD | DBA_DB_WANT_VAR_VALUE, 0));

	gen_ensure_equals(dba_db_cursor_remaining(cur), 1);

	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(has_data);

	dba_record_clear(result);
	CHECKED(dba_db_cursor_to_record(cur, result));

	CHECKED(dba_record_key_enqi(result, DBA_KEY_REP_COD, &repcod, &found));
	gen_ensure(found);
	gen_ensure_equals(repcod, 255);

	dba_db_cursor_delete(cur);

	// Query with querybest and priomax
	CHECKED(dba_db_cursor_create(db, &cur));

	dba_record_clear(query);
	CHECKED(dba_record_key_seti(query, DBA_KEY_PRIOMAX, 100));
	CHECKED(dba_record_key_setc(query, DBA_KEY_QUERY, "best"));
	CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2009));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 11));
	CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 11));
	CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 0));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MIN, 0));
	CHECKED(dba_record_key_seti(query, DBA_KEY_SEC, 0));
	CHECKED(dba_record_key_setc(query, DBA_KEY_VAR, "B12101"));
	CHECKED(dba_db_cursor_query(cur, query, DBA_DB_WANT_REPCOD | DBA_DB_WANT_VAR_VALUE, 0));

	gen_ensure_equals(dba_db_cursor_remaining(cur), 1);

	CHECKED(dba_db_cursor_next(cur, &has_data));
	gen_ensure(has_data);

	dba_record_clear(result);
	CHECKED(dba_db_cursor_to_record(cur, result));

	CHECKED(dba_record_key_enqi(result, DBA_KEY_REP_COD, &repcod, &found));
	gen_ensure(found);
	gen_ensure_equals(repcod, 11);

	dba_db_cursor_delete(cur);
}
#endif

}

/* vim:set ts=4 sw=4: */
