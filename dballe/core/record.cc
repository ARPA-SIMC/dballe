/*
 * dballe/record - groups of related variables
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "defs.h"

#include "config.h"

#include <sstream>
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

std::ostream& operator<<(std::ostream& o, dba_keyword k)
{
    return o << Record::keyword_name(k);
}

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

bool Record::contains(const Record& subset) const
{
	// Compare the keyword tables
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
	{
		if (subset.keydata[i] != NULL && keydata[i] == NULL)
			return false;
		if (subset.keydata[i] != NULL && keydata[i] != NULL
		 && *subset.keydata[i] != *keydata[i])
			return false;
	}

	// Compare the values
	vector<Var*>::const_iterator s1 = m_vars.begin();
	vector<Var*>::const_iterator s2 = subset.m_vars.begin();
	while (s1 != m_vars.end() && s2 != subset.m_vars.end())
	{
		if ((*s1)->code() < (*s2)->code())
			// s1 has a value not in s2
			++s1;
		else if ((*s1)->code() == (*s2)->code())
		{
			// they both have the same values
			if (**s1 != **s2)
				return false;
			++s1;
			++s2;
		}
		else if ((*s2)->code() < (*s1)->code())
		{
			// s2 has a value not in s1
			return false;
		}
	}
	return true;
}

bool Record::contains(dba_keyword parameter) const throw()
{
    const Var* res = key_peek(parameter);
    return res ? res->value() != NULL : false;
}

bool Record::contains(wreport::Varcode parameter) const throw()
{
    const Var* res = var_peek(parameter);
    return res ? res->value() != NULL : false;
}

bool Record::contains_level() const throw ()
{
    return contains(DBA_KEY_LEVELTYPE1) || contains(DBA_KEY_L1)
        || contains(DBA_KEY_LEVELTYPE2) || contains(DBA_KEY_L2);
}

bool Record::contains_trange() const throw ()
{
    return contains(DBA_KEY_PINDICATOR) || contains(DBA_KEY_P1) || contains(DBA_KEY_P2);
}

bool Record::contains_datetime() const throw ()
{
    return contains(DBA_KEY_YEAR) || contains(DBA_KEY_MONTH) || contains(DBA_KEY_DAY)
        || contains(DBA_KEY_HOUR) || contains(DBA_KEY_MIN) || contains(DBA_KEY_SEC);
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

const wreport::Var* Record::peek(const char* name) const
{
	Varcode code = 0;
	if (name[0] != 'B' && (code = varcode_alias_resolve(name)) == 0)
	{
		dba_keyword param = keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			error_notfound::throwf("looking for misspelled parameter \"%s\"", name);
		return key_peek(param);
	} else {
		if (code == 0)
			code = WR_STRING_TO_VAR(name + 1);
		return var_peek(code);
	}
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

const char* Record::peek_value(const char* name) const
{
	const Var* var = peek(name);
	if (var == NULL) return NULL;
	return var->value();
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

void Record::add(std::unique_ptr<wreport::Var> var)
{
    int pos = find_item(var->code());
    if (pos == -1)
    {
        // Insertion sort the new variable

        // Enlarge the buffer
        m_vars.resize(m_vars.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth it */
        for (pos = m_vars.size() - 1; pos > 0; --pos)
            if (m_vars[pos - 1]->code() > var->code())
                m_vars[pos] = m_vars[pos - 1];
            else
                break;
    } else
        delete m_vars[pos];
    m_vars[pos] = var.release();
}

const wreport::Var& Record::get(const char* name) const
{
	const Var* var = peek(name);
	if (var == NULL)
		error_notfound::throwf("\"%s\" not found in record", name);
	return *var;
}

wreport::Var& Record::get(const char* name)
{
	Varcode code = 0;
	if (name[0] != 'B' && (code = varcode_alias_resolve(name)) == 0)
	{
		dba_keyword param = keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			error_notfound::throwf("looking for misspelled parameter \"%s\"", name);
		return key(param);
	} else {
		if (code == 0)
			code = WR_STRING_TO_VAR(name + 1);
		return var(code);
	}
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

void Record::unset(const char* name)
{
	Varcode code = 0;
	if (name[0] != 'B' && (code = varcode_alias_resolve(name)) == 0)
	{
		dba_keyword param = keyword_byname(name);
		if (param == DBA_KEY_ERROR)
			error_notfound::throwf("looking for misspelled parameter \"%s\"", name);
		return key_unset(param);
	} else {
		if (code == 0)
			code = WR_STRING_TO_VAR(name + 1);
		return var_unset(code);
	}
}

const std::vector<wreport::Var*>& Record::vars() const
{
	return m_vars;
}

void Record::set(const Level& lev)
{
    if (lev.ltype1 == MISSING_INT)
        unset(DBA_KEY_LEVELTYPE1);
    else
        set(DBA_KEY_LEVELTYPE1, lev.ltype1);

    if (lev.l1 == MISSING_INT)
        unset(DBA_KEY_L1);
    else
        set(DBA_KEY_L1, lev.l1);

    if (lev.ltype2 == MISSING_INT)
        unset(DBA_KEY_LEVELTYPE2);
    else
        set(DBA_KEY_LEVELTYPE2, lev.ltype2);

    if (lev.l2 == MISSING_INT)
        unset(DBA_KEY_L2);
    else
        set(DBA_KEY_L2, lev.l2);
}

void Record::set(const Trange& tr)
{
    if (tr.pind == MISSING_INT)
        unset(DBA_KEY_PINDICATOR);
    else
        set(DBA_KEY_PINDICATOR, tr.pind);

    if (tr.p1 == MISSING_INT)
        unset(DBA_KEY_P1);
    else
        set(DBA_KEY_P1, tr.p1);

    if (tr.p2 == MISSING_INT)
        unset(DBA_KEY_P2);
    else
        set(DBA_KEY_P2, tr.p2);
}

Level Record::get_level() const
{
    return Level(
            get(DBA_KEY_LEVELTYPE1, MISSING_INT),
            get(DBA_KEY_L1, MISSING_INT),
            get(DBA_KEY_LEVELTYPE2, MISSING_INT),
            get(DBA_KEY_L2, MISSING_INT));
}

Trange Record::get_trange() const
{
    return Trange(
            get(DBA_KEY_PINDICATOR, MISSING_INT),
            get(DBA_KEY_P1, MISSING_INT),
            get(DBA_KEY_P2, MISSING_INT));
}

void Record::get_datetime(int (&val)[6]) const
{
    val[0] = get(DBA_KEY_YEAR, MISSING_INT);
    val[1] = get(DBA_KEY_MONTH, MISSING_INT);
    val[2] = get(DBA_KEY_DAY, MISSING_INT);
    val[3] = get(DBA_KEY_HOUR, MISSING_INT);
    val[4] = get(DBA_KEY_MIN, MISSING_INT);
    val[5] = get(DBA_KEY_SEC, MISSING_INT);
}

void Record::get_datetimemin(int (&val)[6]) const
{
    val[0] = get(DBA_KEY_YEARMIN, MISSING_INT);
    val[1] = get(DBA_KEY_MONTHMIN, MISSING_INT);
    val[2] = get(DBA_KEY_DAYMIN, MISSING_INT);
    val[3] = get(DBA_KEY_HOURMIN, MISSING_INT);
    val[4] = get(DBA_KEY_MINUMIN, MISSING_INT);
    val[5] = get(DBA_KEY_SECMIN, MISSING_INT);
}

void Record::get_datetimemax(int (&val)[6]) const
{
    val[0] = get(DBA_KEY_YEARMAX, MISSING_INT);
    val[1] = get(DBA_KEY_MONTHMAX, MISSING_INT);
    val[2] = get(DBA_KEY_DAYMAX, MISSING_INT);
    val[3] = get(DBA_KEY_HOURMAX, MISSING_INT);
    val[4] = get(DBA_KEY_MINUMAX, MISSING_INT);
    val[5] = get(DBA_KEY_SECMAX, MISSING_INT);
}

void Record::set_datetime(const int (&val)[6])
{
    if (val[0] == MISSING_INT) unset(DBA_KEY_YEAR);  else set(DBA_KEY_YEAR,  val[0]);
    if (val[1] == MISSING_INT) unset(DBA_KEY_MONTH); else set(DBA_KEY_MONTH, val[1]);
    if (val[2] == MISSING_INT) unset(DBA_KEY_DAY);   else set(DBA_KEY_DAY,   val[2]);
    if (val[3] == MISSING_INT) unset(DBA_KEY_HOUR);  else set(DBA_KEY_HOUR,  val[3]);
    if (val[4] == MISSING_INT) unset(DBA_KEY_MIN);   else set(DBA_KEY_MIN,   val[4]);
    if (val[5] == MISSING_INT) unset(DBA_KEY_SEC);   else set(DBA_KEY_SEC,   val[5]);
}

void Record::set_datetimemin(const int (&val)[6])
{
    if (val[0] == MISSING_INT) unset(DBA_KEY_YEARMIN);  else set(DBA_KEY_YEARMIN,  val[0]);
    if (val[1] == MISSING_INT) unset(DBA_KEY_MONTHMIN); else set(DBA_KEY_MONTHMIN, val[1]);
    if (val[2] == MISSING_INT) unset(DBA_KEY_DAYMIN);   else set(DBA_KEY_DAYMIN,   val[2]);
    if (val[3] == MISSING_INT) unset(DBA_KEY_HOURMIN);  else set(DBA_KEY_HOURMIN,  val[3]);
    if (val[4] == MISSING_INT) unset(DBA_KEY_MINUMIN);  else set(DBA_KEY_MINUMIN,  val[4]);
    if (val[5] == MISSING_INT) unset(DBA_KEY_SECMIN);   else set(DBA_KEY_SECMIN,   val[5]);
}

void Record::set_datetimemax(const int (&val)[6])
{
    if (val[0] == MISSING_INT) unset(DBA_KEY_YEARMAX);  else set(DBA_KEY_YEARMAX,  val[0]);
    if (val[1] == MISSING_INT) unset(DBA_KEY_MONTHMAX); else set(DBA_KEY_MONTHMAX, val[1]);
    if (val[2] == MISSING_INT) unset(DBA_KEY_DAYMAX);   else set(DBA_KEY_DAYMAX,   val[2]);
    if (val[3] == MISSING_INT) unset(DBA_KEY_HOURMAX);  else set(DBA_KEY_HOURMAX,  val[3]);
    if (val[4] == MISSING_INT) unset(DBA_KEY_MINUMAX);  else set(DBA_KEY_MINUMAX,  val[4]);
    if (val[5] == MISSING_INT) unset(DBA_KEY_SECMAX);   else set(DBA_KEY_SECMAX,   val[5]);
}

Datetime Record::get_datetime() const
{
    return Datetime(
        get(DBA_KEY_YEAR).enqi(),
        get(DBA_KEY_MONTH, 1),
        get(DBA_KEY_DAY, 1),
        get(DBA_KEY_HOUR, 0),
        get(DBA_KEY_MIN, 0),
        get(DBA_KEY_SEC, 0));
}

Datetime Record::get_datetimemin() const
{
    return Datetime(
        get(DBA_KEY_YEARMIN).enqi(),
        get(DBA_KEY_MONTHMIN, 1),
        get(DBA_KEY_DAYMIN, 1),
        get(DBA_KEY_HOURMIN, 0),
        get(DBA_KEY_MINUMIN, 0),
        get(DBA_KEY_SECMIN, 0));
}

Datetime Record::get_datetimemax() const
{
    int year = get(DBA_KEY_YEARMAX).enqi();
    int month = get(DBA_KEY_MONTHMAX, 12);
    int day = get(DBA_KEY_DAYMAX, 0);
    if (day == 0) day = Date::days_in_month(year, month);
    return Datetime(year, month, day,
        get(DBA_KEY_HOURMAX, 23),
        get(DBA_KEY_MINUMAX, 59),
        get(DBA_KEY_SECMAX, 59));
}

void Record::unset_datetime()
{
    unset(DBA_KEY_YEAR);
    unset(DBA_KEY_MONTH);
    unset(DBA_KEY_DAY);
    unset(DBA_KEY_HOUR);
    unset(DBA_KEY_MIN);
    unset(DBA_KEY_SEC);
}

void Record::unset_datetimemin()
{
    unset(DBA_KEY_YEARMIN);
    unset(DBA_KEY_MONTHMIN);
    unset(DBA_KEY_DAYMIN);
    unset(DBA_KEY_HOURMIN);
    unset(DBA_KEY_MINUMIN);
    unset(DBA_KEY_SECMIN);
}

void Record::unset_datetimemax()
{
    unset(DBA_KEY_YEARMAX);
    unset(DBA_KEY_MONTHMAX);
    unset(DBA_KEY_DAYMAX);
    unset(DBA_KEY_HOURMAX);
    unset(DBA_KEY_MINUMAX);
    unset(DBA_KEY_SECMAX);
}

void Record::set(const Datetime& dt)
{
    set(DBA_KEY_YEAR,  (int)dt.date.year);
    set(DBA_KEY_MONTH, (int)dt.date.month);
    set(DBA_KEY_DAY,   (int)dt.date.day);
    set(DBA_KEY_HOUR,  (int)dt.time.hour);
    set(DBA_KEY_MIN,   (int)dt.time.minute);
    set(DBA_KEY_SEC,   (int)dt.time.second);
}

void Record::setmin(const Datetime& dt)
{
    set(DBA_KEY_YEARMIN,  (int)dt.date.year);
    set(DBA_KEY_MONTHMIN, (int)dt.date.month);
    set(DBA_KEY_DAYMIN,   (int)dt.date.day);
    set(DBA_KEY_HOURMIN,  (int)dt.time.hour);
    set(DBA_KEY_MINUMIN,  (int)dt.time.minute);
    set(DBA_KEY_SECMIN,   (int)dt.time.second);
}

void Record::setmax(const Datetime& dt)
{
    set(DBA_KEY_YEARMAX,  (int)dt.date.year);
    set(DBA_KEY_MONTHMAX, (int)dt.date.month);
    set(DBA_KEY_DAYMAX,   (int)dt.date.day);
    set(DBA_KEY_HOURMAX,  (int)dt.time.hour);
    set(DBA_KEY_MINUMAX,  (int)dt.time.minute);
    set(DBA_KEY_SECMAX,   (int)dt.time.second);
}

void Record::set_datetime(int ye, int mo, int da, int ho, int mi, int se)
{
    set(DBA_KEY_YEAR,  ye);
    set(DBA_KEY_MONTH, mo);
    set(DBA_KEY_DAY,   da);
    set(DBA_KEY_HOUR,  ho);
    set(DBA_KEY_MIN,   mi);
    set(DBA_KEY_SEC,   se);
}

void Record::set(const Coord& c)
{
    set(DBA_KEY_LAT, c.lat);
    set(DBA_KEY_LON, c.lon);
}

void Record::set_ana_context()
{
	key(DBA_KEY_YEAR).seti(1000);
	key(DBA_KEY_MONTH).seti(1);
	key(DBA_KEY_DAY).seti(1);
	key(DBA_KEY_HOUR).seti(0);
	key(DBA_KEY_MIN).seti(0);
	key(DBA_KEY_SEC).seti(0);
	unset(DBA_KEY_LEVELTYPE1);
	unset(DBA_KEY_L1);
	unset(DBA_KEY_LEVELTYPE2);
	unset(DBA_KEY_L2);
	unset(DBA_KEY_PINDICATOR);
	unset(DBA_KEY_P1);
	unset(DBA_KEY_P2);
}

bool Record::is_ana_context() const
{
    return get(DBA_KEY_YEAR, MISSING_INT) == 1000;
}

void Record::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s) error_consistency::throwf("there should be an = between the name and the value in '%s'", str);

    string key(str, s - str);
    set_from_string(key.c_str(), s + 1);
}

void Record::set_from_string(const char* name, const char* val)
{
    dba_keyword k = Record::keyword_byname(name);

    if (k != DBA_KEY_ERROR)
        key(k).set_from_formatted(val);
    else
        var(resolve_varcode_safe(name)).set_from_formatted(val);
}

std::string Record::to_string() const
{
    std::stringstream s;
    bool first = true;

    for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
        if (keydata[i] != NULL)
        {
            if (first)
                first = false;
            else
                s << ",";
            s << keyword_name((dba_keyword)i) << "=" << keydata[i]->format("");
        }

    for (vector<Var*>::const_iterator i = m_vars.begin(); i != m_vars.end(); ++i)
    {
        if (first)
            first = false;
        else
            s << ",";
        s << wreport::varcode_format((*i)->code()) << "=" << (*i)->format("");
    }

    return s.str();
}

void Record::print(FILE* out) const
{
	for (int i = 0; i < KEYWORD_TABLE_SIZE; ++i)
		if (keydata[i] != NULL)
			keydata[i]->print(out);

	for (vector<Var*>::const_iterator i = m_vars.begin(); i != m_vars.end(); ++i)
		(*i)->print(out);
}


#if 0
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
    return s != NULL ? strtol(s, 0, 10) : MISSING_INT;
}

static inline int min_with_undef(int v1, int v2)
{
    if (v1 == MISSING_INT)
        return v2;
    if (v2 == MISSING_INT)
        return v1;
    return v1 < v2 ? v1 : v2;
}

static inline int max_with_undef(int v1, int v2)
{
	if (v1 == MISSING_INT)
		return v2;
	if (v2 == MISSING_INT)
		return v1;
	return v1 > v2 ? v1 : v2;
}

static inline int max_days(int y, int m)
{
	int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (m != 2)
		return days[m - 1];
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
           ((minvalues[i - 1] == MISSING_INT && minvalues[i] != MISSING_INT) ||
            (maxvalues[i - 1] == MISSING_INT && maxvalues[i] != MISSING_INT)))
		{
			Varinfo key1 = keyword_info(names[i - 1]);
			Varinfo key2 = keyword_info(names[i]);

			error_consistency::throwf("%s extremes are unset but %s extremes are set",
					key1->desc, key2->desc);
		}
	}

	/* Now values is either 6 times MISSING_INT, 6 values, or X values followed by 6-X times MISSING_INT */

	/* If one of the extremes has been selected, fill in the blanks */

    if (minvalues[0] != MISSING_INT)
    {
        minvalues[1] = minvalues[1] != MISSING_INT ? minvalues[1] : 1;
        minvalues[2] = minvalues[2] != MISSING_INT ? minvalues[2] : 1;
        minvalues[3] = minvalues[3] != MISSING_INT ? minvalues[3] : 0;
        minvalues[4] = minvalues[4] != MISSING_INT ? minvalues[4] : 0;
        minvalues[5] = minvalues[5] != MISSING_INT ? minvalues[5] : 0;
    }

    if (maxvalues[0] != MISSING_INT)
    {
        maxvalues[1] = maxvalues[1] != MISSING_INT ? maxvalues[1] : 12;
        maxvalues[2] = maxvalues[2] != MISSING_INT ? maxvalues[2] : max_days(maxvalues[0], maxvalues[1]);
        maxvalues[3] = maxvalues[3] != MISSING_INT ? maxvalues[3] : 23;
        maxvalues[4] = maxvalues[4] != MISSING_INT ? maxvalues[4] : 59;
        maxvalues[5] = maxvalues[5] != MISSING_INT ? maxvalues[5] : 59;
    }
}

void Record::parse_date(int* values) const
{
    dba_keyword names[] = { DBA_KEY_YEAR, DBA_KEY_MONTH, DBA_KEY_DAY, DBA_KEY_HOUR, DBA_KEY_MIN, DBA_KEY_SEC };
    for (int i = 0; i < 6; i++)
    {
        values[i] = peek_int(*this, names[i]);

        if (i > 0 && (values[i-1] == MISSING_INT && values[i] != MISSING_INT))
        {
            Varinfo key1 = keyword_info(names[i - 1]);
            Varinfo key2 = keyword_info(names[i]);

            error_consistency::throwf("%s is unset but %s is set",
                    key1->desc, key2->desc);
        }
    }

    /* Now values is either 6 times MISSING_INT, 6 values, or X values followed by 6-X times MISSING_INT */

    /* If one of the extremes has been selected, fill in the blanks */

    if (values[0] != MISSING_INT)
    {
        values[1] = values[1] != MISSING_INT ? values[1] : 1;
        values[2] = values[2] != MISSING_INT ? values[2] : 1;
        values[3] = values[3] != MISSING_INT ? values[3] : 0;
        values[4] = values[4] != MISSING_INT ? values[4] : 0;
        values[5] = values[5] != MISSING_INT ? values[5] : 0;
    }
}

MatchedRecord::MatchedRecord(const Record& r)
    : r(r)
{
}

MatchedRecord::~MatchedRecord()
{
}

matcher::Result MatchedRecord::match_var_id(int val) const
{
    if (const wreport::Var* var = r.var_peek(WR_VAR(0, 33, 195)))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_station_id(int val) const
{
    if (const wreport::Var* var = r.key_peek(DBA_KEY_ANA_ID))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_station_wmo(int block, int station) const
{
    if (const wreport::Var* var = r.var_peek(WR_VAR(0, 1, 1)))
    {
        // Match block
        if (var->enqi() != block) return matcher::MATCH_NO;

        // If station was not requested, we are done
        if (station == -1) return matcher::MATCH_YES;

        // Match station
        if (const wreport::Var* var = r.var_peek(WR_VAR(0, 1, 2)))
        {
            if (var->enqi() != station) return matcher::MATCH_NO;
            return matcher::MATCH_YES;
        }
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_date(const int* min, const int* max) const
{
    int date[6];
    r.parse_date(date);
    if (date[0] == MISSING_INT) return matcher::MATCH_NA;
    return Matched::date_in_range(date, min, max);
}

matcher::Result MatchedRecord::match_coords(int latmin, int latmax, int lonmin, int lonmax) const
{
    matcher::Result r1 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.key_peek(DBA_KEY_LAT))
        r1 = Matched::int_in_range(var->enqi(), latmin, latmax);
    else if (latmin == MISSING_INT && latmax == MISSING_INT)
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.key_peek(DBA_KEY_LON))
        r2 = Matched::int_in_range(var->enqi(), lonmin, lonmax);
    else if (lonmin == MISSING_INT && lonmax == MISSING_INT)
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_rep_memo(const char* memo) const
{
    if (const char* var = r.key_peek_value(DBA_KEY_REP_MEMO))
    {
        return strcmp(memo, var) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

}

/* vim:set ts=4 sw=4: */
