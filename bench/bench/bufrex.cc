#include "Benchmark.h"
#include <tests/test-utils.h>

#include <dballe/dba_file.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>

#include <vector>

using namespace std;
using namespace tut_dballe;

class bufrex_read : public Benchmark
{
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

		bufrex_vector msgbase;

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
		
		for (vector<bufrex_raw>::iterator i = msgbase.begin();
				i != msgbase.end(); i++)
		{
			dba_msg msg = 0;
			DBA_RUN_OR_RETURN(bufrex_raw_to_msg(*i, &msg));
			dba_msg_delete(msg);
		}
		timing("interpreting %d bufrex_raw messages", msgbase.size());

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
