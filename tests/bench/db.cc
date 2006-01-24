#include "Benchmark.h"

#include <dballe/dba_file.h>
#include <dballe/core/dba_record.h>
#include <dballe/db/dballe.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>

#include <stdlib.h>
#include <vector>

using namespace std;

const char* dsn = "test";
const char* user = "enrico";
const char* password = "";

/* Some utility random generator functions */

int rnd(int min, int max)
{
	return min + (int) (max * (rand() / (RAND_MAX + 1.0)));
}

double rnd(double min, double max)
{
	return min + (int) (max * (rand() / (RAND_MAX + 1.0)));
}

string rnd(int len)
{
	string res;
	int max = rnd(1, len);
	for (int i = 0; i < max; i++)
		res += (char)rnd('a', 'z');
	return res;
}

bool rnd(double prob)
{
	return (rnd(0, 100) < prob*100) ? true : false;
}

const static dba_varcode varcodes[] = {
	DBA_VAR(0,  1,   1),
	DBA_VAR(0,  1,   2),
	DBA_VAR(0,  1,   8),
	DBA_VAR(0,  1,  11),
	DBA_VAR(0,  1,  12),
	DBA_VAR(0,  1,  13),
	DBA_VAR(0,  2,   1),
	DBA_VAR(0,  2,   2),
	DBA_VAR(0,  2,   5),
	DBA_VAR(0,  2,  11),
	DBA_VAR(0,  2,  12),
	DBA_VAR(0,  2,  61),
	DBA_VAR(0,  2,  62),
	DBA_VAR(0,  2,  63),
	DBA_VAR(0,  2,  70),
	DBA_VAR(0,  4,   1),
	DBA_VAR(0,  4,   2),
	DBA_VAR(0,  4,   3),
	DBA_VAR(0,  4,   4),
	DBA_VAR(0,  4,   5),
	DBA_VAR(0,  5,   1),
	DBA_VAR(0,  6,   1),
	DBA_VAR(0,  7,   1),
	DBA_VAR(0,  7,   2),
	DBA_VAR(0,  7,   4),
	DBA_VAR(0,  7,  31),
	DBA_VAR(0,  8,   1),
	DBA_VAR(0,  8,   4),
	DBA_VAR(0,  8,  21),
	DBA_VAR(0, 10,   3),
	DBA_VAR(0, 10,   4),
	DBA_VAR(0, 10,  51),
	DBA_VAR(0, 10,  61),
	DBA_VAR(0, 10,  63),
	DBA_VAR(0, 10, 197),
	DBA_VAR(0, 11,   1),
	DBA_VAR(0, 11,   2),
	DBA_VAR(0, 11,   3),
	DBA_VAR(0, 11,   4),
};

class db_work : public Benchmark
{
	vector<dba_record> reused_pseudoana;
	vector<dba_record> reused_context;

	dba_err fill_pseudoana(dba_record rec)
	{
		dba_record ana;
		if (reused_pseudoana.empty() || rnd(0.3))
		{
			DBA_RUN_OR_RETURN(dba_record_create(&ana));

			/* Pseudoana */
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LAT, rnd(-180, 180)));
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LON, rnd(-180, 180)));
			if (rnd(0.8))
			{
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_BLOCK, rnd(0, 99)));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_STATION, rnd(0, 999)));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 0));
			} else {
				DBA_RUN_OR_RETURN(dba_record_key_setc(ana, DBA_KEY_IDENT, rnd(10).c_str()));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 1));
			}
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_HEIGHT, rnd(1, 3000)));
			DBA_RUN_OR_RETURN(dba_record_key_setc(ana, DBA_KEY_NAME, rnd(20).c_str()));

			reused_pseudoana.push_back(ana);
		} else {
			ana = reused_pseudoana[rnd(0, reused_pseudoana.size() - 1)];
		}
		DBA_RUN_OR_RETURN(dba_record_add(rec, ana));
		return dba_error_ok();
	}

	dba_err fill_context(dba_record rec)
	{
		dba_record ctx;
		if (reused_context.empty() || rnd(0.7))
		{
			DBA_RUN_OR_RETURN(dba_record_create(&ctx));

			/* Context */
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_YEAR, rnd(2002, 2005)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MONTH, rnd(1, 12)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_DAY, rnd(1, 31)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_HOUR, rnd(0, 23)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MIN, rnd(0, 59)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_LEVELTYPE, rnd(0, 300)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L1, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L2, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_PINDICATOR, rnd(0, 300)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P1, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P2, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_REP_COD, rnd(0, 20)));

			reused_context.push_back(ctx);
		} else {
			ctx = reused_context[rnd(0, reused_context.size() - 1)];
		}
		DBA_RUN_OR_RETURN(dba_record_add(rec, ctx));
		return dba_error_ok();
	}

	dba_err fill_record(dba_record rec)
	{
		DBA_RUN_OR_RETURN(fill_pseudoana(rec));
		DBA_RUN_OR_RETURN(fill_context(rec));
		DBA_RUN_OR_RETURN(dba_record_var_setc(rec, varcodes[rnd(0, sizeof(varcodes) / sizeof(dba_varcode))], "11111"));
		return dba_error_ok();
	}

