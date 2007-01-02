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

#include "Benchmark.h"
#include <dballe/tests/test-utils.h>

#include <dballe/core/record.h>
#include <dballe/db/dba_db.h>
#include <dballe/db/import.h>
#include <dballe/db/export.h>

#include <stdlib.h>
#include <vector>

using namespace std;
using namespace tut_dballe;

namespace bench_db {

const char* dsn = "test";
const char* user = "enrico";
const char* password = "";

static dba_err msg_counter(dba_msgs msgs, void* data)
{
	dba_msgs_delete(msgs);
	(*(int*)data)++;
	return dba_error_ok();
}

class db_work : public Benchmark
{
	generator gen;
	bool fast;

protected:
	virtual dba_err main()
	{
		static const int iterations = 3000;

		dba_record rec;
		DBA_RUN_OR_RETURN(dba_record_create(&rec));

		dba_db db;
		DBA_RUN_OR_RETURN(dba_db_create(dsn, user, password, &db));

		// Reset the database
		DBA_RUN_OR_RETURN(dba_db_reset(db, NULL));
		timing("reset the database");

		// Insert random data in the database
		for (int i = 0; i < iterations; i++)
		{
			dba_record_clear(rec);
			DBA_RUN_OR_RETURN(gen.fill_record(rec));
			//dba_record_print(rec, stderr);
			DBA_RUN_OR_RETURN(dba_db_insert(db, rec, 1, 1, NULL, NULL));
		}

		timing("inserted %d random records in the database", iterations);

		msg_vector msgs;

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
		static const char* aof_files[] = {
			"aof/obs1-14.63.aof",
			"aof/obs1-21.1.aof",
			"aof/obs1-24.2104.aof",
			"aof/obs1-24.34.aof",
			"aof/obs2-144.2198.aof",
			"aof/obs4-165.2027.aof",
			"aof/obs5-35.61.aof",
			"aof/obs5-36.30.aof",
			"aof/obs6-32.1573.aof",
		};

		for (size_t i = 0; i < sizeof(bufr_files) / sizeof(const char*); i++)
			DBA_RUN_OR_RETURN(read_file(BUFR, string("../tests/") + bufr_files[i], msgs));
		for (size_t i = 0; i < sizeof(crex_files) / sizeof(const char*); i++)
			DBA_RUN_OR_RETURN(read_file(CREX, string("../tests/") + crex_files[i], msgs));
		for (size_t i = 0; i < sizeof(aof_files) / sizeof(const char*); i++)
			DBA_RUN_OR_RETURN(read_file(AOF, string("../tests/") + aof_files[i], msgs));

		DBA_RUN_OR_RETURN(dba_db_reset(db, NULL));
		timing("reset the database");

		for (msg_vector::const_iterator i = msgs.begin();
				i != msgs.end(); i++)
			for (int j = 0; j < (*i)->len; ++j)
			DBA_RUN_OR_RETURN(dba_import_msg(db, (*i)->msgs[j], -1, 1, fast));

		timing("inserted %d messages in the database", msgs.size());

		dba_record query;
		int count = 0;
		DBA_RUN_OR_RETURN(dba_record_create(&query));
		for (msg_vector::const_iterator i = msgs.begin();
				i != msgs.end(); i++)
			for (int j = 0; j < (*i)->len; ++j)
			{
				DBA_RUN_OR_RETURN(dba_record_key_seti(query, DBA_KEY_REP_COD, dba_msg_repcod_from_type((*i)->msgs[j]->type)));
				DBA_RUN_OR_RETURN(dba_db_export(db, query, msg_counter, &count));
			}

		timing("exported %d messages from the database, new style", count);

#if 0
		count = 0;
		for (msg_vector::const_iterator i = msgs.begin();
				i != msgs.end(); i++)
		{
	        DBA_RUN_OR_RETURN(dba_record_key_seti(query, DBA_KEY_REP_COD, rep_cod_from_msg(*i)));
			DBA_RUN_OR_RETURN(dba_db_export_old(db, (*i)->type, query, msg_counter, &count));
		}

		timing("exported %d messages from the database, old style", count);
#endif

		dba_db_delete(db);
		dba_record_delete(rec);
		dba_record_delete(query);

		return dba_error_ok();
	}

public:
	db_work(bool fast = false) : Benchmark(fast ? "readfast" : "read"), fast(fast) {}
	~db_work() {}
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
		addChild(new db_work(true));
	}
};

static RegisterRoot r(new top());

}

/* vim:set ts=4 sw=4: */
