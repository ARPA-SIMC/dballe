/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <dballe/db/internals.h>

namespace tut {
using namespace tut_dballe;

static void print_results(dba_db_cursor cur)
{
	dba_record result;
	CHECKED(dba_record_create(&result));
	fprintf(stderr, "Query: %s\n%d results:\n", dba_querybuf_get(cur->query), cur->count);
	for (int i = 0; true; ++i)
	{
		int has_data;
		CHECKED(dba_db_cursor_next(cur, &has_data));
		if (!has_data)
			break;
		fprintf(stderr, " * Result %d:\n", i);
		CHECKED(dba_db_cursor_to_record(cur, result));
		dba_record_print(result, stderr);
	}
	dba_record_delete(result);
}


struct dba_db_dballe_shar
{
	// Records with test data
	TestRecord sampleAna;
	TestRecord extraAna;
	TestRecord sampleBase;
	TestRecord sample0;
	TestRecord sample00;
	TestRecord sample01;
	TestRecord sample1;
	TestRecord sample10;
	TestRecord sample11;

	// Work records
	dba_record insert;
	dba_record query;
	dba_record result;
	dba_record qc;

	// DB handle
	dba_db db;

	dba_db_dballe_shar()
		: insert(NULL), query(NULL), result(NULL), qc(NULL), db(NULL)
	{
		CHECKED(create_dba_db(&db));

		CHECKED(dba_record_create(&insert));
		CHECKED(dba_record_create(&query));
		CHECKED(dba_record_create(&result));
		CHECKED(dba_record_create(&qc));

		// Common data (ana)
		sampleAna.set(DBA_KEY_LAT, 12.34560);
		sampleAna.set(DBA_KEY_LON, 76.54320);
		sampleAna.set(DBA_KEY_MOBILE, 0);
		/*
		sampleAna.set(DBA_KEY_YEAR_IDENT, 2003);
		sampleAna.set(DBA_KEY_MONTH_IDENT, 3);
		sampleAna.set(DBA_KEY_DAY_IDENT, 23);
		sampleAna.set(DBA_KEY_HOUR_IDENT, 12);
		sampleAna.set(DBA_KEY_MIN_IDENT, 30);
		*/
		extraAna.set(DBA_VAR(0, 7,  1), 42);		// Height
		extraAna.set(DBA_VAR(0, 7, 31), 234);		// Heightbaro
		extraAna.set(DBA_VAR(0, 1,  1), 1);			// Block
		extraAna.set(DBA_VAR(0, 1,  2), 52);		// Station
		extraAna.set(DBA_VAR(0, 1, 19), "Cippo Lippo");	// Name

		// Common data
		sampleBase.set(DBA_KEY_YEAR, 1945);
		sampleBase.set(DBA_KEY_MONTH, 4);
		sampleBase.set(DBA_KEY_DAY, 25);
		sampleBase.set(DBA_KEY_HOUR, 8);
		sampleBase.set(DBA_KEY_LEVELTYPE, 10);
		sampleBase.set(DBA_KEY_L1, 11);
		sampleBase.set(DBA_KEY_L2, 22);
		sampleBase.set(DBA_KEY_PINDICATOR, 20);
		sampleBase.set(DBA_KEY_P1, 111);

		// Specific data
		sample0.set(DBA_KEY_MIN, 0);
		sample0.set(DBA_KEY_P2, 122);
		sample0.set(DBA_KEY_REP_COD, 1);
		sample0.set(DBA_KEY_PRIORITY, 100);

		sample00.set(DBA_VAR(0, 1, 11), "DB-All.e!");
		sample01.set(DBA_VAR(0, 1, 12), 500);

		sample1.set(DBA_KEY_MIN, 30);
		sample1.set(DBA_KEY_P2, 123);
		sample1.set(DBA_KEY_REP_COD, 2);
		sample1.set(DBA_KEY_PRIORITY, 80);

		sample10.set(DBA_VAR(0, 1, 11), "Arpa-Sim!");
		sample11.set(DBA_VAR(0, 1, 12), 600);

		/*
static struct test_data tdata3_patch[] = {
	{ "mobile", "1" },
	{ "ident", "Cippo" },
};
		*/

	}

	~dba_db_dballe_shar()
	{
		if (insert != NULL) dba_record_delete(insert);
		if (query != NULL) dba_record_delete(query);
		if (result != NULL) dba_record_delete(result);
		if (qc != NULL) dba_record_delete(qc);
		if (db != NULL) dba_db_delete(db);
	}
};
TESTGRP(dba_db_dballe);

/* Test querybuf */
template<> template<>
void to::test<1>()
{
	dba_querybuf buf = NULL;

	CHECKED(dba_querybuf_create(10, &buf));
	gen_ensure(buf != NULL);
	
	/* A new querybuf contains the empty string */
	gen_ensure_equals(string(dba_querybuf_get(buf)), string());
	gen_ensure_equals(dba_querybuf_size(buf), 0);

	dba_querybuf_reset(buf);
	gen_ensure_equals(string(dba_querybuf_get(buf)), string());
	gen_ensure_equals(dba_querybuf_size(buf), 0);

	CHECKED(dba_querybuf_append(buf, "ciao"));
	gen_ensure_equals(string(dba_querybuf_get(buf)), string("ciao"));
	gen_ensure_equals(dba_querybuf_size(buf), 4);

	CHECKED(dba_querybuf_appendf(buf, "%d %s", 42, "--"));
	gen_ensure_equals(string(dba_querybuf_get(buf)), string("ciao42 --"));
	gen_ensure_equals(dba_querybuf_size(buf), 9);
	
	dba_querybuf_reset(buf);
	gen_ensure_equals(string(dba_querybuf_get(buf)), string());
	gen_ensure_equals(dba_querybuf_size(buf), 0);

	CHECKED(dba_querybuf_append(buf, "123456789"));
	gen_ensure_equals(string(dba_querybuf_get(buf)), string("123456789"));
	gen_ensure_equals(dba_querybuf_size(buf), 9);

	dba_querybuf_reset(buf);
	gen_ensure_equals(string(dba_querybuf_get(buf)), string());
	gen_ensure_equals(dba_querybuf_size(buf), 0);
	CHECKED(dba_querybuf_start_list(buf, ", "));
	CHECKED(dba_querybuf_append_list(buf, "1"));
	CHECKED(dba_querybuf_append_list(buf, "2"));
	CHECKED(dba_querybuf_append_list(buf, "3"));
	gen_ensure_equals(string(dba_querybuf_get(buf)), string("1, 2, 3"));
	gen_ensure_equals(dba_querybuf_size(buf), 7);

	dba_querybuf_delete(buf);
}

// Ensure that reset will work on an empty database
template<> template<>
void to::test<2>()
{
	CHECKED(dba_db_delete_tables(db));
	CHECKED(dba_db_reset(db, 0));
	// Run twice to see if it is idempotent
	CHECKED(dba_db_reset(db, 0));
}

template<> template<>
void to::test<3>()
{
	/*dba_error_set_callback(DBA_ERR_NONE, crash, 0);*/

	/* Test a dballe session */

	/* Start with an empty database */
	CHECKED(dba_db_reset(db, 0));

	/* Insert the ana station */
	dba_record_clear(insert);
	CHECKED(dba_record_set_ana_context(insert));
	sampleAna.copyTestDataToRecord(insert);
	extraAna.copyTestDataToRecord(insert);
	/* Insert the anagraphical record */
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	/* Fill in data for the first record */
	dba_record_clear(insert);
	sampleAna.copyTestDataToRecord(insert);
	sampleBase.copyTestDataToRecord(insert);
	sample0.copyTestDataToRecord(insert);
	sample00.copyTestDataToRecord(insert);
	sample01.copyTestDataToRecord(insert);

	/* Insert the record */
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));
	/* Check if duplicate updates are allowed by insert */
	CHECKED(dba_db_insert(db, insert, 1, 0, NULL, NULL));
	/* Check if duplicate updates are trapped by insert_new */
	gen_ensure(dba_db_insert(db, insert, 0, 0, NULL, NULL) == DBA_ERROR);

	/* Insert another record (similar but not the same) */
	dba_record_clear(insert);
	sampleAna.copyTestDataToRecord(insert);
	sampleBase.copyTestDataToRecord(insert);
	sample1.copyTestDataToRecord(insert);
	sample10.copyTestDataToRecord(insert);
	sample11.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));
	/* Check again if duplicate updates are trapped */
	gen_ensure(dba_db_insert(db, insert, 0, 0, NULL, NULL) == DBA_ERROR);

	/* Check dba_ana_* functions */
	{
		int has_data;
		int count = 0;
		dba_db_cursor cursor;

		/*
		CHECKED(dba_ana_count(db, &count));
		fail_unless(count == 1);
		*/
		dba_record_clear(query);

		/* Iterate the anagraphic database */
		CHECKED(dba_db_ana_query(db, query, &cursor, &count));

		gen_ensure(cursor != 0);
		gen_ensure_equals(count, 1);

		/* There should be an item */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		/* Check that the result matches */
		ensureTestRecEquals(result, sampleAna);
		//ensureTestRecEquals(result, extraAna);

		/* There should be only one item */
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 0);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(!has_data);

		dba_db_cursor_delete(cursor);
	}

	/* Try many possible queries */

