/*
 * dballe/record - groups of related variables
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

#include "record.h"
#include "var.h"
#include "aliases.h"

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

using namespace wreport;
using namespace std;

namespace dballe {

/*
 * Size of the keyword table.  It should be the number of items in
 * dba_record_keyword.gperf, plus 1
 */
#define KEYWORD_TABLE_SIZE DBA_KEY_COUNT

#define assert_is_dba_record(rec) do { \
		assert((rec) != NULL); \
	} while (0)

Record::Record()
{
	memset(keydata, 0, sizeof(keydata));
}

Record::Record(const Record& rec)
{
	// Copy the keyword table
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
	{
		if (rec.keydata[i] == NULL)
			keydata[i] = NULL;
		else
			keydata[i] = new Var(*rec.keydata[i]);
	}

	// Copy the variable list
	for (vector<Var*>::const_iterator i = rec.m_vars.begin();
			i != rec.m_vars.end(); ++i)
		m_vars.push_back(new Var(**i));
}

Record::~Record()
{
	clear();
}

Record& Record::operator=(const Record& rec)
{
	// Prevent self-copying
	if (this == &rec) return *this;

	// Copy the keyword table first
	for (int i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (keydata[i] != NULL)
		{
			if (rec.keydata[i] != NULL)
				*keydata[i] = *rec.keydata[i];
			else
			{
				delete keydata[i];
				keydata[i] = NULL;
			}
		} else if (rec.keydata[i] != NULL)
			keydata[i] = new Var(*rec.keydata[i]);
	}

	// Copy the variable list
	clear_vars();
	for (vector<Var*>::const_iterator i = rec.m_vars.begin();
			i != rec.m_vars.end(); ++i)
		m_vars.push_back(new Var(**i));

	return *this;
}

bool Record::operator==(const Record& rec) const
{
	// Compare the keywords
	for (int i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (keydata[i] == NULL && rec.keydata[i] == NULL)
			continue;

		if (keydata[i] == NULL || rec.keydata[i] == NULL)
			return false;

		if (*keydata[i] != *rec.keydata[i])
			return false;
	}

	// Compare the variables
	vector<Var*>::const_iterator i1 = m_vars.begin();
	vector<Var*>::const_iterator i2 = rec.m_vars.begin();
	for ( ; i1 != m_vars.end() && i2 != rec.m_vars.end(); ++i1, ++i2)
		if (**i1 != **i2) return false;
	if (i1 != m_vars.end() || i2 != rec.m_vars.end())
		return false;

	return true;
}

void Record::clear_vars()
{
	for (vector<Var*>::iterator i = m_vars.begin();
			i != m_vars.end(); ++i)
		delete *i;
	m_vars.clear();
}

void Record::clear()
{
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
		if (keydata[i] != NULL)
		{
			delete keydata[i];
			keydata[i] = NULL;
		}
	clear_vars();
}

int Record::find_item(Varcode code) const throw ()
{
	/* Binary search */
	int low = 0, high = m_vars.size() - 1;
	while (low <= high)
	{
		int middle = low + (high - low)/2;
		int cmp = (int)code - (int)m_vars[middle]->code();
		if (cmp < 0)
			high = middle - 1;
		else if (cmp > 0)
			low = middle + 1;
		else
			return middle;
	}
	return -1;
}

Var& Record::get_item(Varcode code)
{
	int pos = find_item(code);
	if (pos == -1)
		error_notfound::throwf("looking for parameter \"B%02d%03d\"",
			WR_VAR_X(code), WR_VAR_Y(code));
	return *m_vars[pos];
}

const Var& Record::get_item(Varcode code) const
{
	int pos = find_item(code);
	if (pos == -1)
		error_notfound::throwf("looking for parameter \"B%02d%03d\"",
			WR_VAR_X(code), WR_VAR_Y(code));
	return *m_vars[pos];
}

void Record::remove_item(Varcode code)
{
	int pos = find_item(code);
	if (pos == -1) return;
	delete m_vars[pos];
	m_vars.erase(m_vars.begin() + pos);
}

void Record::add(const Record& source)
{
	// Add the keyword table
	for (int i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (source.keydata[i] != NULL)
		{
			if (keydata[i] != NULL)
				*keydata[i] = *source.keydata[i];
			else
				keydata[i] = new Var(*source.keydata[i]);
		}
	}

	// Add the variables list
	vector<Var*>::const_iterator src = source.m_vars.begin();
	size_t dst = 0;
	while (src != source.m_vars.end() && dst < m_vars.size())
	{
		if ((*src)->code() < m_vars[dst]->code())
		{
			// Insert
			m_vars.insert(m_vars.begin() + dst, new Var(**src));
			++src;
		} else if ((*src)->code() == m_vars[dst]->code()) {
			// Overwrite
			*m_vars[dst] = **src;
			++dst;
			++src;
		} else {
			++dst;
		}
	}
	// Append the remaining source vars
	for ( ; src != source.m_vars.end(); ++src)
		m_vars.push_back(new Var(**src));
}

void Record::set_to_difference(const Record& source1, const Record& source2)
{
	// Copy the keyword table
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
	{
		Var* src = NULL;
		if (source2.keydata[i] != NULL &&
			(source1.keydata[i] == NULL ||
			 strcmp(source1.keydata[i]->value(), source2.keydata[i]->value()) != 0))
			src = source2.keydata[i];

		if (keydata[i] != NULL)
		{
			if (src != NULL)
				*keydata[i] = *src;
			else
			{
				delete keydata[i];
				keydata[i] = NULL;
			}
		} else if (src != NULL)
			keydata[i] = new Var(*src);
	}

	// Copy the variables list
	clear_vars();

	vector<Var*>::const_iterator s1 = source1.m_vars.begin();
	vector<Var*>::const_iterator s2 = source2.m_vars.begin();
	while (s1 != source1.m_vars.end() && s2 != source2.m_vars.end())
	{
		if ((*s1)->code() < (*s2)->code())
			++s1;
		else if ((*s1)->code() == (*s2)->code())
		{
			if (**s1 != **s2)
				m_vars.push_back(new Var(**s2));
			++s1;
			++s2;
		}
		else if ((*s2)->code() < (*s1)->code())
		{
			m_vars.push_back(new Var(**s2));
			++s2;
		}
	}
	for ( ; s2 != source2.m_vars.end(); ++s2)
		m_vars.push_back(new Var(**s2));
}

const Var* Record::key_peek(dba_keyword parameter) const throw ()
{
	return keydata[parameter];
}

const Var* Record::var_peek(Varcode code) const throw ()
{
	int pos = find_item(code);
	if (pos == -1) return NULL;
	return m_vars[pos];
}

const char* Record::key_peek_value(dba_keyword parameter) const throw ()
{
	const Var* res = key_peek(parameter);
	return res ? res->value() : NULL;
}

const char* Record::var_peek_value(Varcode code) const throw ()
{
	const Var* res = var_peek(code);
	return res ? res->value() : NULL;
}

const wreport::Var& Record::key(dba_keyword parameter) const
{
	const Var* res = key_peek(parameter);
	if (!res)
		error_notfound::throwf("Parameter %s not found in record", keyword_name(parameter));
	return *res;
}

const wreport::Var& Record::var(wreport::Varcode code) const
{
	const Var* res = var_peek(code);
	if (!res)
		error_notfound::throwf("Variable %01d%02d%03d not found in record",
				WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
	return *res;
}

Var& Record::key(dba_keyword parameter)
{
	if (keydata[parameter] == NULL)
		keydata[parameter] = new Var(keyword_info(parameter));
	return *keydata[parameter];
}

Var& Record::var(wreport::Varcode code)
{
	int pos = find_item(code);
	if (pos == -1)
	{
		// Insertion sort the new variable

		// Enlarge the buffer
		m_vars.resize(m_vars.size() + 1);

		/* Insertionsort.  Crude, but our datasets should be too small for an
		 * RB-Tree to be worth it */
		for (pos = m_vars.size() - 1; pos > 0; --pos)
		    if (m_vars[pos - 1]->code() > code)
			m_vars[pos] = m_vars[pos - 1];
		    else
			break;
		m_vars[pos] = newvar(code).release();
	}
	return *m_vars[pos];
}

void Record::key_unset(dba_keyword parameter)
{
	if (keydata[parameter] != NULL)
	{
		delete keydata[parameter];
		keydata[parameter] = NULL;
	}
}
void Record::var_unset(wreport::Varcode code)
{
	int pos = find_item(code);
	if (pos != -1)
	{
		delete m_vars[pos];
		m_vars.erase(m_vars.begin() + pos);
	}
}

const std::vector<wreport::Var*>& Record::vars() const
{
	return m_vars;
}

#if 0

dba_err dba_record_set_from_string(dba_record rec, const char* str)
{
	/* Split the input as name=val */
	const char* s;
	const char* val;
	dba_varcode varcode;
	dba_varinfo info;
	dba_keyword param;
	
	if ((s = strchr(str, '=')) == NULL)
		return dba_error_consistency("there should be an = between the name and the value in '%s'", str);

//		name = strndup(queryparm, s - queryparm);
	val = s + 1;

	/* First see if it's an alias or a variable */
	if ((varcode = dba_varcode_alias_resolve_substring(str, s - str)) != 0 || str[0] == 'B')
	{
		if (varcode == 0)
			varcode = DBA_STRING_TO_VAR(str + 1);

		/* Query informations about the parameter */
		DBA_RUN_OR_RETURN(dba_varinfo_query_local(varcode, &info));

		if (VARINFO_IS_STRING(info))
			DBA_RUN_OR_RETURN(dba_record_var_setc(rec, varcode, val));
		else
			DBA_RUN_OR_RETURN(dba_record_var_setd(rec, varcode, strtod(val, 0)));
	} else {
		/* Else handle a normal keyword */

		param = dba_record_keyword_byname_len(str, s - str);

		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled keyword \"%.*s\"", s - str, str);

		/* Query informations about the parameter */
		DBA_RUN_OR_RETURN(dba_record_keyword_info(param, &info));

		if (VARINFO_IS_STRING(info))
			DBA_RUN_OR_RETURN(dba_record_key_setc(rec, param, val));
		else
			DBA_RUN_OR_RETURN(dba_record_key_setd(rec, param, strtod(val, 0)));
	}

	return dba_error_ok();
}

static dba_err get_key(dba_record rec, dba_keyword parameter, dba_var* var)
{
	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);

	/* Lookup the variable in the keyword table */
	if (rec->keydata[parameter] == NULL)
		*var = NULL;
	else
		*var = rec->keydata[parameter];

	return dba_error_ok();
}

static dba_err get_var(dba_record rec, dba_varcode code, dba_var* var)
{
	dba_item cur;
	for (cur = rec->vars; cur != NULL; cur = cur->next)
		if (dba_var_code(cur->var) == code)
		{
			*var = cur->var;
			return dba_error_ok();
		}
	*var = NULL;
	return dba_error_ok();
}

static dba_err get(dba_record rec, const char* name, dba_var* var)
{
	dba_varcode code = 0;
    if (name[0] != 'B' && (code = dba_varcode_alias_resolve(name)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", name);
		return get_key(rec, param, var);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(name + 1);
		return get_var(rec, code, var);
	}
}

dba_err dba_record_contains(dba_record rec, const char* name, int *found)
{
	dba_varcode code = 0;
    if (name[0] != 'B' && (code = dba_varcode_alias_resolve(name)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", name);
		return dba_record_contains_key(rec, param, found);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(name + 1);
		return dba_record_contains_var(rec, code, found);
	}
}

dba_err dba_record_contains_key(dba_record rec, dba_keyword parameter, int *found)
{
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("looking for informations about parameter #%d", parameter);

	*found = (rec->keydata[parameter] != NULL);

	return dba_error_ok();
}

dba_err dba_record_contains_var(dba_record rec, dba_varcode code, int *found)
{
	*found = rec->has_item(code);

	return dba_error_ok();
}

dba_err dba_record_enq(dba_record rec, const char* name, dba_var* var)
{
	dba_varcode code = 0;
    if (name[0] != 'B' && (code = dba_varcode_alias_resolve(name)) == 0)
	{
		dba_var myvar;
		dba_keyword param = dba_record_keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", name);
		DBA_RUN_OR_RETURN(get_key(rec, param, &myvar));
		if (myvar == NULL)
			return dba_var_create(dba_record_keyword_byindex(param), var);
		else
			return dba_var_copy(myvar, var);
	} else {
		dba_var myvar;
		if (code == 0)
			code = DBA_STRING_TO_VAR(name + 1);
		DBA_RUN_OR_RETURN(get_var(rec, code, &myvar));
		if (myvar == NULL)
		{
			dba_varinfo info;
			DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, &info));
			return dba_var_create(info, var);
		}
		else
			return dba_var_copy(myvar, var);
	}
}

dba_err dba_record_key_enq(dba_record rec, dba_keyword parameter, dba_var* var)
{
	dba_var myvar;
	DBA_RUN_OR_RETURN(get_key(rec, parameter, &myvar));
	if (myvar == NULL)
		return dba_var_create(dba_record_keyword_byindex(parameter), var);
	else
		return dba_var_copy(myvar, var);
}

dba_err dba_record_var_enq(dba_record rec, dba_varcode code, dba_var* var)
{
	dba_var myvar;
	DBA_RUN_OR_RETURN(get_var(rec, code, &myvar));
	if (myvar == NULL)
	{
		dba_varinfo info;
		DBA_RUN_OR_RETURN(dba_varinfo_query_local(code, &info));
		return dba_var_create(info, var);
	}
	else
		return dba_var_copy(myvar, var);
}

dba_err dba_record_enqi(dba_record rec, const char* name, int* value, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get(rec, name, &var));
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqi(var, value);
	}
}

dba_err dba_record_key_enqi(dba_record rec, dba_keyword parameter, int* val, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_key(rec, parameter, &var));
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqi(var, val);
	}
}

dba_err dba_record_var_enqi(dba_record rec, dba_varcode code, int* val, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_var(rec, code, &var)); // TAINTED
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqi(var, val);
	}
}

dba_err dba_record_enqd(dba_record rec, const char* name, double* val, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get(rec, name, &var));
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqd(var, val);
	}
}

dba_err dba_record_key_enqd(dba_record rec, dba_keyword parameter, double* val, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_key(rec, parameter, &var));
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqd(var, val);
	}
}

dba_err dba_record_var_enqd(dba_record rec, dba_varcode code, double* val, int* found)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_var(rec, code, &var));
	if (var == NULL)
	{
		*found = 0;
		return dba_error_ok();
	}
	else
	{
		*found = 1;
		return dba_var_enqd(var, val);
	}
}

dba_err dba_record_enqc(dba_record rec, const char* name, const char** val)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get(rec, name, &var));
	if (var == NULL)
	{
		*val = NULL;
		return dba_error_ok();
	}
	else
		return dba_var_enqc(var, val);
}

dba_err dba_record_key_enqc(dba_record rec, dba_keyword parameter, const char** val)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_key(rec, parameter, &var));
	if (var == NULL)
	{
		*val = NULL;
		return dba_error_ok();
	}
	else
		return dba_var_enqc(var, val);
}

dba_err dba_record_var_enqc(dba_record rec, dba_varcode code, const char** val)
{
	dba_var var;
	DBA_RUN_OR_RETURN(get_var(rec, code, &var));
	if (var == NULL)
	{
		*val = NULL;
		return dba_error_ok();
	}
	else
		return dba_var_enqc(var, val);
}

dba_err dba_record_key_set(dba_record rec, dba_keyword parameter, dba_var var)
{
	dba_varinfo info;
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);
	if (rec->keydata[parameter] != NULL)
	{
		dba_var_delete(rec->keydata[parameter]);
		rec->keydata[parameter] = NULL;
	}
	info = dba_record_keyword_byindex(parameter);
	DBA_RUN_OR_RETURN(dba_var_convert(var, info, &(rec->keydata[parameter])));
	return dba_error_ok();
}

dba_err dba_record_var_set(dba_record rec, dba_varcode code, dba_var var)
{
	assert_is_dba_record(rec);

	dba_err err = DBA_OK;
	dba_item i = NULL;

	/* If var is undef, remove this variable from the record */
	if (dba_var_value(var) == NULL)
		dba_record_remove_item(rec, code);
	else
	{
		/* Lookup the variable in the hash table */
		DBA_RUN_OR_GOTO(fail1, dba_record_obtain_item(rec, code, &i));

		/* Set the value of the variable */
		if (i->var != NULL)
		{
			dba_var_delete(i->var);
			i->var = NULL;
		}
		if (dba_var_code(var) == code)
			DBA_RUN_OR_GOTO(fail1, dba_var_copy(var, &(i->var)));
		else
		{
			dba_varinfo info;
			DBA_RUN_OR_GOTO(fail1, dba_varinfo_query_local(code, &info));
			DBA_RUN_OR_GOTO(fail1, dba_var_convert(var, info, &(i->var)));
		}
	}
	return dba_error_ok();
fail1:
	if (i != NULL)
		dba_record_remove_dba_item(rec, i);
	return err;
}

dba_err dba_record_var_set_direct(dba_record rec, dba_var var)
{
	dba_item i;
	dba_varcode varcode = dba_var_code(var);

	assert_is_dba_record(rec);

	/* If var is undef, remove this variable from the record */
	if (dba_var_value(var) == NULL)
	{
		dba_record_remove_item(rec, varcode);
	} else {
		/* Lookup the variable in the hash table */
		DBA_RUN_OR_RETURN(dba_record_obtain_item(rec, varcode, &i));

		/* Set the value of the variable */
		if (i->var != NULL)
		{
			dba_var_delete(i->var);
			i->var = NULL;
		}
		DBA_RUN_OR_RETURN(dba_var_copy(var, &(i->var)));
	}
	return dba_error_ok();
}

dba_err dba_record_set_ana_context(dba_record rec)
{
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_YEAR, 1000));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MONTH, 1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_DAY, 1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_HOUR, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MIN, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_SEC, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE1, 257));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L1, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_LEVELTYPE2, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_L2, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_PINDICATOR, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P1, 0));
	DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_P2, 0));
	/* DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_REP_COD, 254)); */
	return dba_error_ok();
}

dba_err dba_record_key_seti(dba_record rec, dba_keyword parameter, int value)
{
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);

	if (rec->keydata[parameter] != NULL)
		return dba_var_seti(rec->keydata[parameter], value);
	else
		return dba_var_createi(dba_record_keyword_byindex(parameter), value, &(rec->keydata[parameter]));
}

dba_err dba_record_var_seti(dba_record rec, dba_varcode code, int value)
{
	dba_err err;
	dba_item i = NULL;

	assert_is_dba_record(rec);

	/* Lookup the variable in the hash table */
	DBA_RUN_OR_RETURN(dba_record_obtain_item(rec, code, &i));

	/* Set the integer value of the variable */
	if (i->var == NULL)
	{
		dba_varinfo info;
		DBA_RUN_OR_GOTO(fail, dba_varinfo_query_local(code, &info));
		DBA_RUN_OR_GOTO(fail, dba_var_createi(info, value, &(i->var)));
	}
	else
		DBA_RUN_OR_GOTO(fail, dba_var_seti(i->var, value));

	return dba_error_ok();

fail:
	if (i != NULL)
		dba_record_rollback_obtain(rec);
	return err;
}

dba_err dba_record_key_setd(dba_record rec, dba_keyword parameter, double value)
{
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);

	if (rec->keydata[parameter] != NULL)
		return dba_var_setd(rec->keydata[parameter], value);
	else
		return dba_var_created(dba_record_keyword_byindex(parameter), value, &(rec->keydata[parameter]));
}

dba_err dba_record_var_setd(dba_record rec, dba_varcode code, double value)
{
	dba_err err;
	dba_item i = NULL;

	assert_is_dba_record(rec);

	/* Lookup the variable in the hash table */
	DBA_RUN_OR_RETURN(dba_record_obtain_item(rec, code, &i));

	/* Set the double value of the variable */
	if (i->var == NULL)
	{
		dba_varinfo info;
		DBA_RUN_OR_GOTO(fail, dba_varinfo_query_local(code, &info));
		DBA_RUN_OR_GOTO(fail, dba_var_created(info, value, &(i->var)));
	}
	else
		DBA_RUN_OR_GOTO(fail, dba_var_setd(i->var, value));

	return dba_error_ok();

fail:
	if (i != NULL)
		dba_record_rollback_obtain(rec);
	return err;
}

dba_err dba_record_key_setc(dba_record rec, dba_keyword parameter, const char* value)
{
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);

	if (rec->keydata[parameter] != NULL)
		return dba_var_setc(rec->keydata[parameter], value);
	else
		return dba_var_createc(dba_record_keyword_byindex(parameter), value, &(rec->keydata[parameter]));
}

dba_err dba_record_var_setc(dba_record rec, dba_varcode code, const char* value)
{
	dba_err err;
	dba_item i = NULL;

	assert_is_dba_record(rec);

	/* Lookup the variable in the hash table */
	DBA_RUN_OR_RETURN(dba_record_obtain_item(rec, code, &i));

	/* Set the string value of the variable */
	if (i->var == NULL)
	{
		dba_varinfo info;
		DBA_RUN_OR_GOTO(fail, dba_varinfo_query_local(code, &info));
		DBA_RUN_OR_GOTO(fail, dba_var_createc(info, value, &(i->var)));
	}
	else
		DBA_RUN_OR_GOTO(fail, dba_var_setc(i->var, value));
	return dba_error_ok();

fail:
	if (i != NULL)
		dba_record_rollback_obtain(rec);
	return err;
}

dba_err dba_record_key_unset(dba_record rec, dba_keyword parameter)
{
	assert_is_dba_record(rec);

	if (parameter < 0 || parameter >= DBA_KEY_COUNT)
		return dba_error_notfound("keyword #%d is not in the range of valid keywords", parameter);

	/* Delete old value if it exists */
	if (rec->keydata[parameter] != NULL)
	{
		dba_var_delete(rec->keydata[parameter]);
		rec->keydata[parameter] = NULL;
	}
	return dba_error_ok();
}

dba_err dba_record_var_unset(dba_record rec, dba_varcode code)
{
	assert_is_dba_record(rec);

	dba_record_remove_item(rec, code);

	return dba_error_ok();
}

void dba_record_print(dba_record rec, FILE* out)
{
	int i;
	dba_record_cursor cur;
	for (i = 0; i < KEYWORD_TABLE_SIZE; i++)
		if (rec->keydata[i] != NULL)
			dba_var_print(rec->keydata[i], out);

	for (cur = dba_record_iterate_first(rec); cur != NULL; cur = dba_record_iterate_next(rec, cur))
		dba_var_print(dba_record_cursor_variable(cur), out);
}

void dba_record_diff(dba_record rec1, dba_record rec2, int* diffs, FILE* out)
{
	int i;
	dba_record_cursor cur;

	/* First compare the keywords */
	for (i = 0; i < KEYWORD_TABLE_SIZE; i++)
	{
		if (rec1->keydata[i] == NULL && rec2->keydata[i] == NULL)
			continue;
		else
			dba_var_diff(rec1->keydata[i], rec2->keydata[i], diffs, out);
	}

	/* Then compare the hash tables */
	for (cur = dba_record_iterate_first(rec1); cur != NULL; cur = dba_record_iterate_next(rec1, cur))
	{
		dba_varcode code = dba_var_code(cur->var);
		if (!dba_record_has_item(rec2, code))
		{
			fprintf(out, "Variable %d%02d%03d only exists in first record\n",
					WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
			(*diffs)++;
		}
		{
			dba_item item2;
			for (item2 = rec2->vars; item2 != NULL; item2 = item2->next)
				if (dba_var_code(item2->var) == code)
				{
					dba_var_diff(cur->var, item2->var, diffs, out);
					break;
				}
		}
	}

	/* Check for the items in the second one not present in the first one */
	for (cur = dba_record_iterate_first(rec2); cur != NULL; cur = dba_record_iterate_next(rec2, cur))
	{
		dba_varcode code = dba_var_code(cur->var);
		if (!dba_record_has_item(rec1, code))
		{
			fprintf(out, "Variable %d%02d%03d only exists in second record\n",
					WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
			(*diffs)++;
		}
	}
}

#endif

static inline int peek_int(const Record& rec, dba_keyword key)
{
	const char* s = rec.key_peek_value(key);
	return s != NULL ? strtol(s, 0, 10) : -1;
}

static inline int min_with_undef(int v1, int v2)
{
	if (v1 == -1)
		return v2;
	if (v2 == -1)
		return v1;
	return v1 < v2 ? v1 : v2;
}

static inline int max_with_undef(int v1, int v2)
{
	if (v1 == -1)
		return v2;
	if (v2 == -1)
		return v1;
	return v1 > v2 ? v1 : v2;
}

static inline int max_days(int y, int m)
{
	int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (m != 2)
		return days[m-1];
	else
		return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28;
}

/* Buf must be at least 25 bytes long; values must be at least 6 ints long */
void Record::parse_date_extremes(int* minvalues, int* maxvalues) const
{
	dba_keyword names[] = { DBA_KEY_YEAR, DBA_KEY_MONTH, DBA_KEY_DAY, DBA_KEY_HOUR, DBA_KEY_MIN, DBA_KEY_SEC };
	dba_keyword min_names[] = { DBA_KEY_YEARMIN, DBA_KEY_MONTHMIN, DBA_KEY_DAYMIN, DBA_KEY_HOURMIN, DBA_KEY_MINUMIN, DBA_KEY_SECMIN };
	dba_keyword max_names[] = { DBA_KEY_YEARMAX, DBA_KEY_MONTHMAX, DBA_KEY_DAYMAX, DBA_KEY_HOURMAX, DBA_KEY_MINUMAX, DBA_KEY_SECMAX };
	int i;

	/* Get the year */

	for (i = 0; i < 6; i++)
	{
		int val = peek_int(*this, names[i]);
		int min = peek_int(*this, min_names[i]);
		int max = peek_int(*this, max_names[i]);

		minvalues[i] = max_with_undef(val, min);
		maxvalues[i] = min_with_undef(val, max);

		if (i > 0 &&
		  ((minvalues[i-1] == -1 && minvalues[i] != -1) ||
		   (maxvalues[i-1] == -1 && maxvalues[i] != -1)))
		{
			Varinfo key1 = keyword_info(names[i - 1]);
			Varinfo key2 = keyword_info(names[i]);

			error_consistency::throwf("%s extremes are unset but %s extremes are set",
					key1->desc, key2->desc);
		}
	}

	/* Now values is either 6 times -1, 6 values, or X values followed by 6-X times -1 */

	/* If one of the extremes has been selected, fill in the blanks */

	if (minvalues[0] != -1)
	{
		minvalues[1] = minvalues[1] != -1 ? minvalues[1] : 1;
		minvalues[2] = minvalues[2] != -1 ? minvalues[2] : 1;
		minvalues[3] = minvalues[3] != -1 ? minvalues[3] : 0;
		minvalues[4] = minvalues[4] != -1 ? minvalues[4] : 0;
		minvalues[5] = minvalues[5] != -1 ? minvalues[5] : 0;
	}

	if (maxvalues[0] != -1)
	{
		maxvalues[1] = maxvalues[1] != -1 ? maxvalues[1] : 12;
		maxvalues[2] = maxvalues[2] != -1 ? maxvalues[2] : max_days(maxvalues[0], maxvalues[1]);
		maxvalues[3] = maxvalues[3] != -1 ? maxvalues[3] : 23;
		maxvalues[4] = maxvalues[4] != -1 ? maxvalues[4] : 59;
		maxvalues[5] = maxvalues[5] != -1 ? maxvalues[5] : 59;
	}
}

}

/* vim:set ts=4 sw=4: */
