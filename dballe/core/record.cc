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
namespace core {

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

unique_ptr<dballe::Record> Record::clone() const
{
    return unique_ptr<dballe::Record>(new Record(*this));
}

const Record& Record::downcast(const dballe::Record& record)
{
    const Record* ptr = dynamic_cast<const Record*>(&record);
    if (!ptr)
        throw error_consistency("record given is not a core::Record");
    return *ptr;
}

Record& Record::downcast(dballe::Record& record)
{
    Record* ptr = dynamic_cast<Record*>(&record);
    if (!ptr)
        throw error_consistency("record given is not a core::Record");
    return *ptr;
}

wreport::Var& Record::obtain(const char* name)
{
    Varcode code = 0;
    if (name[0] != 'B' && (code = varcode_alias_resolve(name)) == 0)
    {
        dba_keyword param = keyword_byname(name);
        if (param == DBA_KEY_ERROR)
            error_notfound::throwf("invalid parameter \"%s\"", name);
        return obtain(param);
    } else {
        if (code == 0)
            code = WR_STRING_TO_VAR(name + 1);
        return obtain(code);
    }
}

wreport::Var& Record::obtain(dba_keyword key)
{
    if (keydata[key] == NULL)
        keydata[key] = new Var(keyword_info(key));
    return *keydata[key];
}

wreport::Var& Record::obtain(wreport::Varcode key)
{
    int pos = find_item(key);
    if (pos == -1)
    {
        // Insertion sort the new variable

        // Enlarge the buffer
        m_vars.resize(m_vars.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth it */
        for (pos = m_vars.size() - 1; pos > 0; --pos)
            if (m_vars[pos - 1]->code() > key)
                m_vars[pos] = m_vars[pos - 1];
            else
                break;
        m_vars[pos] = newvar(key).release();
    }
    return *m_vars[pos];
}

void Record::seti(const char* key, int val)
{
    if (val == MISSING_INT)
        unset(key);
    else
        obtain(key).seti(val);
}
void Record::setd(const char* key, double val)
{
    obtain(key).setd(val);
}
void Record::setc(const char* key, const char* val)
{
    if (!val)
        unset(key);
    else
    {
        obtain(key).setc(val);
    }
}
void Record::sets(const char* key, const std::string& val)
{
    obtain(key).setc(val.c_str());
}
void Record::setf(const char* key, const char* val)
{
    // See https://github.com/ARPA-SIMC/dballe/issues/29
    auto& var = obtain(key);
    switch (var.info()->type)
    {
        case Vartype::String:
        case Vartype::Binary:
            var.setf(val);
            break;
        case Vartype::Decimal:
        case Vartype::Integer:
            if (strcmp(val, "-") == 0)
                unset(key);
            else
                var.setf(val);
            break;
    }
}

void Record::set_datetime(const Datetime& dt)
{
    if (dt.is_missing())
    {
        key_unset(DBA_KEY_YEAR);
        key_unset(DBA_KEY_MONTH);
        key_unset(DBA_KEY_DAY);
        key_unset(DBA_KEY_HOUR);
        key_unset(DBA_KEY_MIN);
        key_unset(DBA_KEY_SEC);
    } else {
        seti("year",  (int)dt.year);
        seti("month", (int)dt.month);
        seti("day",   (int)dt.day);
        seti("hour",  (int)dt.hour);
        seti("min",   (int)dt.minute);
        seti("sec",   (int)dt.second);
    }
    key_unset(DBA_KEY_YEARMIN);
    key_unset(DBA_KEY_MONTHMIN);
    key_unset(DBA_KEY_DAYMIN);
    key_unset(DBA_KEY_HOURMIN);
    key_unset(DBA_KEY_MINUMIN);
    key_unset(DBA_KEY_SECMIN);
    key_unset(DBA_KEY_YEARMAX);
    key_unset(DBA_KEY_MONTHMAX);
    key_unset(DBA_KEY_DAYMAX);
    key_unset(DBA_KEY_HOURMAX);
    key_unset(DBA_KEY_MINUMAX);
    key_unset(DBA_KEY_SECMAX);
}

void Record::set_datetimerange(const DatetimeRange& range)
{
    if (range.is_missing())
    {
        key_unset(DBA_KEY_YEAR);
        key_unset(DBA_KEY_MONTH);
        key_unset(DBA_KEY_DAY);
        key_unset(DBA_KEY_HOUR);
        key_unset(DBA_KEY_MIN);
        key_unset(DBA_KEY_SEC);
        key_unset(DBA_KEY_YEARMIN);
        key_unset(DBA_KEY_MONTHMIN);
        key_unset(DBA_KEY_DAYMIN);
        key_unset(DBA_KEY_HOURMIN);
        key_unset(DBA_KEY_MINUMIN);
        key_unset(DBA_KEY_SECMIN);
        key_unset(DBA_KEY_YEARMAX);
        key_unset(DBA_KEY_MONTHMAX);
        key_unset(DBA_KEY_DAYMAX);
        key_unset(DBA_KEY_HOURMAX);
        key_unset(DBA_KEY_MINUMAX);
        key_unset(DBA_KEY_SECMAX);
    } else if (range.min == range.max) {
        set(range.min);
    } else if (range.min.is_missing()) {
        key_unset(DBA_KEY_YEAR);
        key_unset(DBA_KEY_MONTH);
        key_unset(DBA_KEY_DAY);
        key_unset(DBA_KEY_HOUR);
        key_unset(DBA_KEY_MIN);
        key_unset(DBA_KEY_SEC);
        key_unset(DBA_KEY_YEARMIN);
        key_unset(DBA_KEY_MONTHMIN);
        key_unset(DBA_KEY_DAYMIN);
        key_unset(DBA_KEY_HOURMIN);
        key_unset(DBA_KEY_MINUMIN);
        key_unset(DBA_KEY_SECMIN);
        seti("yearmax", range.max.year);
        seti("monthmax", range.max.month);
        seti("daymax", range.max.day);
        seti("hourmax", range.max.hour);
        seti("minumax", range.max.minute);
        seti("secmax", range.max.second);
    } else if (range.max.is_missing()) {
        key_unset(DBA_KEY_YEAR);
        key_unset(DBA_KEY_MONTH);
        key_unset(DBA_KEY_DAY);
        key_unset(DBA_KEY_HOUR);
        key_unset(DBA_KEY_MIN);
        key_unset(DBA_KEY_SEC);
        seti("yearmin", range.min.year);
        seti("monthmin", range.min.month);
        seti("daymin", range.min.day);
        seti("hourmin", range.min.hour);
        seti("minumin", range.min.minute);
        seti("secmin", range.min.second);
        key_unset(DBA_KEY_YEARMAX);
        key_unset(DBA_KEY_MONTHMAX);
        key_unset(DBA_KEY_DAYMAX);
        key_unset(DBA_KEY_HOURMAX);
        key_unset(DBA_KEY_MINUMAX);
        key_unset(DBA_KEY_SECMAX);
    } else {
        if (range.min.year == range.max.year)
        {
            seti("year", range.min.year);
            key_unset(DBA_KEY_YEARMIN);
            key_unset(DBA_KEY_YEARMAX);
        }
        else
        {
            key_unset(DBA_KEY_YEAR);
            seti("yearmin", range.min.year);
            seti("yearmax", range.max.year);
        }
        if (range.min.month == range.max.month)
        {
            seti("month", range.min.month);
            key_unset(DBA_KEY_MONTHMIN);
            key_unset(DBA_KEY_MONTHMAX);
        }
        else
        {
            key_unset(DBA_KEY_MONTH);
            seti("monthmin", range.min.month);
            seti("monthmax", range.max.month);
        }
        if (range.min.day == range.max.day)
        {
            seti("day", range.min.day);
            key_unset(DBA_KEY_DAYMIN);
            key_unset(DBA_KEY_DAYMAX);
        }
        else
        {
            key_unset(DBA_KEY_DAY);
            seti("daymin", range.min.day);
            seti("daymax", range.max.day);
        }
        if (range.min.hour == range.max.hour)
        {
            seti("hour", range.min.hour);
            key_unset(DBA_KEY_HOURMIN);
            key_unset(DBA_KEY_HOURMAX);
        }
        else
        {
            key_unset(DBA_KEY_HOUR);
            seti("hourmin", range.min.hour);
            seti("hourmax", range.max.hour);
        }
        if (range.min.minute == range.max.minute)
        {
            seti("min", range.min.minute);
            key_unset(DBA_KEY_MINUMIN);
            key_unset(DBA_KEY_MINUMAX);
        }
        else
        {
            key_unset(DBA_KEY_MIN);
            seti("minumin", range.min.minute);
            seti("minumax", range.max.minute);
        }
        if (range.min.second == range.max.second)
        {
            seti("sec", range.min.second);
            key_unset(DBA_KEY_SECMIN);
            key_unset(DBA_KEY_SECMAX);
        }
        else
        {
            key_unset(DBA_KEY_SEC);
            seti("secmin", range.min.second);
            seti("secmax", range.max.second);
        }
    }
}

void Record::set_level(const Level& lev)
{
    if (lev.ltype1 == MISSING_INT)
        unset("leveltype1");
    else
        seti("leveltype1", lev.ltype1);

    if (lev.l1 == MISSING_INT)
        unset("l1");
    else
        seti("l1", lev.l1);

    if (lev.ltype2 == MISSING_INT)
        unset("leveltype2");
    else
        seti("leveltype2", lev.ltype2);

    if (lev.l2 == MISSING_INT)
        unset("l2");
    else
        seti("l2", lev.l2);
}

void Record::set_trange(const Trange& tr)
{
    if (tr.pind == MISSING_INT)
        unset("pindicator");
    else
        seti("pindicator", tr.pind);

    if (tr.p1 == MISSING_INT)
        unset("p1");
    else
        seti("p1", tr.p1);

    if (tr.p2 == MISSING_INT)
        unset("p2");
    else
        seti("p2", tr.p2);
}

void Record::set_var(const wreport::Var& var)
{
    if (var.isset())
        obtain(var.code()).setval(var);
    else
        var_unset(var.code());
}

void Record::set_var_acquire(std::unique_ptr<wreport::Var>&& var)
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

void Record::set_latrange(const LatRange& lr)
{
    if (lr.is_missing())
    {
        unset("lat");
        unset("latmin");
        unset("latmax");
    } else if (lr.imin == lr.imax) {
        seti("lat", lr.imin);
        unset("latmin");
        unset("latmax");
    } else {
        unset("lat");
        seti("latmin", lr.imin);
        seti("latmax", lr.imax);
    }
}

void Record::set_lonrange(const LonRange& lr)
{
    if (lr.is_missing())
    {
        unset("lon");
        unset("lonmin");
        unset("lonmax");
    } else if (lr.imin == lr.imax) {
        seti("lon", lr.imin);
        unset("lonmin");
        unset("lonmax");
    } else {
        unset("lon");
        seti("lonmin", lr.imin);
        seti("lonmax", lr.imax);
    }
}

const wreport::Var* Record::get(const char* key) const
{
    Varcode code = 0;
    if (key[0] != 'B' && (code = varcode_alias_resolve(key)) == 0)
    {
        dba_keyword param = keyword_byname(key);
        if (param == DBA_KEY_ERROR)
            error_notfound::throwf("invalid parameter \"%s\"", key);
        return key_peek(param);
    } else {
        if (code == 0)
            code = WR_STRING_TO_VAR(key + 1);
        return var_peek(code);
    }
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

bool Record::equals(const dballe::Record& other) const
{
    const auto& rec = downcast(other);

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

void Record::add(const dballe::Record& rec)
{
    const auto& source = downcast(rec);

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
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (!source2.keydata[i])
        {
            // Has been deleted: skip
            delete keydata[i];
            keydata[i] = nullptr;
        } else if (!source1.keydata[i]) {
            // Has been added in source2: add
            *keydata[i] = *source2.keydata[i];
        } else if (source1.keydata[i] != source2.keydata[i]) {
            // Has been changed in source2: add
            *keydata[i] = *source2.keydata[i];
        } else {
            // Has not been changed: skip
            delete keydata[i];
            keydata[i] = nullptr;
        }
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

bool Record::contains(const dballe::Record& rec) const
{
    const auto& subset = downcast(rec);

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
			error_notfound::throwf("invalid parameter \"%s\"", name);
		return key_unset(param);
	} else {
		if (code == 0)
			code = WR_STRING_TO_VAR(name + 1);
		return var_unset(code);
	}
}

void Record::foreach_key_ref(std::function<void(const char*, const wreport::Var&)> dest) const
{
    // Generate keys
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!keydata[i]->isset()) continue;
        dest(keyword_name((dba_keyword)i), *keydata[i]);
    }
    // Generate variables
    string varcode;
    for (const auto& i: m_vars)
    {
        varcode = varcode_format(i->code());
        dest(varcode.c_str(), *i);
    }
}

void Record::foreach_key_copy(std::function<void(const char*, std::unique_ptr<wreport::Var>&&)> dest) const
{
    // Generate keys
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!keydata[i]->isset()) continue;
        dest(keyword_name((dba_keyword)i), move(newvar(*keydata[i])));
    }
    // Generate variables
    string varcode;
    for (const auto& i: m_vars)
    {
        varcode = varcode_format(i->code());
        dest(varcode.c_str(), move(newvar(*i)));
    }
}

bool Record::iter_keys(std::function<bool(dba_keyword, const wreport::Var&)> f) const
{
    for (unsigned i = 0; i < KEYWORD_TABLE_SIZE; ++i)
    {
        if (keydata[i] == NULL) continue;
        if (!f((dba_keyword)i, *keydata[i])) return false;
    }
    return true;
}


const std::vector<wreport::Var*>& Record::vars() const
{
    return m_vars;
}

Coords Record::get_coords() const
{
    return Coords(
            enq("lat", MISSING_INT),
            enq("lon", MISSING_INT));
}

Ident Record::get_ident() const
{
    Ident ident;
    if (const Var* var = get("ident"))
        ident = var->isset() ? var->enqc() : 0;
    return ident;
}

Level Record::get_level() const
{
    return Level(
            enq("leveltype1", MISSING_INT),
            enq("l1",         MISSING_INT),
            enq("leveltype2", MISSING_INT),
            enq("l2",         MISSING_INT));
}

Trange Record::get_trange() const
{
    return Trange(
            enq("pindicator", MISSING_INT),
            enq("p1",         MISSING_INT),
            enq("p2",         MISSING_INT));
}

Datetime Record::get_datetime() const
{
    if (const Var* var = key_peek(DBA_KEY_YEAR))
        return Datetime::lower_bound(
            var->enqi(),
            enq("month", MISSING_INT),
            enq("day", MISSING_INT),
            enq("hour", MISSING_INT),
            enq("min", MISSING_INT),
            enq("sec", MISSING_INT));
    else
        return Datetime();
}

DatetimeRange Record::get_datetimerange() const
{
    // fetch all values involved in the computation
    int ye = enq("year", MISSING_INT);
    int mo = enq("month", MISSING_INT);
    int da = enq("day", MISSING_INT);
    int ho = enq("hour", MISSING_INT);
    int mi = enq("min", MISSING_INT);
    int se = enq("sec", MISSING_INT);
    int yemin = enq("yearmin", MISSING_INT);
    int momin = enq("monthmin", MISSING_INT);
    int damin = enq("daymin", MISSING_INT);
    int homin = enq("hourmin", MISSING_INT);
    int mimin = enq("minumin", MISSING_INT);
    int semin = enq("secmin", MISSING_INT);
    int yemax = enq("yearmax", MISSING_INT);
    int momax = enq("monthmax", MISSING_INT);
    int damax = enq("daymax", MISSING_INT);
    int homax = enq("hourmax", MISSING_INT);
    int mimax = enq("minumax", MISSING_INT);
    int semax = enq("secmax", MISSING_INT);
    // give absolute values priority over ranges
    if (ye != MISSING_INT) yemin = yemax = ye;
    if (mo != MISSING_INT) momin = momax = mo;
    if (da != MISSING_INT) damin = damax = da;
    if (ho != MISSING_INT) homin = homax = ho;
    if (mi != MISSING_INT) mimin = mimax = mi;
    if (se != MISSING_INT) semin = semax = se;
    return DatetimeRange(yemin, momin, damin, homin, mimin, semin, yemax, momax, damax, homax, mimax, semax);
}

Station Record::get_station() const
{
    Station res;
    if (const Var* var = get("lat"))
        res.coords.lat = var->enqi();
    else
        throw error_notfound("record has no 'lat' set");

    if (const Var* var = get("lon"))
        res.coords.lon = var->enqi();
    else
        throw error_notfound("record has no 'lon' set");

    if (const Var* var = get("ident"))
        res.ident = var->isset() ? var->enqc() : 0;

    if (const Var* var = get("rep_memo"))
    {
        if (var->isset())
            res.report = var->enqs();
        else
            throw error_notfound("record has no 'rep_memo' set");
    }
    return res;
}

DBStation Record::get_dbstation() const
{
    DBStation res;
    if (const Var* var = get("ana_id"))
    {
        // If we have ana_id, the rest is optional
        res.id = var->enqi();
        res.coords.lat = enq("lat", MISSING_INT);
        res.coords.lon = enq("lon", MISSING_INT);
        if (const Var* var = get("ident"))
            res.ident = var->isset() ? var->enqc() : 0;
        res.report = enq("rep_memo", "");
    } else {
        // If we do not have ana_id, we require at least lat, lon and rep_memo
        res.id = MISSING_INT;
        if (const Var* var = get("lat"))
            res.coords.lat = var->enqi();
        else
            throw error_notfound("record has no 'lat' set");

        if (const Var* var = get("lon"))
            res.coords.lon = var->enqi();
        else
            throw error_notfound("record has no 'lon' set");

        if (const Var* var = get("ident"))
            res.ident = var->isset() ? var->enqc() : 0;

        if (const Var* var = get("rep_memo"))
        {
            if (var->isset())
                res.report = var->enqs();
            else
                throw error_notfound("record has no 'rep_memo' set");
        }
    }
    return res;
}


void Record::set_coords(const Coords& c)
{
    seti("lat", c.lat);
    seti("lon", c.lon);
}

void Record::set_station(const Station& s)
{
    set("rep_memo", s.report);
    set_coords(s.coords);
    if (s.ident.is_missing())
    {
        unset("ident");
        seti("mobile", 0);
    } else {
        setc("ident", s.ident);
        seti("mobile", 1);
    }
}

void Record::set_dbstation(const DBStation& s)
{
    set_station(s);
    if (s.id != MISSING_INT)
        set("ana_id", s.id);
    else
        unset("ana_id");
}


void Record::set_from_string(const char* str)
{
    // Split the input as name=val
    const char* s = strchr(str, '=');

    if (!s) error_consistency::throwf("there should be an = between the name and the value in '%s'", str);

    string key(str, s - str);
    setf(key.c_str(), s + 1);
}

void Record::set_from_test_string(const std::string& s)
{
    if (s.empty()) return;
    size_t cur = 0;
    while (true)
    {
        size_t next = s.find(", ", cur);
        if (next == string::npos)
        {
            set_from_string(s.substr(cur).c_str());
            break;
        } else {
            set_from_string(s.substr(cur, next - cur).c_str());
            cur = next + 2;
        }
    }
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

MatchedRecord::MatchedRecord(const Record& r)
    : r(r)
{
}

MatchedRecord::~MatchedRecord()
{
}

matcher::Result MatchedRecord::match_var_id(int val) const
{
    if (const wreport::Var* var = r.get("B33195"))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_station_id(int val) const
{
    if (const wreport::Var* var = r.get("ana_id"))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_station_wmo(int block, int station) const
{
    if (const wreport::Var* var = r.get("B01001"))
    {
        // Match block
        if (var->enqi() != block) return matcher::MATCH_NO;

        // If station was not requested, we are done
        if (station == -1) return matcher::MATCH_YES;

        // Match station
        if (const wreport::Var* var = r.get("B01002"))
        {
            if (var->enqi() != station) return matcher::MATCH_NO;
            return matcher::MATCH_YES;
        }
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_datetime(const DatetimeRange& range) const
{
    Datetime dt = r.get_datetime();
    if (dt.is_missing()) return matcher::MATCH_NA;
    return range.contains(dt) ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedRecord::match_coords(const LatRange& latrange, const LonRange& lonrange) const
{
    matcher::Result r1 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.get("lat"))
        r1 = latrange.contains(var->enqi()) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (latrange.is_missing())
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.get("lon"))
        r2 = lonrange.contains(var->enqi()) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (lonrange.is_missing())
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedRecord::match_rep_memo(const char* memo) const
{
    if (const Var* var = r.get("rep_memo"))
        if (var->isset())
            return strcmp(memo, var->enqc()) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

}
}