#define TRY_QUERY(type, param, value, expected_count) do {\
		int count; \
		dba_db_cursor cursor; \
		dba_record_clear(query); \
		CHECKED(dba_record_key_set##type(query, param, value)); \
		CHECKED(dba_db_query(db, query, &cursor, &count)); \
		gen_ensure(cursor != 0); \
		if (0) \
			print_results(cursor); \
		gen_ensure_equals(count, expected_count); \
		dba_db_cursor_delete(cursor); \
	} while (0)

#define TRY_QUERY1(type, param, value, expected_count) do {\
		int count; \
		dba_db_cursor cursor; \
		dba_record_clear(query); \
		CHECKED(dba_record_var_set##type(query, param, value)); \
		CHECKED(dba_db_query(db, query, &cursor, &count)); \
		gen_ensure(cursor != 0); \
		if (0) \
			print_results(cursor); \
		gen_ensure_equals(count, expected_count); \
		dba_db_cursor_delete(cursor); \
	} while (0)

#define TRY_QUERY2(lonmin, lonmax, expected_count) do {\
		int count; \
		dba_db_cursor cursor; \
		dba_record_clear(query); \
		CHECKED(dba_record_key_setd(query, DBA_KEY_LONMIN, lonmin)); \
		CHECKED(dba_record_key_setd(query, DBA_KEY_LONMAX, lonmax)); \
		CHECKED(dba_db_query(db, query, &cursor, &count)); \
		gen_ensure(cursor != 0); \
		if (0) \
			print_results(cursor); \
		gen_ensure_equals(count, expected_count); \
		dba_db_cursor_delete(cursor); \
	} while (0)


	TRY_QUERY(c, DBA_KEY_ANA_ID, "1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_ID, "2", 0);
	TRY_QUERY(i, DBA_KEY_YEAR, 1000, 5);
	TRY_QUERY(i, DBA_KEY_YEAR, 1001, 0);
	TRY_QUERY(i, DBA_KEY_YEARMIN, 1999, 0);
	TRY_QUERY(i, DBA_KEY_YEARMIN, 1945, 4);
	TRY_QUERY(i, DBA_KEY_YEARMAX, 1944, 0);
	TRY_QUERY(i, DBA_KEY_YEARMAX, 1945, 4);
	TRY_QUERY(i, DBA_KEY_YEARMAX, 2030, 4);
	TRY_QUERY(i, DBA_KEY_YEAR, 1944, 0);
	TRY_QUERY(i, DBA_KEY_YEAR, 1945, 4);
	TRY_QUERY(i, DBA_KEY_YEAR, 1946, 0);
	TRY_QUERY1(i, DBA_VAR(0, 1, 1), 1, 4);
	TRY_QUERY1(i, DBA_VAR(0, 1, 1), 2, 0);
	TRY_QUERY1(i, DBA_VAR(0, 1, 2), 52, 4);
	TRY_QUERY1(i, DBA_VAR(0, 1, 2), 53, 0);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "block=1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "B01001=1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "block>1", 0);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "B01001>1", 0);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "block<=1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "B01001<=1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "0<=B01001<=2", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "1<=B01001<=1", 4);
	TRY_QUERY(c, DBA_KEY_ANA_FILTER, "2<=B01001<=4", 0);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01011=DB-All.e!", 2);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01012=500", 2);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01012>=500", 4);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01012>500", 2);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01012<600", 2);
	TRY_QUERY(c, DBA_KEY_DATA_FILTER, "B01012<=600", 4);

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
	TRY_QUERY(d, DBA_KEY_LATMIN, 11, 4);
	TRY_QUERY(d, DBA_KEY_LATMIN, 12.34560, 4);
	TRY_QUERY(d, DBA_KEY_LATMIN, 13, 0);
	TRY_QUERY(d, DBA_KEY_LATMAX, 11, 0);
	TRY_QUERY(d, DBA_KEY_LATMAX, 12.34560, 4);
	TRY_QUERY(d, DBA_KEY_LATMAX, 13, 4);
	TRY_QUERY2(75., 77., 4);
	TRY_QUERY2(76.54320, 76.54320, 4);
	TRY_QUERY2(76.54330, 77., 0);
	TRY_QUERY2(77., 76.54330, 4);
	TRY_QUERY2(77., 76.54320, 4);
	TRY_QUERY2(77., -10, 0);
	TRY_QUERY(i, DBA_KEY_MOBILE, 0, 4);
	TRY_QUERY(i, DBA_KEY_MOBILE, 1, 0);
	//TRY_QUERY(c, DBA_KEY_IDENT_SELECT, "pippo");
	TRY_QUERY(i, DBA_KEY_PINDICATOR, 20, 4);
	TRY_QUERY(i, DBA_KEY_PINDICATOR, 21, 0);
	TRY_QUERY(i, DBA_KEY_P1, 111, 4);
	TRY_QUERY(i, DBA_KEY_P1, 112, 0);
	TRY_QUERY(i, DBA_KEY_P2, 121, 0);
	TRY_QUERY(i, DBA_KEY_P2, 122, 2);
	TRY_QUERY(i, DBA_KEY_P2, 123, 2);
	TRY_QUERY(i, DBA_KEY_LEVELTYPE, 10, 4);
	TRY_QUERY(i, DBA_KEY_LEVELTYPE, 11, 0);
	TRY_QUERY(i, DBA_KEY_L1, 11, 4);
	TRY_QUERY(i, DBA_KEY_L1, 12, 0);
	TRY_QUERY(i, DBA_KEY_L2, 22, 4);
	TRY_QUERY(i, DBA_KEY_L2, 23, 0);
	TRY_QUERY(c, DBA_KEY_VAR, "B01011", 2);
	TRY_QUERY(c, DBA_KEY_VAR, "B01012", 2);
	TRY_QUERY(c, DBA_KEY_VAR, "B01013", 0);
	TRY_QUERY(i, DBA_KEY_REP_COD, 1, 2);
	TRY_QUERY(i, DBA_KEY_REP_COD, 2, 2);
	TRY_QUERY(i, DBA_KEY_REP_COD, 3, 0);
	TRY_QUERY(i, DBA_KEY_PRIORITY, 100, 2);
	TRY_QUERY(i, DBA_KEY_PRIORITY, 80, 2);
	TRY_QUERY(i, DBA_KEY_PRIORITY, 101, 0);
	TRY_QUERY(i, DBA_KEY_PRIOMIN, 70, 4);
	TRY_QUERY(i, DBA_KEY_PRIOMIN, 80, 4);
	TRY_QUERY(i, DBA_KEY_PRIOMIN, 90, 2);
	TRY_QUERY(i, DBA_KEY_PRIOMIN, 100, 2);
	TRY_QUERY(i, DBA_KEY_PRIOMIN, 110, 0);
	TRY_QUERY(i, DBA_KEY_PRIOMAX, 70, 0);
	TRY_QUERY(i, DBA_KEY_PRIOMAX, 80, 2);
	TRY_QUERY(i, DBA_KEY_PRIOMAX, 90, 2);
	TRY_QUERY(i, DBA_KEY_PRIOMAX, 100, 4);
	TRY_QUERY(i, DBA_KEY_PRIOMAX, 110, 4);

	/* Try a query */
	{
		int has_data;
		int count;
		dba_db_cursor cursor;

		dba_record_clear(query);

		/* Prepare a query */
		CHECKED(dba_record_key_setd(query, DBA_KEY_LATMIN, 10.0));

		/* Make the query */
		CHECKED(dba_db_query(db, query, &cursor, &count));

		/* See that a cursor has in fact been allocated */
		gen_ensure(cursor != 0);
		/* 2 + 2 of actual data */
		gen_ensure_equals(count, 4);

		/* There should be at least one item */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		/* Check that the results match */
		ensureTestRecEquals(result, sampleAna);
		ensureTestRecEquals(result, sampleBase);
		ensureTestRecEquals(result, sample0);

		/*
		printrecord(result, "RES: ");
		exit(0);
		*/

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample00);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample01);

		/* The item should have two data in it */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample00);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample01);

		/* There should be also another item */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		/* Check that the results matches */
		ensureTestRecEquals(result, sampleAna);
		ensureTestRecEquals(result, sampleBase);
		ensureTestRecEquals(result, sample1);

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample10);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample11);

		/* The item should have two data in it */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample10);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample11);

		/* Now there should not be anything anymore */
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 0);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(!has_data);

		/* Deallocate the cursor */
		dba_db_cursor_delete(cursor);
	}

	/* Try a query for best value */
	{
		int count, has_data;
		dba_db_cursor cursor;

		dba_record_clear(query);

		/* Prepare a query */
		CHECKED(dba_record_key_seti(query, DBA_KEY_LATMIN, 1000000));
		CHECKED(dba_record_key_setc(query, DBA_KEY_QUERY, "best"));

		/* Make the query */
		CHECKED(dba_db_query(db, query, &cursor, &count));

		/* See that a cursor has in fact been allocated */
		gen_ensure(cursor != 0);

		/* There should be four items */
		gen_ensure_equals(count, 4);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 3);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 2);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 1);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);

		/* Now there should not be anything anymore */
		gen_ensure_equals(dba_db_cursor_remaining(cursor), 0);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(!has_data);

		/* Deallocate the cursor */
		dba_db_cursor_delete(cursor);
	}


	/* Delete one of the items */
	dba_record_clear(query);
	CHECKED(dba_record_key_seti(query, DBA_KEY_YEARMIN, 1945));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MONTHMIN, 4));
	CHECKED(dba_record_key_seti(query, DBA_KEY_DAYMIN, 25));
	CHECKED(dba_record_key_seti(query, DBA_KEY_HOURMIN, 8));
	CHECKED(dba_record_key_seti(query, DBA_KEY_MINUMIN, 10));
	CHECKED(dba_db_remove(db, query));

