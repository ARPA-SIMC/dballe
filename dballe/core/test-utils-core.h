/*
 * DB-ALLe - Archive for punctual meteorological data
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

#include <dballe/core/error.h>
#include <dballe/core/record.h>
#include <dballe/core/rawmsg.h>
#include <dballe/core/file.h>

#include <wibble/tests.h>

#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace dballe {
namespace tests {

// Set and unset an extra string marker to be printed in error messages
void test_tag(const std::string& tag);
void test_untag();

#define ensure_varcode_equals(x, y) dballe::tests::_ensure_varcode_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_varcode_equals(x, y) dballe::tests::_ensure_varcode_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
void _ensure_varcode_equals(const wibble::tests::Location& loc, dballe::Varcode actual, dballe::Varcode expected);

#define ensure_var_equals(x, y) dballe::tests::_ensure_var_equals(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_var_equals(x, y) dballe::tests::_ensure_var_equals(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, int val);
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, double val);
void _ensure_var_equals(const wibble::tests::Location& loc, const Var& var, const std::string& val);

#define ensure_var_undef(x) dballe::tests::_ensure_var_undef(wibble::tests::Location(__FILE__, __LINE__, #x " is undef"), (x))
#define inner_ensure_var_undef(x) dballe::tests::_ensure_var_undef(wibble::tests::Location(loc, __FILE__, __LINE__, #x " is undef"), (x))
void _ensure_var_undef(const wibble::tests::Location& loc, const Var& var);

#define ensure_contains(x, y) dballe::tests::impl_ensure_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_contains(x, y) dballe::tests::impl_ensure_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))

static inline void impl_ensure_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) == std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' does not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

#define ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(__FILE__, __LINE__, #x " == " #y), (x), (y))
#define inner_ensure_not_contains(x, y) arki::tests::impl_ensure_not_contains(wibble::tests::Location(loc, __FILE__, __LINE__, #x " == " #y), (x), (y))

static inline void impl_ensure_not_contains(const wibble::tests::Location& loc, const std::string& haystack, const std::string& needle)
{
	if( haystack.find(needle) != std::string::npos )
	{
		std::stringstream ss;
		ss << "'" << haystack << "' must not contain '" << needle << "'";
		throw tut::failure(loc.msg(ss.str()));
	}
}

/*
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, int val)
{
	int v;
	INNER_CHECKED(dba_enqi(rec, key, &v));
	ensure_equals(v, val);
}
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, double val)
{
	double v;
	INNER_CHECKED(dba_enqd(rec, key, &v));
	ensure_equals(v, val);
}
static void _ensureRecordHas(const char* file, int line, dba_record rec, const char* key, const char* val)
{
	const char* v;
	INNER_CHECKED(dba_enqc(rec, key, &v));
	gen_ensure(strcmp(v, val) == 0);
}
#define ensureRecordHas(...) _ensureRecordHas(__FILE__, __LINE__, __VA_ARGS__)
*/

#if 0

class TestRecord
{
protected:
	class Test
	{
	public:
		virtual ~Test() {}
		virtual void setIn(const char* file, int line, dba_record rec) const = 0;
		virtual void checkIn(const char* file, int line, dba_record rec) const = 0;
		virtual Test* clone() const = 0;
		virtual bool sameTest(const Test* test) const = 0;
	};
	template <typename VAL>
	class TestKey : public Test
	{
	protected:
		dba_keyword m_key;
		VAL m_val;
		dba_err rset(dba_record r, dba_keyword k, int v) const { return dba_record_key_seti(r, k, v); }
		dba_err rset(dba_record r, dba_keyword k, double v) const { return dba_record_key_setd(r, k, v); }
		dba_err rset(dba_record r, dba_keyword k, const std::string& v) const { return dba_record_key_setc(r, k, v.c_str()); }
	public:
		TestKey(dba_keyword key, const VAL& val) : m_key(key), m_val(val) {}
		virtual void setIn(const char* file, int line, dba_record rec) const
		{
			INNER_CHECKED(rset(rec, m_key, m_val));
		}
		virtual void checkIn(const char* file, int line, dba_record rec) const
		{
			dba_var var = dba_record_key_peek(rec, m_key);
			try {
				inner_ensure(var != NULL);
				inner_ensure_var_equals(var, m_val);
			} catch (tut::failure& f) {
				//dba_record_print(rec, stderr);
				string msg = f.what();
				dba_varinfo info;
				dba_record_keyword_info(m_key, &info);
				msg += " comparing ";
				msg += info->desc;
				throw tut::failure(msg);
			}
		}
		virtual Test* clone() const { return new TestKey<VAL>(m_key, m_val); }
		virtual bool sameTest(const Test* test) const
		{
			const TestKey* t = dynamic_cast<const TestKey*>(test);
			if (t == NULL)
				return false;
			return t->m_key == m_key;
		}
	};
	template <typename VAL>
	class TestVar : public Test
	{
	protected:
		dba_varcode m_key;
		VAL m_val;
		dba_err rset(dba_record r, dba_varcode k, int v) const { return dba_record_var_seti(r, k, v); }
		dba_err rset(dba_record r, dba_varcode k, double v) const { return dba_record_var_setd(r, k, v); }
		dba_err rset(dba_record r, dba_varcode k, const std::string& v) const { return dba_record_var_setc(r, k, v.c_str()); }
	public:
		TestVar(dba_varcode key, const VAL& val) : m_key(key), m_val(val) {}
		virtual void setIn(const char* file, int line, dba_record rec) const
		{
			INNER_CHECKED(rset(rec, m_key, m_val));
		}
		virtual void checkIn(const char* file, int line, dba_record rec) const
		{
			dba_var var = dba_record_var_peek(rec, m_key);
			try {
				inner_ensure(var != NULL);
				inner_ensure_var_equals(var, m_val);
			} catch (tut::failure& f) {
				//dba_record_print(rec, stderr);
				string msg = f.what();
				dba_varinfo info;
				dba_varinfo_query_local(m_key, &info);
				msg += " comparing ";
				msg += info->desc;
				throw tut::failure(msg);
			}
		}
		virtual Test* clone() const { return new TestVar<VAL>(m_key, m_val); }
		virtual bool sameTest(const Test* test) const
		{
			const TestVar* t = dynamic_cast<const TestVar*>(test);
			if (t == NULL)
				return false;
			return t->m_key == m_key;
		}
	};
	
	std::vector<Test*> tests;
	void addTest(Test* test)
	{
		for (std::vector<Test*>::iterator i = tests.begin();
				i != tests.end(); i++)
			if ((*i)->sameTest(test))
			{
				delete *i;
				*i = test;
				return;
			}
		tests.push_back(test);
	}
public:
	TestRecord() {}
	TestRecord(const TestRecord& r)
	{
		for (std::vector<Test*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
	}
	~TestRecord() throw ()
	{
		for (std::vector<Test*>::iterator i = tests.begin();
				i != tests.end(); i++)
			delete *i;
	}
	TestRecord& operator=(const TestRecord& r)
	{
		for (std::vector<Test*>::iterator i = tests.begin();
				i != tests.end(); i++)
			delete *i;
		tests.clear();
		for (std::vector<Test*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
		return *this;
	}
	void set(dba_keyword key, int val) { addTest(new TestKey<int>(key, val)); }
	void set(dba_keyword key, double val) { addTest(new TestKey<double>(key, val)); }
	void set(dba_keyword key, const char* val) { addTest(new TestKey<std::string>(key, val)); }
	void set(dba_varcode key, int val) { addTest(new TestVar<int>(key, val)); }
	void set(dba_varcode key, double val) { addTest(new TestVar<double>(key, val)); }
	void set(dba_varcode key, const char* val) { addTest(new TestVar<std::string>(key, val)); }

	void _copyTestDataToRecord(const char* file, int line, dba_record rec)
	{
		for (std::vector<Test*>::const_iterator i = tests.begin();
				i != tests.end(); i++)
			(*i)->setIn(file, line, rec);
	}
#define copyTestDataToRecord(rec) _copyTestDataToRecord(__FILE__, __LINE__, rec)

	void ensureEquals(const char* file, int line, dba_record rec)
	{
		for (std::vector<Test*>::const_iterator i = tests.begin();
				i != tests.end(); i++)
			(*i)->checkIn(file, line, rec);
	}
};
#define ensureTestRecEquals(rec, tr) tr.ensureEquals(__FILE__, __LINE__, rec)
#endif

/* Some utility random generator functions */

static inline int rnd(int min, int max)
{
	return min + (int) ((max - min) * (rand() / (RAND_MAX + 1.0)));
}

static inline double rnd(double min, double max)
{
	return min + (int) ((max - min) * (rand() / (RAND_MAX + 1.0)));
}

static inline std::string rnd(int len)
{
	std::string res;
	int max = rnd(1, len);
	for (int i = 0; i < max; i++)
		res += (char)rnd('a', 'z');
	return res;
}

static inline bool rnd(double prob)
{
	return (rnd(0, 100) < prob*100) ? true : false;
}

#if 0
/* Random message generation functions */

class generator
{
	std::vector<dba_record> reused_pseudoana_fixed;
	std::vector<dba_record> reused_pseudoana_mobile;
	std::vector<dba_record> reused_context;

public:
	~generator();

	dba_err fill_pseudoana(dba_record rec, bool mobile);
	dba_err fill_context(dba_record rec);
	dba_err fill_record(dba_record rec);
};
#endif

/* Message reading functions */

/// Return the pathname of a test file
std::string datafile(const std::string& fname);

#if 0
class dba_raw_consumer
{
public:
	virtual ~dba_raw_consumer() {}

	virtual dba_err consume(dba_rawmsg raw) = 0;
};

dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons);
#endif

std::auto_ptr<File> _open_test_data(const wibble::tests::Location& loc, const char* filename, Encoding type);
#define open_test_data(filename, type) _open_test_data(wibble::tests::Location(__FILE__, __LINE__, "open " #filename " " #type), filename, type)

std::auto_ptr<Rawmsg> _read_rawmsg(const wibble::tests::Location& loc, const char* filename, Encoding type);
#define read_rawmsg(filename, type) _read_rawmsg(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), filename, type)

/* Test environment */
class LocalEnv
{
	std::string key;
	std::string oldVal;
public:
	LocalEnv(const std::string& key, const std::string& val)
		: key(key)
	{
		const char* v = getenv(key.c_str());
		oldVal = v == NULL ? "" : v;
		setenv(key.c_str(), val.c_str(), 1);
	}
	~LocalEnv()
	{
		setenv(key.c_str(), oldVal.c_str(), 1);
	}
};

/**
 * Setup the dba_file system so that reading from any file type results in a
 * dba_rawmsg containing the whole file, and writing writes with the default
 * write implementation.
 *
 * The class is a RAII-style class, so it restores everything to its previous
 * state when it goes out of scope.
 */
class DbaFileSlurpOnly
{
	std::vector<File::CreateFun*> oldFuns;

public:
	DbaFileSlurpOnly();
	~DbaFileSlurpOnly();
};

}
}

// vim:set ts=4 sw=4:
