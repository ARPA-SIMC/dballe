/*
 * fortran/commonapi - Common parts of all Fortran API implementations
 *
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

//#define _GNU_SOURCE
/* _GNU_SOURCE is defined to have asprintf */

#include "commonapi.h"
#include <dballe/core/aliases.h>
#include <dballe/msg/formatter.h>
#include <stdio.h>	// snprintf
#include <limits>
#include <cstdlib>
#include <cstring>
#include <strings.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {

CommonAPIImplementation::CommonAPIImplementation()
	: perms(0), qc_iter(-1), qc_count(0), last_set_code(0)
{
}

CommonAPIImplementation::~CommonAPIImplementation()
{
}

void CommonAPIImplementation::set_permissions(const char* anaflag, const char* dataflag, const char* attrflag)
{
	if (strcasecmp("read",	anaflag) == 0)
		perms |= PERM_ANA_RO;
	if (strcasecmp("write",	anaflag) == 0)
		perms |= PERM_ANA_WRITE;
	if (strcasecmp("read",	dataflag) == 0)
		perms |= PERM_DATA_RO;
	if (strcasecmp("add",	dataflag) == 0)
		perms |= PERM_DATA_ADD;
	if (strcasecmp("write",	dataflag) == 0)
		perms |= PERM_DATA_WRITE;
	if (strcasecmp("read",	attrflag) == 0)
		perms |= PERM_ATTR_RO;
	if (strcasecmp("add",	attrflag) == 0)
		perms |= PERM_ATTR_ADD;
	if (strcasecmp("write",	attrflag) == 0)
		perms |= PERM_ATTR_WRITE;

	if ((perms & (PERM_ANA_RO | PERM_ANA_WRITE)) == 0)
		throw error_consistency("pseudoana should be opened in either 'read' or 'write' mode");
	if ((perms & (PERM_DATA_RO | PERM_DATA_ADD | PERM_DATA_WRITE)) == 0)
		throw error_consistency("data should be opened in one of 'read', 'add' or 'write' mode");
	if ((perms & (PERM_ATTR_RO | PERM_ATTR_ADD | PERM_ATTR_WRITE)) == 0)
		throw error_consistency("attr should be opened in one of 'read', 'add' or 'write' mode");

	if (perms & PERM_ANA_RO && perms & PERM_DATA_WRITE)
		throw error_consistency("when data is 'write' ana must also be set to 'write', because deleting data can potentially also delete pseudoana");
	if (perms & PERM_ATTR_RO && perms & PERM_DATA_WRITE)
		throw error_consistency("when data is 'write' attr must also be set to 'write', because deleting data also delete its attributes");
}

Record& CommonAPIImplementation::choose_input_record(const char*& param)
{
	switch (param[0])
	{
		case '*':
			param = param + 1;
			return qcinput;
		default:
			return input;
	}
}

Record& CommonAPIImplementation::choose_output_record(const char*& param)
{
	switch (param[0])
	{
		case '*':
			param = param + 1;
			return qcoutput;
		default:
			return output;
	}
}

void CommonAPIImplementation::test_input_to_output()
{
	output = input;
}

int CommonAPIImplementation::enqi(const char* param)
{
	Record& rec = choose_output_record(param);
	if (const Var* var = rec.peek(param))
		return var->enqi();
	else
		return missing_int;
}

signed char CommonAPIImplementation::enqb(const char* param)
{
	Record& rec = choose_output_record(param);
	if (const Var* var = rec.peek(param))
	{
		int value = var->enqi();
		if (value < numeric_limits<signed char>::min()
		 || value > numeric_limits<signed char>::max())
			error_consistency::throwf("value queried (%d) does not fit in a byte", value);
		return value;
	}
	else
		return missing_byte;

}

float CommonAPIImplementation::enqr(const char* param)
{
	Record& rec = choose_output_record(param);
	if (const Var* var = rec.peek(param))
	{
		double value = var->enqd();

		if (value < -numeric_limits<float>::max()
		 || value > numeric_limits<float>::max())
			error_consistency::throwf("value queried (%f) does not fit in a real", value);
		return value;
	} else
		return missing_float;
}

double CommonAPIImplementation::enqd(const char* param)
{
	Record& rec = choose_output_record(param);
	if (const Var* var = rec.peek(param))
		return var->enqd();
	else
		return missing_double;
}

const char* CommonAPIImplementation::enqc(const char* param)
{
	Record& rec = choose_output_record(param);
	return rec.peek_value(param);
}

static Varcode checkvar(const char* param)
{
	if (param[0] == 'B')
		return WR_STRING_TO_VAR(param + 1);
	return varcode_alias_resolve(param);
}