#if 0
	/* Querying for both exact time and a time range should fail */
	{
		int count;
		dba_cursor cursor;
		CHECKED(dba_seti(query, "min", 10));
		fail_unless(dba_query(db, query, &cursor, &count) == DBA_ERROR);
		/*
		 * No need to delete, as dba_query failed 
		dba_cursor_delete(cursor);
		 */
	}
#endif

	/* See if the results change after deleting the tdata2 item */
	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_LATMIN, 1000000));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure(count > 0);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		ensureTestRecEquals(result, sampleAna);
		ensureTestRecEquals(result, sampleBase);

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample00);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample01);

		/* The item should have two data in it */
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));

		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11) || cursor->out_idvar == DBA_VAR(0, 1, 12));
		if (cursor->out_idvar == DBA_VAR(0, 1, 11))
			ensureTestRecEquals(result, sample00);
		if (cursor->out_idvar == DBA_VAR(0, 1, 12))
			ensureTestRecEquals(result, sample01);

		gen_ensure_equals(dba_db_cursor_remaining(cursor), 0);
		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(!has_data);
		dba_db_cursor_delete(cursor);
	}

	/* Insert some QC data */
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
			/* fprintf(stderr, "%d B%02d%03d\n", count, DBA_VAR_X(var), DBA_VAR_Y(var)); */
		} while (cursor->count && cursor->out_idvar != DBA_VAR(0, 1, 11));
		gen_ensure(cursor->out_idvar == DBA_VAR(0, 1, 11));
		context = cursor->out_context_id;
		dba_db_cursor_delete(cursor);

		/* Insert new QC data about this report */
		dba_record_clear(qc);
		CHECKED(dba_record_var_seti(qc, DBA_VAR(0, 33, 2), 7));
		CHECKED(dba_record_var_seti(qc, DBA_VAR(0, 33, 3), 5));
		CHECKED(dba_record_var_seti(qc, DBA_VAR(0, 33, 5), 33));
		CHECKED(dba_db_qc_insert(db, context, DBA_VAR(0, 1, 11), qc));

		/* Query back the data */
		dba_record_clear(qc);
		CHECKED(dba_db_qc_query(db, context, DBA_VAR(0, 1, 11), NULL, 0, qc, &qc_count));

		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 2), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 7);
		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 3), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 5);
		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 5), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure_equals(val, 33);

		/* Delete a couple of items */
		{
			dba_varcode todel[] = {DBA_VAR(0, 33, 2), DBA_VAR(0, 33, 5)};
			CHECKED(dba_db_qc_remove(db, context, DBA_VAR(0, 1, 11), todel, 2));
		}
		/* Deleting non-existing items should not fail.  Also try creating a
		 * query with just on item */
		{
			dba_varcode todel[] = {DBA_VAR(0, 33, 2)};
			CHECKED(dba_db_qc_remove(db, context, DBA_VAR(0, 1, 11), todel, 1));
		}

		/* Query back the data */
		dba_record_clear(qc);
		{
			dba_varcode toget[] = { DBA_VAR(0, 33, 2), DBA_VAR(0, 33, 3), DBA_VAR(0, 33, 5) };
			CHECKED(dba_db_qc_query(db, context, DBA_VAR(0, 1, 11), toget, 3, qc, &qc_count));
		}

		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 2), &val, &found));
		gen_ensure_equals(found, 0);
		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 3), &val, &found));
		gen_ensure_equals(found, 1);
		gen_ensure(val == 5);
		CHECKED(dba_record_var_enqi(qc, DBA_VAR(0, 33, 5), &val, &found));
		gen_ensure_equals(found, 0);
	}

	/*dba_error_remove_callback(DBA_ERR_NONE, crash, 0);*/
}

/* Test datetime queries */
template<> template<>
void to::test<4>()
{
	/* Prepare test data */
	TestRecord base, a, b;

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

	base.set(DBA_KEY_LEVELTYPE, 1);
	base.set(DBA_KEY_L1, 0);
	base.set(DBA_KEY_L2, 0);
	base.set(DBA_KEY_PINDICATOR, 1);
	base.set(DBA_KEY_P1, 0);
	base.set(DBA_KEY_P2, 0);

	base.set(DBA_KEY_REP_COD, 1);
	base.set(DBA_KEY_PRIORITY, 100);

	base.set(DBA_VAR(0, 1, 12), 500);

	base.set(DBA_KEY_YEAR, 2006);
	base.set(DBA_KEY_MONTH, 5);
	base.set(DBA_KEY_DAY, 15);
	base.set(DBA_KEY_HOUR, 12);
	base.set(DBA_KEY_MIN, 30);
	base.set(DBA_KEY_SEC, 0);

	/* Year */

	CHECKED(dba_db_reset(db, 0));

	dba_record_clear(insert);
	a = base;
	a.set(DBA_KEY_YEAR, 2005);
	a.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	dba_record_clear(insert);
	b = base;
	b.set(DBA_KEY_YEAR, 2006);
	b.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEARMIN, 2006));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEARMAX, 2005));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, a);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}


	/* Month */

	CHECKED(dba_db_reset(db, 0));

	dba_record_clear(insert);
	a = base;
	a.set(DBA_KEY_YEAR, 2006);
	a.set(DBA_KEY_MONTH, 4);
	a.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	dba_record_clear(insert);
	b = base;
	b.set(DBA_KEY_YEAR, 2006);
	b.set(DBA_KEY_MONTH, 5);
	b.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTHMIN, 5));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTHMAX, 4));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, a);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}


	/* Day */

	CHECKED(dba_db_reset(db, 0));

	dba_record_clear(insert);
	a = base;
	a.set(DBA_KEY_YEAR, 2006);
	a.set(DBA_KEY_MONTH, 5);
	a.set(DBA_KEY_DAY, 2);
	a.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	dba_record_clear(insert);
	b = base;
	b.set(DBA_KEY_YEAR, 2006);
	b.set(DBA_KEY_MONTH, 5);
	b.set(DBA_KEY_DAY, 3);
	b.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAYMIN, 3));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