protected:
	virtual dba_err main()
	{
		static const int iterations = 500;

		dba_record rec;
		DBA_RUN_OR_RETURN(dba_record_create(&rec));

		dba db;
		DBA_RUN_OR_RETURN(dba_open(dsn, user, password, &db));

		// Reset the database
		DBA_RUN_OR_RETURN(dba_reset(db, NULL));
		timing("reset the database");

		// Insert random data in the database
		for (int i = 0; i < iterations; i++)
		{
			dba_record_clear(rec);
			DBA_RUN_OR_RETURN(fill_record(rec));
			//dba_record_print(rec, stderr);
			DBA_RUN_OR_RETURN(dba_insert(db, rec));
		}

		timing("inserted %d random records in the database", iterations);

		dba_close(db);
		dba_record_delete(rec);

		return dba_error_ok();
	}

public:
	db_work() : Benchmark("read") {}
	~db_work()
	{
		for (vector<dba_record>::iterator i = reused_pseudoana.begin();
				i != reused_pseudoana.end(); i++)
			dba_record_delete(*i);
		for (vector<dba_record>::iterator i = reused_context.begin();
				i != reused_context.end(); i++)
			dba_record_delete(*i);
	}
};

#if 0
class db_work : public Benchmark
{
	class dba_raw_consumer
	{
	public:
		virtual ~dba_raw_consumer() {}

		virtual dba_err consume(dba_encoding type, dba_rawmsg raw) = 0;
	};

	class collect_bufrex : public dba_raw_consumer
	{
	public:
		vector<bufrex_raw> msgs;

		virtual ~collect_bufrex()
		{
			for (vector<bufrex_raw>::iterator i = msgs.begin(); i != msgs.end(); i++)
				bufrex_raw_delete(*i);
		}
			
		virtual dba_err consume(dba_encoding type, dba_rawmsg raw)
		{
			dba_err err = DBA_OK;
			bufrex_raw msg;

			switch (type)
			{
				case BUFR:
					DBA_RUN_OR_RETURN(bufrex_raw_create(&msg, BUFREX_BUFR));
					break;
				case CREX:
					DBA_RUN_OR_RETURN(bufrex_raw_create(&msg, BUFREX_CREX));
					break;
				default:
					return dba_error_consistency("unhandled message type");
			}
			DBA_RUN_OR_GOTO(fail, bufrex_raw_decode(msg, raw));
			msgs.push_back(msg);

			return dba_error_ok();

		fail:
			bufrex_raw_delete(msg);
			return err;
		}
	};
	
	dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons)
	{
		dba_err err = DBA_OK;
		dba_file file = 0;
		dba_rawmsg raw = 0;
		int found;

		DBA_RUN_OR_GOTO(cleanup, dba_file_create(&file, type, name.c_str(), "r"));
		DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_create(&raw));

		DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
		while (found)
		{
			DBA_RUN_OR_GOTO(cleanup, cons.consume(type, raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
		}

	cleanup:
		if (file) dba_file_delete(file);
		if (raw) dba_rawmsg_delete(raw);
		return err == DBA_OK ? dba_error_ok() : err;
	}

protected:
	virtual dba_err main()
	{
		static const int iterations = 500;
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
			"crex/test-mare0.crex", 
			"crex/test-mare1.crex", 
			"crex/test-mare2.crex", 
			"crex/test-synop0.crex", 
			"crex/test-synop1.crex", 
			"crex/test-synop2.crex", 
			"crex/test-synop3.crex", 
			"crex/test-temp0.crex", 
		};

		collect_bufrex msgbase;

		unsigned int count;

		// Read data from a collection of files
		for (int i = 0; i < iterations; i++)
			for (count = 0; count < sizeof(bufr_files) / sizeof(const char*); count++)
				DBA_RUN_OR_RETURN(read_file(BUFR, bufr_files[count], msgbase));
		timing("read and parse %d BUFR messages of various kinds", count * iterations);

		for (int i = 0; i < iterations; i++)
			for (count = 0; count < sizeof(crex_files) / sizeof(const char*); count++)
				DBA_RUN_OR_RETURN(read_file(CREX, crex_files[count], msgbase));
		timing("read and parse %d CREX messages of various kinds", count * iterations);
		
		for (vector<bufrex_raw>::iterator i = msgbase.msgs.begin();
				i != msgbase.msgs.end(); i++)
		{
			dba_msg msg = 0;
			DBA_RUN_OR_RETURN(bufrex_raw_to_msg(*i, &msg));
			dba_msg_delete(msg);
		}
		timing("interpreting %d bufrex_raw messages", msgbase.msgs.size());

		return dba_error_ok();
	}

public:
	bufrex_read() : Benchmark("read") {}
};
#endif

class top : public Benchmark
{
public:
	top() : Benchmark("db")
	{
		addChild(new db_work());
	}
};

static RegisterRoot r(new top());

/* vim:set ts=4 sw=4: */
