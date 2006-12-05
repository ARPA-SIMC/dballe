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

#include <dballe/err/dba_error.h>
#include <dballe/core/dba_record.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_msg.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/dba_file.h>
#include <dballe/marshal.h>
#include <dballe/db/dba_db.h>

#include <dballe/tests/tut.h>

#include <string>
#include <vector>
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

struct TestBufrexMsg
{
	struct TestBufrexSubset
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
			virtual int checkIn(const char* file, int line, bufrex_subset braw, int start) const
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
	
		std::vector<Test*> tests;
	
	public:
		int vars;
		TestBufrexSubset() : vars(-1) {}
		TestBufrexSubset(const TestBufrexSubset& r)
		{
			vars = r.vars;
			for (std::vector<Test*>::const_iterator i = r.tests.begin();
					i != r.tests.end(); i++)
				tests.push_back((*i)->clone());
		}
		~TestBufrexSubset() {
			for (std::vector<Test*>::iterator i = tests.begin();
					i != tests.end(); i++)
				delete *i;
		}
		TestBufrexSubset& operator=(const TestBufrexSubset& r)
		{
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
		TestBufrexSubset* clone() { return new TestBufrexSubset(*this); }
		template<class VAL>
		void set(dba_varcode code, VAL val) { tests.push_back(new TestVar<VAL>(code, val)); }
		void setUndef(dba_varcode code) { tests.push_back(new TestUndef(code)); }
		void checkIn(const char* file, int line, bufrex_subset sset) const
		{
			if (vars != -1) inner_ensure_equals(sset->vars_count, vars);

			/* Check the variables */
			int start = 0;
			for (std::vector<Test*>::const_iterator i = tests.begin();
					i != tests.end(); i++)
				start = (*i)->checkIn(file, line, sset, start);
		}
	};

protected:
	std::vector<TestBufrexSubset*> tests;

public:
	int edition;
	int cat;
	int subcat;
	int checkdigit;
	int subsets;
	int vars;

	TestBufrexMsg() : edition(-1), cat(-1), subcat(-1), checkdigit(-1), subsets(-1), vars(-1) {}
	TestBufrexMsg(const TestBufrexMsg& r)
	{
		edition = r.edition;
		cat = r.cat;
		subcat = r.subcat;
		checkdigit = r.checkdigit;
		subsets = r.subsets;
		vars = r.vars;
		for (std::vector<TestBufrexSubset*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
	}
	~TestBufrexMsg() throw ()
	{
		for (std::vector<TestBufrexSubset*>::iterator i = tests.begin();
				i != tests.end(); i++)
			if (*i) delete *i;

	}
	TestBufrexMsg& operator=(const TestBufrexMsg& r)
	{
		edition = r.edition;
		cat = r.cat;
		subcat = r.subcat;
		checkdigit = r.checkdigit;
		subsets = r.subsets;
		vars = r.vars;

		for (std::vector<TestBufrexSubset*>::iterator i = tests.begin();
				i != r.tests.end(); i++)
			delete *i;
		tests.clear();
		for (std::vector<TestBufrexSubset*>::const_iterator i = r.tests.begin();
				i != r.tests.end(); i++)
			tests.push_back((*i)->clone());
		return *this;
	}
	TestBufrexSubset& subset(size_t idx)
	{
		if (idx >= tests.size())
			tests.resize(idx+1, 0);
		if (tests[idx] == 0)
			tests[idx] = new TestBufrexSubset;
		return *tests[idx];
	}
	void ensureEquals(const char* file, int line, bufrex_msg braw)
	{
		/* First check the metadata */

		if (edition != -1) inner_ensure_equals(braw->edition, edition);
		if (cat != -1) inner_ensure_equals(braw->type, cat);
		if (subcat != -1) inner_ensure_equals(braw->subtype, subcat);
		//if (checkdigit != -1) inner_ensure_equals(braw->opt.crex.checkdigit, checkdigit);
		if (subsets != -1) inner_ensure_equals(braw->subsets_count, subsets);

		/* Then check the variables */
		for (size_t i = 0; i < braw->subsets_count; ++i)
			if (i < tests.size() && tests[i])
				tests[i]->checkIn(file, line, braw->subsets[i]);
	}
};
#define ensureBufrexRawEquals(tr, br) tr.ensureEquals(__FILE__, __LINE__, br)

dba_msgs _read_test_msg(const char* file, int line, const char* filename, dba_encoding type);
#define read_test_msg(filename, type) _read_test_msg(__FILE__, __LINE__, filename, type)

bufrex_msg _read_test_msg_raw(const char* file, int line, const char* filename, dba_encoding type);
#define read_test_msg_raw(filename, type) _read_test_msg_raw(__FILE__, __LINE__, filename, type)

bufrex_msg _reencode_test(const char* file, int line, bufrex_msg msg);
#define reencode_test(filename) _reencode_test(__FILE__, __LINE__, msg)


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
	dba_err fill_message(dba_msg msg, bool mobile);
};


/* Message reading functions */

class dba_raw_consumer
{
public:
	virtual ~dba_raw_consumer() {}

	virtual dba_err consume(dba_rawmsg raw) = 0;
};

dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons);

class msg_vector : public dba_raw_consumer, public std::vector<dba_msgs>
{
public:
	virtual ~msg_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			dba_msgs_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_msgs msgs;

		DBA_RUN_OR_RETURN(dba_marshal_decode(raw, &msgs));
		push_back(msgs);

		return dba_error_ok();
	}
};
	
class bufrex_vector : public dba_raw_consumer, public std::vector<bufrex_msg>
{
public:
	virtual ~bufrex_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			bufrex_msg_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_err err = DBA_OK;
		bufrex_msg msg;

		switch (raw->encoding)
		{
			case BUFR:
				DBA_RUN_OR_RETURN(bufrex_msg_create(&msg, BUFREX_BUFR));
				break;
			case CREX:
				DBA_RUN_OR_RETURN(bufrex_msg_create(&msg, BUFREX_CREX));
				break;
			default:
				return dba_error_consistency("unhandled message type");
		}
		DBA_RUN_OR_GOTO(fail, bufrex_msg_decode(msg, raw));
		push_back(msg);

		return dba_error_ok();

	fail:
		bufrex_msg_delete(msg);
		return err;
	}
};

void track_different_msgs(dba_msg msg1, dba_msg msg2, const std::string& prefix);

dba_err create_dba_db(dba_db* db);

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

}

// vim:set ts=4 sw=4:
