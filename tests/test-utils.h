#include <dballe/err/dba_error.h>
#include <dballe/core/dba_record.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/dba_file.h>

#include <tut.h>

#include <string>

#include <iostream>

#define TESTGRP(name) \
typedef test_group<name ## _shar> tg; \
typedef tg::object to; \
tg name ## _tg (#name);

namespace tut_dballe {
using namespace std;
using namespace tut;

class DBAException : public std::exception
{
protected:
	std::string m_what;
public:
	DBAException(const char* file, int line);
	virtual ~DBAException() throw () {}

	virtual const char* what() const throw () { return m_what.c_str(); }
};

// Set and unset an extra string marker to be printed in error messages
void test_tag(const std::string& tag);
void test_untag();

#define CHECKED(...) if (__VA_ARGS__ != DBA_OK) throw DBAException(__FILE__, __LINE__)
#define INNER_CHECKED(...) if (__VA_ARGS__ != DBA_OK) throw DBAException(file, line)

std::string __ensure_errmsg(std::string f, int l, std::string msg);
#define gen_ensure(x) ensure (__ensure_errmsg(__FILE__, __LINE__, #x).c_str(), (x))
#define inner_ensure(x) ensure (__ensure_errmsg(file, line, #x).c_str(), (x))

template <class T,class Q>
void my_ensure_equals(const char* file, int line, const Q& actual, const T& expected)
{
	if( expected != actual )
	{
		std::stringstream ss;
		ss << "expected " << expected << " actual " << actual;
		throw failure(__ensure_errmsg(file, line, ss.str()));
	}
}
#define gen_ensure_equals(x, y) my_ensure_equals(__FILE__, __LINE__, (x), (y))
#define inner_ensure_equals(x, y) my_ensure_equals(file, line, (x), (y))

void _ensure_var_undef(const char* file, int line, dba_var var);
void _ensure_var_equals(const char* file, int line, dba_var var, int val);
void _ensure_var_equals(const char* file, int line, dba_var var, double val);
void _ensure_var_equals(const char* file, int line, dba_var var, const string& val);

#define gen_ensure_var_equals(x, y) _ensure_var_equals(__FILE__, __LINE__, (x), (y))
#define inner_ensure_var_equals(x, y) _ensure_var_equals(file, line, (x), (y))
#define gen_ensure_var_undef(x) _ensure_var_undef(__FILE__, __LINE__, (x))
#define inner_ensure_var_undef(x) _ensure_var_undef(file, line, (x))

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
			inner_ensure(var != NULL);
			inner_ensure_var_equals(var, m_val);
		}
		virtual Test* clone() const { return new TestKey<VAL>(m_key, m_val); }
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
			inner_ensure(var != NULL);
			inner_ensure_var_equals(var, m_val);
		}
		virtual Test* clone() const { return new TestVar<VAL>(m_key, m_val); }
	};
	
	std::vector<Test*> tests;
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
				i != r.tests.end(); i++)
			delete *i;
		tests.clear();
		for (std::vector<Test*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
		return *this;
	}
	void set(dba_keyword key, int val) { tests.push_back(new TestKey<int>(key, val)); }
	void set(dba_keyword key, double val) { tests.push_back(new TestKey<double>(key, val)); }
	void set(dba_keyword key, const char* val) { tests.push_back(new TestKey<std::string>(key, val)); }
	void set(dba_varcode key, int val) { tests.push_back(new TestVar<int>(key, val)); }
	void set(dba_varcode key, double val) { tests.push_back(new TestVar<double>(key, val)); }
	void set(dba_varcode key, const char* val) { tests.push_back(new TestVar<std::string>(key, val)); }

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
#define ensureTestRecEquals(tr, rec) tr.ensureEquals(__FILE__, __LINE__, rec)

class TestBufrexRaw
{
protected:
	class Test
	{
	protected:
		dba_varcode m_code;
		virtual void check_var(const char* file, int line, dba_var var) const = 0;
		virtual std::string toString() const = 0;
	public:
		Test(dba_varcode code) : m_code(code) {}
		virtual ~Test() {}
		virtual int checkIn(const char* file, int line, bufrex_raw braw, int start) const
		{
			for (int i = start; i < braw->vars_count; i++)
				if (dba_var_code(braw->vars[i]) == m_code)
				{
					check_var(file, line, braw->vars[i]);
					return i + 1;
				}
			throw failure(__ensure_errmsg(file, line, "did not find a variable for test " + toString()));
			return 0;
		}
		virtual Test* clone() = 0;
	};
	class TestUndef : public Test
	{
	protected:
		virtual void check_var(const char* file, int line, dba_var var) const
		{
			inner_ensure_var_undef(var);
		}
		virtual std::string toString() const
		{
			ostringstream res;
			switch (DBA_VAR_F(m_code))
			{
				case 0: res << 'B'; break;
				case 1: res << 'R'; break;
				case 2: res << 'C'; break;
				case 3: res << 'D'; break;
				default: break;
			}
			//os << std::setfill('0') << std::setw(2) << DBA_VAR_X(code);
			//os << std::setfill('0') << std::setw(3) << DBA_VAR_Y(code);
			res << DBA_VAR_X(m_code);
			res << DBA_VAR_Y(m_code);
			res << ":undef";
			//res << (dba_varcode)m_code << ":" << m_val;
			return res.str();
		}
	public:
		TestUndef(dba_varcode code) : Test(code) {}
		virtual Test* clone() { return new TestUndef(m_code); }
	};
	template<class VAL>
	class TestVar : public Test
	{
	protected:
		VAL m_val;
		virtual void check_var(const char* file, int line, dba_var var) const
		{
			inner_ensure_var_equals(var, m_val);
		}
		virtual std::string toString() const
		{
			ostringstream res;
			switch (DBA_VAR_F(m_code))
			{
				case 0: res << 'B'; break;
				case 1: res << 'R'; break;
				case 2: res << 'C'; break;
				case 3: res << 'D'; break;
				default: break;
			}
			//os << std::setfill('0') << std::setw(2) << DBA_VAR_X(code);
			//os << std::setfill('0') << std::setw(3) << DBA_VAR_Y(code);
			res << DBA_VAR_X(m_code);
			res << DBA_VAR_Y(m_code);
			res << ":" << m_val;
			//res << (dba_varcode)m_code << ":" << m_val;
			return res.str();
		}
	public:
		TestVar(dba_varcode code, const VAL& val) : Test(code), m_val(val) {}
		virtual Test* clone() { return new TestVar<VAL>(m_code, m_val); }
	};
	
	int edition;
	int cat;
	int subcat;
	//int checkdigit;
	int vars;

	std::vector<Test*> tests;
public:
	TestBufrexRaw() : edition(-1), cat(-1), subcat(-1)/*, checkdigit(-1)*/, vars(-1) {}
	TestBufrexRaw(const TestBufrexRaw& r)
	{
		edition = r.edition;
		cat = r.cat;
		subcat = r.subcat;
		//checkdigit = r.checkdigit;
		vars = r.vars;
		for (std::vector<Test*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
	}
	~TestBufrexRaw() throw ()
	{
		for (std::vector<Test*>::iterator i = tests.begin();
				i != tests.end(); i++)
			delete *i;
	}
	TestBufrexRaw& operator=(const TestBufrexRaw& r)
	{
		edition = r.edition;
		cat = r.cat;
		subcat = r.subcat;
		//checkdigit = r.checkdigit;
		vars = r.vars;

		for (std::vector<Test*>::iterator i = tests.begin();
				i != r.tests.end(); i++)
			delete *i;
		tests.clear();
		for (std::vector<Test*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
		return *this;
	}
	void setEdition(int val) { edition = val; }
	void setCat(int val) { cat = val; }
	void setSubcat(int val) { subcat = val; }
	//void setCheckdigit(int val) { checkdigit = val; }
	void setVars(int val) { vars = val; }
	template<class VAL>
	void set(dba_varcode code, VAL val) { tests.push_back(new TestVar<VAL>(code, val)); }
	void setUndef(dba_varcode code) { tests.push_back(new TestUndef(code)); }
	void ensureEquals(const char* file, int line, bufrex_raw braw)
	{
		/* First check the metadata */

		/*
		CHECKED(bufrex_raw_get_edition(braw, &val));
		inner_ensure_equals(val, edition);
		*/

		inner_ensure_equals(braw->type, cat);
		inner_ensure_equals(braw->subtype, subcat);

		//inner_ensure_equals(val, checkdigit);

		inner_ensure_equals(braw->vars_count, vars);

		/* Then check the variables */
		int start = 0;
		for (std::vector<Test*>::const_iterator i = tests.begin();
				i != tests.end(); i++)
			start = (*i)->checkIn(file, line, braw, start);
	}
};
#define ensureBufrexRawEquals(tr, br) tr.ensureEquals(__FILE__, __LINE__, br)

dba_msg _read_test_msg(const char* file, int line, const char* filename, dba_encoding type);
#define read_test_msg(filename, type) _read_test_msg(__FILE__, __LINE__, filename, type)

bufrex_raw _read_test_msg_raw(const char* file, int line, const char* filename, dba_encoding type);
#define read_test_msg_raw(filename, type) _read_test_msg_raw(__FILE__, __LINE__, filename, type)

bufrex_raw _reencode_test(const char* file, int line, bufrex_raw msg);
#define reencode_test(filename) _reencode_test(__FILE__, __LINE__, msg)

}
