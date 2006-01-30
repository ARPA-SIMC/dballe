#include <tests/test-utils.h>
#include <dballe/dba_init.h>
#include <dballe/db/dba_import.h>
#include <dballe/db/dba_export.h>

namespace tut {
using namespace tut_dballe;

struct dba_db_import_shar
{
	// DB handle
	dba_db db;

	dba_db_import_shar() : db(NULL)
	{
		CHECKED(dba_init());
		CHECKED(dba_db_open("test", "enrico", "", &db));
	}

	~dba_db_import_shar()
	{
		if (db != NULL) dba_db_close(db);
		dba_shutdown();
		test_untag();
	}
};
TESTGRP(dba_db_import);

static int rep_cod_from_msg(dba_msg msg)
{
	switch (msg->type)
	{
		case MSG_SYNOP: return 1;
		case MSG_SHIP: return 10;
		case MSG_BUOY: return 9;
		case MSG_AIREP: return 12;
		case MSG_AMDAR: return 13;
		case MSG_ACARS: return 14;
		case MSG_PILOT: return 4;
		case MSG_TEMP: return 3;
		case MSG_TEMP_SHIP: return 11;
		case MSG_GENERIC: return 255;
	}
	return 255;
}

static dba_err msg_collector(dba_msg msg, void* data)
{
	vector<dba_msg>* vec = static_cast<vector<dba_msg>*>(data);
	(*vec).push_back(msg);
	return dba_error_ok();
}

static void track_different_msgs(dba_msg msg1, dba_msg msg2, const std::string& prefix)
{
	string fname1 = "/tmp/test-" + prefix + "1.bufr";
	string fname2 = "/tmp/test-" + prefix + "2.bufr";
	FILE* out1 = fopen(fname1.c_str(), "w");
	FILE* out2 = fopen(fname2.c_str(), "w");
	dba_msg_print(msg1, out1);
	dba_msg_print(msg2, out2);
	fclose(out1);
	fclose(out2);
	cerr << "Wrote mismatching messages to " << fname1 << " and " << fname2 << endl;
}

template<> template<>
void to::test<1>()
{
	const char* files[] = {
		"crex/test-mare0.crex",
		"crex/test-mare1.crex",
		"crex/test-mare2.crex",
		"crex/test-synop0.crex",
		"crex/test-synop1.crex",
		"crex/test-synop2.crex",
		"crex/test-synop3.crex",
		"crex/test-temp0.crex",
		NULL
	};

	/* Create the records we use to work */
	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msg msg = read_test_msg(files[i], CREX);

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, 0));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(msg)));
		CHECKED(dba_db_export(db, msg->type, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("temp0") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "crex-old");
		gen_ensure_equals(diffs, 0);

		/* Try the new export algorithm to see if it gives the same results */
		vector<dba_msg> msgs1;
		CHECKED(dba_db_query_msgs(db, msg->type, query, msg_collector, &msgs1));
		gen_ensure_equals(msgs1.size(), 1u);
		gen_ensure(msgs1[0] != NULL);
		diffs = 0;
		dba_msg_diff(msg, msgs1[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs1[0], "crex-new");
		gen_ensure_equals(diffs, 0);

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
		for (vector<dba_msg>::iterator i = msgs1.begin(); i != msgs1.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

template<> template<>
void to::test<2>()
{
	const char* files[] = {
		"bufr/obs0-1.22.bufr", 
		"bufr/obs0-3.504.bufr", 
		"bufr/obs1-9.2.bufr", 
		"bufr/obs1-11.16.bufr", 
		"bufr/obs1-13.36.bufr", 
		"bufr/obs1-19.3.bufr", 
		"bufr/obs1-21.1.bufr", 
		"bufr/obs2-101.16.bufr", 
		"bufr/obs2-102.1.bufr", 
		"bufr/obs2-91.2.bufr", 
		"bufr/obs4-142.13803.bufr", 
		"bufr/obs4-142.1.bufr", 
		"bufr/obs4-144.4.bufr", 
		"bufr/obs4-145.4.bufr", 
		"bufr/test-airep1.bufr",
		"bufr/test-temp1.bufr", 
		NULL
	};

	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msg msg = read_test_msg(files[i], BUFR);

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, 0));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(msg)));
		CHECKED(dba_db_export(db, msg->type, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("1-21") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "bufr-old");
		gen_ensure_equals(diffs, 0);

		/* Try the new export algorithm to see if it gives the same results */
		vector<dba_msg> msgs1;
		CHECKED(dba_db_query_msgs(db, msg->type, query, msg_collector, &msgs1));
		gen_ensure_equals(msgs1.size(), 1u);
		gen_ensure(msgs1[0] != NULL);
		diffs = 0;
		dba_msg_diff(msg, msgs1[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs1[0], "bufr-new");
		gen_ensure_equals(diffs, 0);

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
		for (vector<dba_msg>::iterator i = msgs1.begin(); i != msgs1.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

template<> template<>
void to::test<3>()
{
	const char* files[] = {
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs5-36.30.aof",
		"aof/obs6-32.1573.aof",
		NULL
	};
	/*	"aof/test-01.aof", */

	dba_record query;
	CHECKED(dba_record_create(&query));

	for (int i = 0; files[i] != NULL; i++)
	{
		test_tag(files[i]);
		dba_msg msg = read_test_msg(files[i], AOF);

		CHECKED(dba_db_reset(db, NULL));
		CHECKED(dba_import_msg(db, msg, 0));

		vector<dba_msg> msgs;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(msg)));
		CHECKED(dba_db_export(db, msg->type, query, msg_collector, &msgs));
		gen_ensure_equals(msgs.size(), 1u);
		gen_ensure(msgs[0] != NULL);

		/*
		if (string(files[i]).find("1-21") != string::npos)
		{
			dba_msg_print(msg, stderr);
			dba_msg_print(msgs[0], stderr);
		}
		*/

		// Compare the two dba_msg
		int diffs = 0;
		dba_msg_diff(msg, msgs[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs[0], "aof-old");
		gen_ensure_equals(diffs, 0);

		/* Try the new export algorithm to see if it gives the same results */
		vector<dba_msg> msgs1;
		CHECKED(dba_db_query_msgs(db, msg->type, query, msg_collector, &msgs1));
		gen_ensure_equals(msgs1.size(), 1u);
		gen_ensure(msgs1[0] != NULL);
		diffs = 0;
		dba_msg_diff(msg, msgs1[0], &diffs, stderr);
		if (diffs != 0) track_different_msgs(msg, msgs1[0], "aof-new");
		gen_ensure_equals(diffs, 0);

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
			dba_msg_delete(*i);
		for (vector<dba_msg>::iterator i = msgs1.begin(); i != msgs1.end(); i++)
			dba_msg_delete(*i);
	}

	dba_record_delete(query);
	test_untag();
}

// Check that multiple messages are correctly identified during export
template<> template<>
void to::test<4>()
{
	// msg1 has latitude 33.88
	// msg2 has latitude 46.22
	dba_msg msg1 = read_test_msg("bufr/obs0-1.22.bufr", BUFR);
	dba_msg msg2 = read_test_msg("bufr/obs0-3.504.bufr", BUFR);

	CHECKED(dba_db_reset(db, NULL));
	CHECKED(dba_import_msg(db, msg1, 0));
	CHECKED(dba_import_msg(db, msg2, 0));

	dba_record query;
	CHECKED(dba_record_create(&query));
	CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(msg1)));

	// Export with the old algorithm
	vector<dba_msg> msgs;
	CHECKED(dba_db_export(db, msg1->type, query, msg_collector, &msgs));
	gen_ensure_equals(msgs.size(), 2u);
	gen_ensure(msgs[0] != NULL);
	gen_ensure(msgs[1] != NULL);

	// Compare the two dba_msg
	int diffs = 0;
	dba_msg_diff(msg1, msgs[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg1, msgs[0], "synop1-old");
	gen_ensure_equals(diffs, 0);

	diffs = 0;
	dba_msg_diff(msg2, msgs[1], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg2, msgs[1], "synop2-old");
	gen_ensure_equals(diffs, 0);

	// Try the new export algorithm to see if it gives the same results
	vector<dba_msg> msgs1;
	CHECKED(dba_db_query_msgs(db, msg1->type, query, msg_collector, &msgs1));
	gen_ensure_equals(msgs1.size(), 2u);
	gen_ensure(msgs1[0] != NULL);
	gen_ensure(msgs1[1] != NULL);

	diffs = 0;
	dba_msg_diff(msg1, msgs1[0], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg1, msgs1[0], "synop1-new");
	gen_ensure_equals(diffs, 0);

	diffs = 0;
	dba_msg_diff(msg2, msgs1[1], &diffs, stderr);
	if (diffs != 0) track_different_msgs(msg2, msgs1[1], "synop2-new");
	gen_ensure_equals(diffs, 0);

	dba_msg_delete(msg1);
	dba_msg_delete(msg2);
	for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
		dba_msg_delete(*i);
	for (vector<dba_msg>::iterator i = msgs1.begin(); i != msgs1.end(); i++)
		dba_msg_delete(*i);

	dba_record_delete(query);
}

}

	/*
	CHECKED(dba_import_crex_file(db, "crex/test-synop1.crex"));
	CHECKED(dba_import_crex_file(db, "crex/test-synop2.crex"));
	CHECKED(dba_import_crex_file(db, "crex/test-synop3.crex"));
	CHECKED(dba_import_crex_file(db, "crex/test-mare0.crex"));
	CHECKED(dba_import_crex_file(db, "crex/test-mare1.crex"));
	CHECKED(dba_import_crex_file(db, "crex/test-mare2.crex"));
	*/

	/* Try importing a broken file */
	/*
	fail_unless(dba_import_crex_file(db, "crex/test-buoy-baddigit.crex") == DBA_ERROR);
	fail_unless(dba_error_get_code() == DBA_ERR_PARSE);
	*/