/*
		dba_record_print(result, stderr); cerr << "---" << endl;
		dba_record_clear(query);
		b.copyTestDataToRecord(query);
		dba_record_print(query, stderr); cerr << "---" << endl;
*/
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAYMAX, 2));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, a);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}


	/* Hour */

	CHECKED(dba_db_reset(db, 0));

	dba_record_clear(insert);
	a = base;
	a.set(DBA_KEY_YEAR, 2006);
	a.set(DBA_KEY_MONTH, 5);
	a.set(DBA_KEY_DAY, 3);
	a.set(DBA_KEY_HOUR, 12);
	a.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	dba_record_clear(insert);
	b = base;
	b.set(DBA_KEY_YEAR, 2006);
	b.set(DBA_KEY_MONTH, 5);
	b.set(DBA_KEY_DAY, 3);
	b.set(DBA_KEY_HOUR, 13);
	b.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOURMIN, 13));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOURMAX, 12));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, a);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 13));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}


	/* Minute */

	CHECKED(dba_db_reset(db, 0));

	dba_record_clear(insert);
	a = base;
	a.set(DBA_KEY_YEAR, 2006);
	a.set(DBA_KEY_MONTH, 5);
	a.set(DBA_KEY_DAY, 3);
	a.set(DBA_KEY_HOUR, 12);
	a.set(DBA_KEY_MIN, 29);
	a.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 1, NULL, NULL));

	dba_record_clear(insert);
	b = base;
	b.set(DBA_KEY_YEAR, 2006);
	b.set(DBA_KEY_MONTH, 5);
	b.set(DBA_KEY_DAY, 3);
	b.set(DBA_KEY_HOUR, 12);
	b.set(DBA_KEY_MIN, 30);
	b.copyTestDataToRecord(insert);
	CHECKED(dba_db_insert(db, insert, 0, 0, NULL, NULL));

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 12));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MINUMIN, 30));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 12));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MINUMAX, 29));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, a);
		dba_db_cursor_delete(cursor);
	}

	{
		int count, has_data;
		dba_db_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_record_key_seti(query, DBA_KEY_YEAR, 2006));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MONTH, 5));
		CHECKED(dba_record_key_seti(query, DBA_KEY_DAY, 3));
		CHECKED(dba_record_key_seti(query, DBA_KEY_HOUR, 12));
		CHECKED(dba_record_key_seti(query, DBA_KEY_MIN, 30));
		CHECKED(dba_db_query(db, query, &cursor, &count));
		gen_ensure_equals(count, 1);

		CHECKED(dba_db_cursor_next(cursor, &has_data));
		gen_ensure(has_data);
		CHECKED(dba_db_cursor_to_record(cursor, result));
		gen_ensure_equals(cursor->count, 0);
		gen_ensure_equals(cursor->out_idvar, DBA_VAR(0, 1, 12));
		ensureTestRecEquals(result, b);
		dba_db_cursor_delete(cursor);
	}
}

