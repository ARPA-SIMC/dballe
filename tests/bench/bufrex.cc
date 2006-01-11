#include "Benchmark.h"

#include <dballe/dba_file.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>

#include <vector>

using namespace std;

class bufrex_read : public Benchmark
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

class top : public Benchmark
{
public:
	top() : Benchmark("bufrex")
	{
		addChild(new bufrex_read());
	}
};

static RegisterRoot r(new top());

/* vim:set ts=4 sw=4: */
