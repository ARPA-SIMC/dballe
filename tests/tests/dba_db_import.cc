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
		CHECKED(dba_db_create("test", "enrico", "", &db));
	}

	~dba_db_import_shar()
	{
		if (db != NULL) dba_db_delete(db);
		dba_shutdown();
		test_untag();
	}
};
TESTGRP(dba_db_import);

static int rep_cod_from_msg(dba_msg_type type)
{
	switch (type)
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

static int rep_cod_from_msg(dba_msg msg)
{
	return rep_cod_from_msg(msg->type);
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

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
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

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
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

		dba_msg_delete(msg);
		for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
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

	dba_msg_delete(msg1);
	dba_msg_delete(msg2);
	for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
		dba_msg_delete(*i);

	dba_record_delete(query);
}

// Check that all imported messages are found on export
template<> template<>
void to::test<5>()
{
	generator gen;

	CHECKED(dba_db_reset(db, NULL));

	/* Fix the seed so that we always get predictable results */
	srand(1);
	
	/* Import 100 random messages */
	for (int i = 0; i < 100; i++)
	{
		dba_msg msg;
		CHECKED(dba_msg_create(&msg));
		gen.fill_message(msg, rnd(0.8));
		CHECKED(dba_import_msg(db, msg, 0));
		dba_msg_delete(msg);
	}

	// Prepare the query
	dba_record query;
	CHECKED(dba_record_create(&query));
	CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, 255));

	// Export with the old algorithm
	vector<dba_msg> msgs;
	CHECKED(dba_db_export(db, MSG_GENERIC, query, msg_collector, &msgs));
	gen_ensure_equals(msgs.size(), 100u);
	for (vector<dba_msg>::iterator i = msgs.begin(); i != msgs.end(); i++)
		dba_msg_delete(*i);

	dba_record_delete(query);
}

static dba_err msg_counter(dba_msg msg, void* data)
{
	dba_msg_delete(msg);
	(*(int*)data)++;
	return dba_error_ok();
}

// Check that the right messages are exported
template<> template<>
void to::test<6>()
{
	msg_vector msgs;

	// All the various input messages with unique data
	static const char* bufr_files[] = {
		"bufr/obs0-1.22.bufr",
		"bufr/obs0-3.504.bufr",
		"bufr/obs1-11.16.bufr",
		"bufr/obs1-13.36.bufr",
		"bufr/obs1-19.3.bufr",
		"bufr/obs1-21.1.bufr",
		"bufr/obs1-9.2.bufr",
		"bufr/obs2-101.16.bufr",
		"bufr/obs2-102.1.bufr",
		"bufr/obs2-91.2.bufr",
		"bufr/obs4-142.13803.bufr",
		"bufr/obs4-142.1.bufr",
		"bufr/obs4-144.4.bufr",
		"bufr/obs4-145.4.bufr",
	};
	static const char* crex_files[] = {
		"crex/test-mare1.crex",
		"crex/test-mare2.crex",
		"crex/test-synop0.crex",
		"crex/test-synop1.crex",
		"crex/test-synop2.crex",
		"crex/test-synop3.crex",
		"crex/test-temp0.crex",
	};
	static const char* aof_files[] = {
		"aof/obs1-14.63.aof",
		"aof/obs1-21.1.aof",
		"aof/obs1-24.2104.aof",
		"aof/obs1-24.34.aof",
		"aof/obs2-144.2198.aof",
		"aof/obs4-165.2027.aof",
		"aof/obs5-35.61.aof",
		"aof/obs6-32.1573.aof",
	};

	for (size_t i = 0; i < sizeof(bufr_files) / sizeof(const char*); i++)
		CHECKED(read_file(BUFR, bufr_files[i], msgs));
	for (size_t i = 0; i < sizeof(crex_files) / sizeof(const char*); i++)
		CHECKED(read_file(CREX, crex_files[i], msgs));
	for (size_t i = 0; i < sizeof(aof_files) / sizeof(const char*); i++)
		CHECKED(read_file(AOF, aof_files[i], msgs));

	CHECKED(dba_db_reset(db, NULL));

	map<dba_msg_type, int> rep_cods;
	for (msg_vector::const_iterator i = msgs.begin();
			i != msgs.end(); i++)
	{
		CHECKED(dba_import_msg(db, *i, 1));
		rep_cods[(*i)->type]++;
	}

	dba_record query;
	CHECKED(dba_record_create(&query));
	for (map<dba_msg_type, int>::const_iterator i = rep_cods.begin(); i != rep_cods.end(); i++)
	{
		test_tag(dba_msg_type_name(i->first));

		int count = 0;
		CHECKED(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(i->first)));
		CHECKED(dba_db_export(db, i->first, query, msg_counter, &count));
		gen_ensure_equals(count, i->second);
	}

	test_untag();
}

}

// vim:set ts=4 sw=4:
