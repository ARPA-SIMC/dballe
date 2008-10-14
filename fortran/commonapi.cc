/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

using namespace std;

namespace dballef {

CommonAPIImplementation::CommonAPIImplementation()
	: perms(0), input(NULL), output(NULL), qcinput(NULL), qcoutput(NULL), qc_iter(NULL), qc_count(0), cached_spiega(0)
{
	/* Allocate the records */
	checked(dba_record_create(&input));
	checked(dba_record_create(&output));
	checked(dba_record_create(&qcinput));
	checked(dba_record_create(&qcoutput));
}

CommonAPIImplementation::~CommonAPIImplementation()
{
	if (qcoutput != NULL)
		dba_record_delete(qcoutput);
	if (qcinput != NULL)
		dba_record_delete(qcinput);
	if (output != NULL)
		dba_record_delete(output);
	if (input != NULL)
		dba_record_delete(input);
	if (cached_spiega)
		free(cached_spiega);
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
		checked(dba_error_consistency("pseudoana should be opened in either 'read' or 'write' mode"));
	if ((perms & (PERM_DATA_RO | PERM_DATA_ADD | PERM_DATA_WRITE)) == 0)
		checked(dba_error_consistency("data should be opened in one of 'read', 'add' or 'write' mode"));
	if ((perms & (PERM_ATTR_RO | PERM_ATTR_ADD | PERM_ATTR_WRITE)) == 0)
		checked(dba_error_consistency("attr should be opened in one of 'read', 'add' or 'write' mode"));

	if (perms & PERM_ANA_RO && perms & PERM_DATA_WRITE)
		checked(dba_error_consistency("when data is 'write' ana must also be set to 'write', because deleting data can potentially also delete pseudoana"));
	if (perms & PERM_ATTR_RO && perms & PERM_DATA_WRITE)
		checked(dba_error_consistency("when data is 'write' attr must also be set to 'write', because deleting data also delete its attributes"));
}

dba_record CommonAPIImplementation::choose_input_record(const char*& param)
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

dba_record CommonAPIImplementation::choose_output_record(const char*& param)
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
	checked(dba_record_copy(output, input));
}

int CommonAPIImplementation::enqi(const char* param)
{
	dba_record rec = choose_output_record(param);
	int found;
	int value;

	checked(dba_record_enqi(rec, param, &value, &found));
	if (!found) return missing_int;
	return value;
}

signed char CommonAPIImplementation::enqb(const char* param)
{
	dba_record rec = choose_output_record(param);
	int found;
	int value;

	checked(dba_record_enqi(rec, param, &value, &found));

	if (!found) return missing_byte;
	if (value < numeric_limits<signed char>::min()
	 || value > numeric_limits<signed char>::max())
		checked(dba_error_consistency("value queried (%d) does not fit in a byte", value));
	return value;
}

float CommonAPIImplementation::enqr(const char* param)
{
	dba_record rec = choose_output_record(param);
	int found;
	double value;

	checked(dba_record_enqd(rec, param, &value, &found));

	if (!found) return missing_float;
	if (value < -numeric_limits<float>::max()
	 || value > numeric_limits<float>::max())
		checked(dba_error_consistency("value queried (%f) does not fit in a real", value));
	return value;
}

double CommonAPIImplementation::enqd(const char* param)
{
	dba_record rec = choose_output_record(param);
	int found;
	double value;

	checked(dba_record_enqd(rec, param, &value, &found));

	if (!found)
		value = missing_double;
	return value;
}

const char* CommonAPIImplementation::enqc(const char* param)
{
	dba_record rec = choose_output_record(param);
	const char* value;

	checked(dba_record_enqc(rec, param, &value));
	return value;
}

static dba_varcode checkvar(const char* param)
{
	if (param[0] == 'B')
		return DBA_STRING_TO_VAR(param + 1);
	return dba_varcode_alias_resolve(param);
}

static dba_keyword prepare_key_change(dba_record rec, const char* param)
{
	dba_keyword key = dba_record_keyword_byname(param);
	switch (key)
	{
		case DBA_KEY_ERROR:
			checked(dba_error_notfound("looking for misspelled parameter \"%s\"", param));
			break;
		case DBA_KEY_LAT:
		case DBA_KEY_LON:
			checked(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
			break;
		case DBA_KEY_ANA_ID:
			checked(dba_record_key_unset(rec, DBA_KEY_LAT));
			checked(dba_record_key_unset(rec, DBA_KEY_LON));
			break;
		default: break;
	}
	return key;
}

void CommonAPIImplementation::seti(const char* param, int value)
{
	dba_record rec = choose_input_record(param);
	dba_varcode code = checkvar(param);

	if (code)
	{
		checked(dba_record_var_seti(rec, code, value));
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		checked(dba_record_key_seti(rec, key, value));
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
	dba_record rec = choose_input_record(param);
	dba_varcode code = checkvar(param);

	if (code)
	{
		checked(dba_record_var_setd(rec, code, value));
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		checked(dba_record_key_setd(rec, key, value));
	}
}

void CommonAPIImplementation::setc(const char* param, const char* value)
{
	dba_record rec = choose_input_record(param);
	dba_varcode code = checkvar(param);

	if (code)
	{
		checked(dba_record_var_setc(rec, code, value));
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		checked(dba_record_key_setc(rec, key, value));
	}
}

void CommonAPIImplementation::setcontextana()
{
	checked(dba_record_set_ana_context(input));
}

void CommonAPIImplementation::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
	int found;
	checked(dba_record_key_enqi(output, DBA_KEY_LEVELTYPE1, &ltype1, &found));
	if (!found) ltype1 = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_L1, &l1, &found));
	if (!found) l1 = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_LEVELTYPE2, &ltype2, &found));
	if (!found) ltype2 = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_L2, &l2, &found));
	if (!found) l2 = missing_int;
}

void CommonAPIImplementation::setlevel(int ltype1, int l1, int ltype2, int l2)
{
	if (ltype1 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_LEVELTYPE1));
	else
		checked(dba_record_key_seti(input, DBA_KEY_LEVELTYPE1, ltype1));

	if (l1 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_L1));
	else
		checked(dba_record_key_seti(input, DBA_KEY_L1, l1));

	if (ltype2 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_LEVELTYPE2));
	else
		checked(dba_record_key_seti(input, DBA_KEY_LEVELTYPE2, ltype2));

	if (l2 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_L2));
	else
		checked(dba_record_key_seti(input, DBA_KEY_L2, l2));
}

void CommonAPIImplementation::enqtimerange(int& ptype, int& p1, int& p2)
{
	int found;
	checked(dba_record_key_enqi(output, DBA_KEY_PINDICATOR, &ptype, &found));
	if (!found) ptype = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_P1, &p1, &found));
	if (!found) p1 = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_P2, &p2, &found));
	if (!found) p2 = missing_int;
}

void CommonAPIImplementation::settimerange(int ptype, int p1, int p2)
{
	if (ptype == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_PINDICATOR));
	else
		checked(dba_record_key_seti(input, DBA_KEY_PINDICATOR, ptype));
	if (p1 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_P1));
	else
		checked(dba_record_key_seti(input, DBA_KEY_P1, p1));
	if (p2 == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_P2));
	else
		checked(dba_record_key_seti(input, DBA_KEY_P2, p2));
}

void CommonAPIImplementation::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
	int found;
	checked(dba_record_key_enqi(output, DBA_KEY_YEAR, &year, &found));
	if (!found) year = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_MONTH, &month, &found));
	if (!found) month = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_DAY, &day, &found));
	if (!found) day = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_HOUR, &hour, &found));
	if (!found) hour = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_MIN, &min, &found));
	if (!found) min = missing_int;
	checked(dba_record_key_enqi(output, DBA_KEY_SEC, &sec, &found));
	if (!found) sec = missing_int;
}

void CommonAPIImplementation::setdate(int year, int month, int day, int hour, int min, int sec)
{
	if (year == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_YEAR));
	else
		checked(dba_record_key_seti(input, DBA_KEY_YEAR, year));
	if (month == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_MONTH));
	else
		checked(dba_record_key_seti(input, DBA_KEY_MONTH, month));
	if (day == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_DAY));
	else
		checked(dba_record_key_seti(input, DBA_KEY_DAY, day));
	if (hour == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_HOUR));
	else
		checked(dba_record_key_seti(input, DBA_KEY_HOUR, hour));
	if (min == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_MIN));
	else
		checked(dba_record_key_seti(input, DBA_KEY_MIN, min));
	if (sec == missing_int)
		checked(dba_record_key_unset(input, DBA_KEY_SEC));
	else
		checked(dba_record_key_seti(input, DBA_KEY_SEC, sec));
}

void CommonAPIImplementation::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
	checked(dba_record_key_seti(input, DBA_KEY_YEARMIN, year));
	checked(dba_record_key_seti(input, DBA_KEY_MONTHMIN, month));
	checked(dba_record_key_seti(input, DBA_KEY_DAYMIN, day));
	checked(dba_record_key_seti(input, DBA_KEY_HOURMIN, hour));
	checked(dba_record_key_seti(input, DBA_KEY_MINUMIN, min));
	checked(dba_record_key_seti(input, DBA_KEY_SECMIN, sec));
}

void CommonAPIImplementation::setdatemax(int year, int month, int day, int hour, int min, int sec)
{

	checked(dba_record_key_seti(input, DBA_KEY_YEARMAX, year));
	checked(dba_record_key_seti(input, DBA_KEY_MONTHMAX, month));
	checked(dba_record_key_seti(input, DBA_KEY_DAYMAX, day));
	checked(dba_record_key_seti(input, DBA_KEY_HOURMAX, hour));
	checked(dba_record_key_seti(input, DBA_KEY_MINUMAX, min));
	checked(dba_record_key_seti(input, DBA_KEY_SECMAX, sec));
}

void CommonAPIImplementation::unset(const char* param)
{
	dba_record rec = choose_input_record(param);
	dba_varcode code = checkvar(param);

	if (code)
	{
		checked(dba_record_var_unset(rec, code));
	} else {
		dba_keyword key = prepare_key_change(rec, param);
		checked(dba_record_key_unset(rec, key));
	}
}

void CommonAPIImplementation::unsetall()
{
	clear_qcinput();
	dba_record_clear(input);
}

char* CommonAPIImplementation::spiegal(int ltype1, int l1, int ltype2, int l2)
{
	if (cached_spiega)
	{
		free(cached_spiega);
		cached_spiega = 0;
	}
	checked(dba_formatter_describe_level_or_layer(ltype1, l1, ltype2, l2, &cached_spiega));
	return cached_spiega;
}

char* CommonAPIImplementation::spiegat(int ptype, int p1, int p2)
{
	if (cached_spiega)
	{
		free(cached_spiega);
		cached_spiega = 0;
	}
	checked(dba_formatter_describe_trange(ptype, p1, p2, &cached_spiega));
	return cached_spiega;
}

char* CommonAPIImplementation::spiegab(const char* varcode, const char* value)
{
	dba_var var = NULL;
	dba_varinfo info;

	if (cached_spiega)
	{
		free(cached_spiega);
		cached_spiega = 0;
	}

	try {
		checked(dba_varinfo_query_local(DBA_STRING_TO_VAR(varcode + 1), &info));
		checked(dba_var_createc(info, value, &var));

		if (info->is_string)
		{
			const char* s;
			checked(dba_var_enqc(var, &s));
			asprintf(&cached_spiega, "%s (%s) %s", s, info->unit, info->desc);
		} else {
			double d;
			checked(dba_var_enqd(var, &d));
			asprintf(&cached_spiega, "%.*f (%s) %s", info->scale > 0 ? info->scale : 0, d, info->unit, info->desc);
		}
		return cached_spiega;
	} catch (...) {
		// If an exception happens, we clean up var and rethrow it
		if (var)
			dba_var_delete(var);
		throw;
	}
}

const char* CommonAPIImplementation::ancora()
{
	static char parm[10];

	if (qc_iter == NULL)
		checked(dba_error_notfound("reading a QC item"));

	dba_varcode var = dba_var_code(dba_record_cursor_variable(qc_iter));
	snprintf(parm, 10, "*B%02d%03d", DBA_VAR_X(var), DBA_VAR_Y(var));

	/* Get next value from qc */
	qc_iter = dba_record_iterate_next(qcoutput, qc_iter);

	return parm;
}

void CommonAPIImplementation::get_referred_data_id(int* id_context, dba_varcode* id_var) const
{
	int found;

	/* Read context ID */
	checked(dba_record_key_enqi(qcinput, DBA_KEY_CONTEXT_ID, id_context, &found));
	if (!found)
		checked(dba_error_notfound("looking for variable context id"));

	/* Read variable ID */
	if (const char* val = dba_record_key_peek_value(qcinput, DBA_KEY_VAR_RELATED))
		*id_var = DBA_STRING_TO_VAR(val + 1);
	else
		checked(dba_error_consistency("finding out which variabile to add attributes to, *var is not set"));
}

void CommonAPIImplementation::read_qc_list(dba_varcode** res_arr, size_t* res_len) const
{
	dba_varcode* arr = NULL;
	size_t arr_len = 0;

	try {
		if (const char* val = dba_record_key_peek_value(qcinput, DBA_KEY_VAR))
		{
			/* Get only the QC values in *varlist */
			if (*val != '*')
				checked(dba_error_consistency("QC values must start with '*'"));
			
			if ((arr = (dba_varcode*)malloc(1 * sizeof(dba_varcode))) == NULL)
				checked(dba_error_alloc("allocating the dba_varcode array to pass to dba_qc_query"));

			arr_len = 1;
			arr[0] = DBA_STRING_TO_VAR(val + 2);
		}
		else if (const char* val = dba_record_key_peek_value(qcinput, DBA_KEY_VARLIST))
		{
			/* Get only the QC values in *varlist */
			size_t pos;
			size_t len;
			const char* s;
			int i;

			/* Count the number of commas (and therefore of items in the
			 * list) to decide the size of arr */
			for (s = val, arr_len = 1; *s; ++s)
				if (*s == ',')
					++arr_len;
			if ((arr = (dba_varcode*)malloc(arr_len * sizeof(dba_varcode))) == NULL)
				checked(dba_error_alloc("allocating the dba_varcode array to pass to dba_qc_query"));

			for (pos = 0, i = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
			{
				if (*(val+pos) != '*')
					checked(dba_error_consistency("QC value names must start with '*'"));
				arr[i++] = DBA_STRING_TO_VAR(val + pos + 2);
			}
		}

		*res_arr = arr;
		*res_len = arr_len;
	} catch (...) {
		if (arr != NULL)
			free(arr);
		throw;
	}
}

void CommonAPIImplementation::clear_qcinput()
{
	int saved_context_id = -1;
	char saved_varname[8];

	// Save the values to be preserved
	if (const char* val = dba_record_key_peek_value(qcinput, DBA_KEY_CONTEXT_ID))
		saved_context_id = strtol(val, NULL, 10);
	if (const char* val = dba_record_key_peek_value(qcinput, DBA_KEY_VAR_RELATED))
	{
		strncpy(saved_varname, val, 7);
		saved_varname[6] = 0;
	}
	else
		saved_varname[0] = 0;

	// Clear the qcinput record
	dba_record_clear(qcinput);

	// Restore the saved values
	if (saved_context_id != -1)
		checked(dba_record_key_seti(qcinput, DBA_KEY_CONTEXT_ID, saved_context_id));
	if (saved_varname[0] != 0)
		checked(dba_record_key_setc(qcinput, DBA_KEY_VAR_RELATED, saved_varname));
}

}

/* vim:set ts=4 sw=4: */