#if 0


	
	CHECKED(dba_record_create(&insert));
	CHECKED(dba_record_create(&result));
	CHECKED(dba_record_create(&qc));


	/* Fill in some data */
	FILLREC(insert, tdata0_ana);
	FILLREC(insert, tdata0);
	FILLREC(insert, tdata1_patch);

	/* Insert the record */
	CHECKED(dba_insert_new(db, insert));
	/* Check if duplicate updates are allowed by insert */
	CHECKED(dba_insert(db, insert));
	/* Check if duplicate updates are trapped by insert_new */
	fail_unless(dba_insert_new(db, insert) == DBA_ERROR);

	/* Insert another record (similar but not the same) */
	FILLREC(insert, tdata2_patch);
	CHECKED(dba_insert_new(db, insert));
	/* Check if duplicate updates are trapped */
	fail_unless(dba_insert_new(db, insert) == DBA_ERROR);

	/* Check dba_ana_* functions */
	{
		int count = 0;
		dba_cursor cursor;

		CHECKED(dba_ana_count(db, &count));
		fail_unless(count == 1);

		/* Iterate the anagraphic database */
		CHECKED(dba_ana_query(db, &cursor));

		/* There should be an item */
		CHECKED(dba_ana_cursor_next(cursor, result));

		/* Check that the results match */
		CHECKREC(result, tdata0_ana);

		/* There should be only one item */
		fail_unless(dba_ana_cursor_next(cursor, result) == DBA_ERROR);

		dba_cursor_delete(cursor);
	}

	/* Try a query */
	{
		dba_cursor cursor;

		dba_record_clear(query);

		/* Prepare a query */
		CHECKED(dba_seti(query, "latmin", 1000000));

		/* Make the query */
		CHECKED(dba_query(db, query, &cursor));

		/* See that a cursor has in fact been allocated */
		fail_unless(cursor != 0);

		/* There should be at least one item */
		CHECKED(dba_cursor_next(cursor, result));

		/* Check that the results match */
		CHECKREC(result, tdata0_ana);
		CHECKREC(result, tdata0);
		CHECKREC(result, tdata1_patch);

		/* There should be also another item */
		CHECKED(dba_cursor_next(cursor, result));

		/* Check that the results match */
		CHECKREC(result, tdata0_ana);
		CHECKREC(result, tdata0);
		CHECKREC(result, tdata2_patch);

		/* Now there should not be anything anymore */
		fail_unless(dba_cursor_next(cursor, result) == DBA_ERROR);

		/* Deallocate the cursor */
		dba_cursor_delete(cursor);
	}

	/* Delete one of the items */
	dba_record_clear(query);
	CHECKED(dba_seti(query, "minumin", 10));
	CHECKED(dba_delete(db, query));

	CHECKED(dba_seti(query, "min", 10));

	/* Querying for both exact time and a time range should fail */
	{
		dba_cursor cursor;
		fail_unless(dba_query(db, query, &cursor) == DBA_ERROR);
		dba_cursor_delete(cursor);
	}

	/* See if the results change */
	{
		dba_cursor cursor;
		dba_record_clear(query);
		CHECKED(dba_seti(query, "latmin", 1000000));
		CHECKED(dba_query(db, query, &cursor));
		CHECKED(dba_cursor_next(cursor, result));
		CHECKREC(result, tdata0_ana);
		CHECKREC(result, tdata0);
		CHECKREC(result, tdata1_patch);
		fail_unless(dba_cursor_next(cursor, result) == DBA_ERROR);
		dba_cursor_delete(cursor);
	}

	/* Insert some QC data */
	{
		dba_cursor cursor;
		int val;

		dba_record_clear(query);
		CHECKED(dba_seti(query, "latmin", 1000000));
		CHECKED(dba_query(db, query, &cursor));
		CHECKED(dba_cursor_next(cursor, result));
		dba_cursor_delete(cursor);

		/* Insert new QC data about this report */
		dba_record_clear(qc);
		dba_seti(qc, "B30001", 11);
		dba_seti(qc, "B30002", 22);
		dba_seti(qc, "B30004", 33);
		CHECKED(dba_qc_insert(db, result, DBA_VAR(0, 1, 93), qc));

		/* Query back the data */
		dba_record_clear(qc);
		CHECKED(dba_qc_query(db, result, DBA_VAR(0, 1, 93), qc));

		CHECKED(dba_enqi(qc, "B30001", &val));
		fail_unless(val == 11);
		CHECKED(dba_enqi(qc, "B30002", &val));
		fail_unless(val == 22);
		CHECKED(dba_enqi(qc, "B30004", &val));
		fail_unless(val == 33);

		/* Delete a couple of items */
		dba_unset(qc, "B30002");
		CHECKED(dba_qc_delete(db, result, DBA_VAR(0, 1, 93), qc));

		/* Query back the data */
		dba_record_clear(qc);
		CHECKED(dba_qc_query(db, result, DBA_VAR(0, 1, 93), qc));

		fail_unless(dba_enqi(qc, "B30001", &val) == DBA_ERROR);
		CHECKED(dba_enqi(qc, "B30002", &val));
		fail_unless(val == 22);
		fail_unless(dba_enqi(qc, "B30004", &val) == DBA_ERROR);
	}
#endif

#if 0
	{
		/* Insert some many things */
		int i;
		for (i = 20; i < 10000; i++)
		{
			CHECKED(dba_seti(rec, "B01050", i));
			CHECKED(dba_seti(rec, "l1", i));
			CHECKED(dba_insert(db, rec));
		}
	}
#endif

	/*
	dba_record_delete(qc);
	dba_record_delete(insert);
	dba_record_delete(result);
	*/

// vim:set ts=4 sw=4:
