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

#include <extra/test-utils-core.h>
#include <dballe/bufrex/msg.h>
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/file.h>
#include <dballe/msg/marshal.h>
#include <dballe/db/db.h>

#include <string>
#include <vector>
#include <iostream>

namespace tut_dballe {
using namespace std;
using namespace tut;

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
			if (vars != -1) inner_ensure_equals(sset->vars_count, vars, "vars in subset");

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

	TestBufrexMsg() : edition(-1), cat(-1), subcat(-1), checkdigit(-1), subsets(-1) {}
	TestBufrexMsg(const TestBufrexMsg& r)
	{
		edition = r.edition;
		cat = r.cat;
		subcat = r.subcat;
		checkdigit = r.checkdigit;
		subsets = r.subsets;
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

		if (edition != -1) inner_ensure_equals(braw->edition, edition, "edition");
		if (cat != -1) inner_ensure_equals(braw->type, cat, "category");
		if (subcat != -1) inner_ensure_equals(braw->subtype, subcat, "subcategory");
		//if (checkdigit != -1) inner_ensure_equals(braw->opt.crex.checkdigit, checkdigit);
		if (subsets != -1) inner_ensure_equals(braw->subsets_count, (size_t)subsets, "subsets");

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

/* Random message generation functions */

class msg_generator : public generator
{
public:
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
void track_different_msgs(dba_msgs msgs1, dba_msgs msgs2, const std::string& prefix);

dba_err create_dba_db(dba_db* db);

}

// vim:set ts=4 sw=4:
