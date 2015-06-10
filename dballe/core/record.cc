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
            error_notfound::throwf("looking for misspelled parameter \"%s\"", name);
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
        obtain(key).setc(val);
}
void Record::sets(const char* key, const std::string& val)
{
    obtain(key).setc(val.c_str());
}
void Record::setf(const char* key, const char* val)
{
    obtain(key).set_from_formatted(val);
}

void Record::set_datetime(const Datetime& dt)
{
    if (dt.is_missing())
    {
        unset("year");
        unset("month");
        unset("day");
        unset("hour");
        unset("min");
        unset("sec");
    } else {
        seti("year",  (int)dt.year);
        seti("month", (int)dt.month);
        seti("day",   (int)dt.day);
        seti("hour",  (int)dt.hour);
        seti("min",   (int)dt.minute);
        seti("sec",   (int)dt.second);
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
    if (var.value())
        obtain(var.code()).copy_val(var);
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
    return peek(key);
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
        return Datetime(
            var->enqi(),
            enq("month", 1),
            enq("day", 1),
            enq("hour", 0),
            enq("min", 0),
            enq("sec", 0));
    else
        return Datetime();
}

Datetime Record::get_datetimemin() const
{
    if (const Var* var = key_peek(DBA_KEY_YEARMIN))
        return Datetime(
            var->enqi(),
            enq("monthmin", 1),
            enq("daymin", 1),
            enq("hourmin", 0),
            enq("minumin", 0),
            enq("secmin", 0));
    else
        return Datetime();
}

Datetime Record::get_datetimemax() const
{
    if (const Var* var = key_peek(DBA_KEY_YEARMAX))
    {
        int year = var->enqi();
        int month = enq("monthmax", 12);
        int day = enq("daymax", 0);
        if (day == 0) day = Date::days_in_month(year, month);
        return Datetime(year, month, day,
            enq("hourmax", 23),
            enq("minumax", 59),
            enq("secmax", 59));
    } else
        return Datetime();
}

void Record::unset_datetime()
{
    key_unset(DBA_KEY_YEAR);
    key_unset(DBA_KEY_MONTH);
    key_unset(DBA_KEY_DAY);
    key_unset(DBA_KEY_HOUR);
    key_unset(DBA_KEY_MIN);
    key_unset(DBA_KEY_SEC);
}

void Record::unset_datetimemin()
{
    key_unset(DBA_KEY_YEARMIN);
    key_unset(DBA_KEY_MONTHMIN);
    key_unset(DBA_KEY_DAYMIN);
    key_unset(DBA_KEY_HOURMIN);
    key_unset(DBA_KEY_MINUMIN);
    key_unset(DBA_KEY_SECMIN);
}

void Record::unset_datetimemax()
{
    key_unset(DBA_KEY_YEARMAX);
    key_unset(DBA_KEY_MONTHMAX);
    key_unset(DBA_KEY_DAYMAX);
    key_unset(DBA_KEY_HOURMAX);
    key_unset(DBA_KEY_MINUMAX);
    key_unset(DBA_KEY_SECMAX);
}

void Record::setmin(const Datetime& dt)
{
    seti("yearmin",  (int)dt.year);
    seti("monthmin", (int)dt.month);
    seti("daymin",   (int)dt.day);
    seti("hourmin",  (int)dt.hour);
    seti("minumin",  (int)dt.minute);
    seti("secmin",   (int)dt.second);
}

void Record::setmax(const Datetime& dt)
{
    seti("yearmax",  (int)dt.year);
    seti("monthmax", (int)dt.month);
    seti("daymax",   (int)dt.day);
    seti("hourmax",  (int)dt.hour);
    seti("minumax",  (int)dt.minute);
    seti("secmax",   (int)dt.second);
}

void Record::set_coords(const Coords& c)
{
    seti("lat", c.lat);
    seti("lon", c.lon);
}

void Record::set_ana_context()
{
    seti("year", 1000);
    seti("month", 1);
    seti("day", 1);
    seti("hour", 0);
    seti("min", 0);
    seti("sec", 0);
    key_unset(DBA_KEY_LEVELTYPE1);
    key_unset(DBA_KEY_L1);
    key_unset(DBA_KEY_LEVELTYPE2);
    key_unset(DBA_KEY_L2);
    key_unset(DBA_KEY_PINDICATOR);
    key_unset(DBA_KEY_P1);
    key_unset(DBA_KEY_P2);
}

bool Record::is_ana_context() const
{
    return enq("year", MISSING_INT) == 1000;
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
    } else
        for (unsigned i = 1; i < 6; ++i)
            minvalues[i] = MISSING_INT;

    if (maxvalues[0] != MISSING_INT)
    {
        maxvalues[1] = maxvalues[1] != MISSING_INT ? maxvalues[1] : 12;
        maxvalues[2] = maxvalues[2] != MISSING_INT ? maxvalues[2] : max_days(maxvalues[0], maxvalues[1]);
        maxvalues[3] = maxvalues[3] != MISSING_INT ? maxvalues[3] : 23;
        maxvalues[4] = maxvalues[4] != MISSING_INT ? maxvalues[4] : 59;
        maxvalues[5] = maxvalues[5] != MISSING_INT ? maxvalues[5] : 59;
    } else
        for (unsigned i = 1; i < 6; ++i)
            maxvalues[i] = MISSING_INT;
}

/* Buf must be at least 25 bytes long; values must be at least 6 ints long */
void Record::parse_date_extremes(Datetime& dtmin, Datetime& dtmax) const
{
    int raw_min[6];
    int raw_max[6];
    parse_date_extremes(raw_min, raw_max);
    dtmin.from_array(raw_min);
    dtmax.from_array(raw_max);
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

matcher::Result MatchedRecord::match_date(const Datetime& min, const Datetime& max) const
{
    Datetime dt = r.get_datetime().lower_bound();
    if (dt.is_missing()) return matcher::MATCH_NA;
    return Matched::date_in_range(dt, min, max);
}

matcher::Result MatchedRecord::match_coords(const LatRange& latrange, const LonRange& lonrange) const
{
    matcher::Result r1 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.key_peek(DBA_KEY_LAT))
        r1 = latrange.contains(var->enqi()) ? matcher::MATCH_YES : matcher::MATCH_NO;
    else if (latrange.is_missing())
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (const wreport::Var* var = r.key_peek(DBA_KEY_LON))
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
    if (const char* var = r.key_peek_value(DBA_KEY_REP_MEMO))
    {
        return strcmp(memo, var) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

}
}
