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

#include <dballe/core/record.h>

#include <vector>

using namespace std;

namespace bench_dba_record {

class create : public Benchmark
{
protected:
	virtual dba_err main()
	{
		static const int iterations = 1500000;

		for (int i = 0; i < iterations; i++)
		{
			dba_record rec;
			DBA_RUN_OR_RETURN(dba_record_create(&rec));
			dba_record_delete(rec);
		}
		timing("%d dba_record_create", iterations);

		return dba_error_ok();
	}

public:
	create() : Benchmark("create") {}
};

class enqset : public Benchmark
{
	vector<dba_keyword> int_keys;
	vector<dba_keyword> float_keys;
	vector<dba_keyword> char_keys;
	vector<dba_varcode> int_vars;
	vector<dba_varcode> float_vars;
	vector<dba_varcode> char_vars;

protected:
	virtual dba_err main()
	{
		static const int iterations = 1500000;
		int ival; double dval; const char* cval; int bval;
		dba_record rec;

		DBA_RUN_OR_RETURN(dba_record_create(&rec));

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = int_keys.begin(); j != int_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_seti(rec, *j, 42));
		timing("%d dba_record_key_seti", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = float_keys.begin(); j != float_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_setd(rec, *j, 42.42));
		timing("%d dba_record_key_setd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = char_keys.begin(); j != char_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_setc(rec, *j, "foobar"));
		timing("%d dba_record_key_setc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = int_keys.begin(); j != int_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, *j, &ival, &bval));
		timing("%d dba_record_key_enqi", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = float_keys.begin(); j != float_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqd(rec, *j, &dval, &bval));
		timing("%d dba_record_key_enqd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_keyword>::const_iterator j = char_keys.begin(); j != char_keys.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_key_enqc(rec, *j, &cval));
		timing("%d dba_record_key_enqc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = int_vars.begin(); j != int_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_seti(rec, *j, 42));
		timing("%d dba_record_var_seti", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = float_vars.begin(); j != float_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_setd(rec, *j, 42.42));
		timing("%d dba_record_var_setd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = char_vars.begin(); j != char_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_setc(rec, *j, "foobar"));
		timing("%d dba_record_var_setc", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = int_vars.begin(); j != int_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqi(rec, *j, &ival, &bval));
		timing("%d dba_record_var_enqi", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = float_vars.begin(); j != float_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqd(rec, *j, &dval, &bval));
		timing("%d dba_record_var_enqd", iterations);

		for (int i = 0; i < iterations; i++)
			for (vector<dba_varcode>::const_iterator j = char_vars.begin(); j != char_vars.end(); j++)
				DBA_RUN_OR_RETURN(dba_record_var_enqc(rec, *j, &cval));
		timing("%d dba_record_var_enqc", iterations);

		dba_record_delete(rec);

		return dba_error_ok();
	}

public:
	enqset() : Benchmark("enqset")
	{
		int_keys.push_back(DBA_KEY_PRIORITY);
		int_keys.push_back(DBA_KEY_PRIOMAX);
		int_keys.push_back(DBA_KEY_PRIOMIN);
		int_keys.push_back(DBA_KEY_LEVELTYPE);
		int_keys.push_back(DBA_KEY_CONTEXT_ID);
		float_keys.push_back(DBA_KEY_LAT);
		float_keys.push_back(DBA_KEY_LON);
		float_keys.push_back(DBA_KEY_LATMAX);
		float_keys.push_back(DBA_KEY_LATMIN);
		float_keys.push_back(DBA_KEY_LONMAX);
		char_keys.push_back(DBA_KEY_REP_MEMO);
		char_keys.push_back(DBA_KEY_IDENT);
		char_keys.push_back(DBA_KEY_DATETIME);
		char_keys.push_back(DBA_KEY_VAR);
		char_keys.push_back(DBA_KEY_VARLIST);
		int_vars.push_back(  DBA_VAR(0, 1,   1));
		int_vars.push_back(  DBA_VAR(0, 1,   2));
		int_vars.push_back(  DBA_VAR(0, 4,   1));
		int_vars.push_back(  DBA_VAR(0, 4,   2));
		int_vars.push_back(  DBA_VAR(0, 4,   3));
		float_vars.push_back(DBA_VAR(0, 2,   5));
		float_vars.push_back(DBA_VAR(0, 2,  63));
		float_vars.push_back(DBA_VAR(0, 5,   1));
		float_vars.push_back(DBA_VAR(0, 7,   2));
		float_vars.push_back(DBA_VAR(0, 14, 16));
		char_vars.push_back( DBA_VAR(0, 1,   8));
		char_vars.push_back( DBA_VAR(0, 1,  11));
		char_vars.push_back( DBA_VAR(0, 1,   8));
		char_vars.push_back( DBA_VAR(0, 1,  11));
		char_vars.push_back( DBA_VAR(0, 1,   8));
	}
};

class top : public Benchmark
{
public:
	top() : Benchmark("dba_record")
	{
		addChild(new create());
		addChild(new enqset());
	}
};

static RegisterRoot r(new top());

}

/* vim:set ts=4 sw=4: */