/* Test datetime queries */
template<> template<>
void to::test<5>()
{
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
void to::test<6>()
{
	CHECKED(dba_db_remove_orphans(db));
}

/* Insert some attributes and try to read them again */
template<> template<>
void to::test<7>()
{
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
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  1,  7),  1));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  2, 48),  2));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  5, 40),  3));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  5, 41),  4));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  5, 43),  5));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0, 33, 32),  6));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  7, 24),  7));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  5, 21),  8));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  7, 25),  9));
	CHECKED(dba_record_var_seti(qc, DBA_VAR(0,  5, 22), 10));

	CHECKED(dba_db_qc_insert_new(db, contextid, DBA_VAR(0, 1, 11), qc));

	dba_record_clear(qc);
	int count;
	CHECKED(dba_db_qc_query(db, contextid, DBA_VAR(0, 1, 11), NULL, 0, qc, &count));

	std::map<dba_varcode, std::string> values;
	values.insert(make_pair(DBA_VAR(0, 1, 7), std::string("1")));
	values.insert(make_pair(DBA_VAR(0, 2, 48), std::string("2")));
	values.insert(make_pair(DBA_VAR(0,  1,  7), std::string("1")));
	values.insert(make_pair(DBA_VAR(0,  2, 48), std::string("2")));
	values.insert(make_pair(DBA_VAR(0,  5, 40), std::string("3")));
	values.insert(make_pair(DBA_VAR(0,  5, 41), std::string("4")));
	values.insert(make_pair(DBA_VAR(0,  5, 43), std::string("5")));
	values.insert(make_pair(DBA_VAR(0, 33, 32), std::string("6")));
	values.insert(make_pair(DBA_VAR(0,  7, 24), std::string("7")));
	values.insert(make_pair(DBA_VAR(0,  5, 21), std::string("8")));
	values.insert(make_pair(DBA_VAR(0,  7, 25), std::string("9")));
	values.insert(make_pair(DBA_VAR(0,  5, 22), std::string("10")));

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
					DBA_VAR_X(dba_var_code(var)),
					DBA_VAR_Y(dba_var_code(var)));
			ensure(false, buf);
		} else {
			gen_ensure_equals(string(dba_var_value(var)), v->second);
			values.erase(v);
		}
	}
}

/* Query using lonmin > latmax */
template<> template<>
void to::test<8>()
{
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

	int count, has_data;
	dba_db_cursor cursor;
	CHECKED(dba_db_query(db, query, &cursor, &count));
	gen_ensure_equals(count, 2);
}

}

/* vim:set ts=4 sw=4: */