static dba_keyword prepare_key_change(Record& rec, const char* param)
{
	dba_keyword key = Record::keyword_byname(param);
	switch (key)
	{
		case DBA_KEY_ERROR:
			error_notfound::throwf("looking for misspelled parameter \"%s\"", param);
			break;
		case DBA_KEY_LAT:
		case DBA_KEY_LON:
			rec.key_unset(DBA_KEY_ANA_ID);
			break;
		case DBA_KEY_ANA_ID:
			rec.key_unset(DBA_KEY_LAT);
			rec.key_unset(DBA_KEY_LON);
			break;
		default: break;
	}
	return key;
}

void CommonAPIImplementation::seti(const char* param, int value)
{
	Record& rec = choose_input_record(param);
	Varcode code = checkvar(param);

	if (code)
	{
		rec.var(code).seti(value);
		last_set_code = code;
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		rec.key(key).seti(value);
	}
}

void CommonAPIImplementation::setb(const char* param, signed char value)
{
	return seti(param, value);
}

void CommonAPIImplementation::setr(const char* param, float value)
{
	return setd(param, value);
}

void CommonAPIImplementation::setd(const char* param, double value)
{
	Record& rec = choose_input_record(param);
	Varcode code = checkvar(param);

	if (code)
	{
		last_set_code = code;
		rec.var(code).setd(value);
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		rec.key(key).setd(value);
	}
}

void CommonAPIImplementation::setc(const char* param, const char* value)
{
	Record& rec = choose_input_record(param);
	Varcode code = checkvar(param);

	if (code)
	{
		last_set_code = code;
		rec.var(code).setc(value);
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		rec.key(key).setc(value);
	}
}

void CommonAPIImplementation::setcontextana()
{
	input.set_ana_context();
}

static inline void enqi_or_missing(int& out, Record& rec, dba_keyword key)
{
	if (const Var* var = rec.key_peek(key))
		out = var->enqi();
	else
		out = API::missing_int;
}

static inline void seti_or_missing(Record& rec, dba_keyword key, int val)
{
	if (val == API::missing_int)
		rec.unset(key);
	else
		rec.set(key, val);
}

void CommonAPIImplementation::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
	enqi_or_missing(ltype1, output, DBA_KEY_LEVELTYPE1);
	enqi_or_missing(l1, output, DBA_KEY_L1);
	enqi_or_missing(ltype2, output, DBA_KEY_LEVELTYPE2);
	enqi_or_missing(l2, output, DBA_KEY_L2);
}

void CommonAPIImplementation::setlevel(int ltype1, int l1, int ltype2, int l2)
{
	seti_or_missing(input, DBA_KEY_LEVELTYPE1, ltype1);
	seti_or_missing(input, DBA_KEY_L1, l1);
	seti_or_missing(input, DBA_KEY_LEVELTYPE2, ltype2);
	seti_or_missing(input, DBA_KEY_L2, l2);
}

void CommonAPIImplementation::enqtimerange(int& ptype, int& p1, int& p2)
{
	enqi_or_missing(ptype, output, DBA_KEY_PINDICATOR);
	enqi_or_missing(p1, output, DBA_KEY_P1);
	enqi_or_missing(p2, output, DBA_KEY_P2);
}

void CommonAPIImplementation::settimerange(int ptype, int p1, int p2)
{
	seti_or_missing(input, DBA_KEY_PINDICATOR, ptype);
	seti_or_missing(input, DBA_KEY_P1, p1);
	seti_or_missing(input, DBA_KEY_P2, p2);
}

void CommonAPIImplementation::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
	enqi_or_missing(year, output, DBA_KEY_YEAR);
	enqi_or_missing(month, output, DBA_KEY_MONTH);
	enqi_or_missing(day, output, DBA_KEY_DAY);
	enqi_or_missing(hour, output, DBA_KEY_HOUR);
	enqi_or_missing(min, output, DBA_KEY_MIN);
	enqi_or_missing(sec, output, DBA_KEY_SEC);
}

void CommonAPIImplementation::setdate(int year, int month, int day, int hour, int min, int sec)
{
	seti_or_missing(input, DBA_KEY_YEAR, year);
	seti_or_missing(input, DBA_KEY_MONTH, month);
	seti_or_missing(input, DBA_KEY_DAY, day);
	seti_or_missing(input, DBA_KEY_HOUR, hour);
	seti_or_missing(input, DBA_KEY_MIN, min);
	seti_or_missing(input, DBA_KEY_SEC, sec);
}

void CommonAPIImplementation::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
	input.set(DBA_KEY_YEARMIN, year);
	input.set(DBA_KEY_MONTHMIN, month);
	input.set(DBA_KEY_DAYMIN, day);
	input.set(DBA_KEY_HOURMIN, hour);
	input.set(DBA_KEY_MINUMIN, min);
	input.set(DBA_KEY_SECMIN, sec);
}

void CommonAPIImplementation::setdatemax(int year, int month, int day, int hour, int min, int sec)
{

	input.set(DBA_KEY_YEARMAX, year);
	input.set(DBA_KEY_MONTHMAX, month);
	input.set(DBA_KEY_DAYMAX, day);
	input.set(DBA_KEY_HOURMAX, hour);
	input.set(DBA_KEY_MINUMAX, min);
	input.set(DBA_KEY_SECMAX, sec);
}

void CommonAPIImplementation::unset(const char* param)
{
	Record& rec = choose_input_record(param);
	Varcode code = checkvar(param);

	if (code)
	{
		rec.var_unset(code);
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		rec.key_unset(key);
	}
}

void CommonAPIImplementation::unsetall()
{
	clear_qcinput();
	input.clear();
}

const char* CommonAPIImplementation::spiegal(int ltype1, int l1, int ltype2, int l2)
{
	cached_spiega = describe_level_or_layer(ltype1, l1, ltype2, l2);
	return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegat(int ptype, int p1, int p2)
{
	cached_spiega = describe_trange(ptype, p1, p2);
	return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegab(const char* varcode, const char* value)
{
	Varinfo info = varinfo(WR_STRING_TO_VAR(varcode + 1));
	Var var(info, value);

	char buf[1024];
	if (info->is_string())
		snprintf(buf, 1023, "%s (%s) %s", var.enqc(), info->unit, info->desc);
	else
		snprintf(buf, 1023, "%.*f (%s) %s", info->scale > 0 ? info->scale : 0, var.enqd(), info->unit, info->desc);
	cached_spiega = buf;
	return cached_spiega.c_str();
}

const char* CommonAPIImplementation::ancora()
{
	static char parm[10];

	if (qc_iter < 0 || (unsigned)qc_iter >= qcoutput.vars().size())
		throw error_notfound("reading a QC item");

	Varcode var = qcoutput.vars()[qc_iter]->code();
	snprintf(parm, 10, "*B%02d%03d", WR_VAR_X(var), WR_VAR_Y(var));

	/* Get next value from qc */
	++qc_iter;

	return parm;
}

void CommonAPIImplementation::get_referred_data_id(int* id_context, Varcode* id_var) const
{
	/* Read context ID */
	if (const Var* var = qcinput.key_peek(DBA_KEY_CONTEXT_ID))
		*id_context = var->enqi();
	else
		throw error_notfound("looking for variable context id");

	/* Read variable ID */
	if (const char* val = qcinput.key_peek_value(DBA_KEY_VAR_RELATED))
		*id_var = WR_STRING_TO_VAR(val + 1);
	else
		throw error_consistency("finding out which variabile to add attributes to, *var is not set");
}

void CommonAPIImplementation::read_qc_list(vector<Varcode>& res_arr) const
{
	res_arr.clear();
	if (const char* val = qcinput.key_peek_value(DBA_KEY_VAR))
	{
		/* Get only the QC values in *varlist */
		if (*val != '*')
			throw error_consistency("QC values must start with '*'");
		
		res_arr.push_back(WR_STRING_TO_VAR(val + 2));
	}
	else if (const char* val = qcinput.key_peek_value(DBA_KEY_VARLIST))
	{
		/* Get only the QC values in *varlist */
		size_t pos;
		size_t len;
		int i;

		for (pos = 0, i = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
		{
			if (*(val+pos) != '*')
				throw error_consistency("QC value names must start with '*'");
			res_arr.push_back(WR_STRING_TO_VAR(val + pos + 2));
		}
	}
}

void CommonAPIImplementation::clear_qcinput()
{
	int saved_context_id = -1;
	char saved_varname[8];

	// Save the values to be preserved
	if (const char* val = qcinput.key_peek_value(DBA_KEY_CONTEXT_ID))
		saved_context_id = strtol(val, NULL, 10);
	if (const char* val = qcinput.key_peek_value(DBA_KEY_VAR_RELATED))
	{
		strncpy(saved_varname, val, 7);
		saved_varname[6] = 0;
	}
	else
		saved_varname[0] = 0;

	// Clear the qcinput record
	qcinput.clear();

	// Restore the saved values
	if (saved_context_id != -1)
		qcinput.set(DBA_KEY_CONTEXT_ID, saved_context_id);
	if (saved_varname[0] != 0)
		qcinput.set(DBA_KEY_VAR_RELATED, saved_varname);
}

}
}

/* vim:set ts=4 sw=4: */
