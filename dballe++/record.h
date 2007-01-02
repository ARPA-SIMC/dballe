#ifndef DBALLE_CPP_RECORD_H
#define DBALLE_CPP_RECORD_H

#include <dballe/core/record.h>

#include <dballe++/var.h>

namespace dballe {

class RecordIterator
{
	dba_record m_rec;
	dba_record_cursor m_cur;

public:
	RecordIterator(dba_record rec)
		: m_rec(rec), m_cur(dba_record_iterate_first(rec)) {}
	RecordIterator(dba_record rec, dba_record_cursor cur)
		: m_rec(rec), m_cur(cur) {}

	// Iterator methods
	// operator->() is missing for simplicity's sake

	bool operator==(const RecordIterator& rc)
	{
		return m_cur == rc.m_cur;
	}
	bool operator!=(const RecordIterator& rc)
	{
		return m_cur != rc.m_cur;
	}
	RecordIterator& operator++()
	{
		m_cur = dba_record_iterate_next(m_rec, m_cur);
		return *this;
	}
	RecordIterator operator++(int)
	{
		RecordIterator res(*this);
		m_cur = dba_record_iterate_next(m_rec, m_cur);
		return res;
	}
	Var operator*() const
	{
		dba_var res;
		checked(dba_var_copy(dba_record_cursor_variable(m_cur), &res));
		return res;
	}

	// Bindings-friendly methods
	
	bool valid() const { return m_cur != NULL; }
	bool next() { ++(*this); return valid(); }
	Var var() const { return **this; }
};

class Record
{
	dba_record m_rec;

	dba_varcode stringToVar(const std::string& str)
	{
		const char* s = str.c_str();
		return DBA_STRING_TO_VAR(s + 1);
	}

public:
	typedef Var value_type;
	typedef RecordIterator iterator;

	/// Wraps an existing dba_record, taking charge of memory allocation
	Record(dba_record rec) : m_rec(rec) {}

	Record(const Record& rec)
	{
		checked(dba_record_create(&m_rec));
		checked(dba_record_copy(m_rec, rec.rec()));
	}
	Record()
	{
		checked(dba_record_create(&m_rec));
	}
	~Record()
	{
		dba_record_delete(m_rec);
	}

	Record& operator=(const Record& rec)
	{
		if (m_rec != rec.rec())
			checked(dba_record_copy(m_rec, rec.rec()));
		return *this;
	}

	Record copy() const
	{
		return Record(*this);
	}

	void add(const Record& rec)
	{
		checked(dba_record_add(m_rec, rec.rec()));
	}

	Record difference(const Record& rec) const
	{
		Record res;
		checked(dba_record_difference(res.rec(), m_rec, rec.rec()));
		return res;
	}

	bool equals(const Record& rec) const
	{
		return dba_record_equals(m_rec, rec.rec()) == 1;
	}

	void clear() { dba_record_clear(m_rec); }
	void clearVars() { dba_record_clear_vars(m_rec); }

	bool contains(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyContains(dba_record_keyword_byname(parm.c_str()));
		else
			return varContains(stringToVar(parm));
	}
	bool keyContains(dba_keyword parameter)
	{
		int found;
		checked(dba_record_contains_key(m_rec, parameter, &found));
		return found != 0;
	}
	bool varContains(dba_varcode code)
	{
		int found;
		checked(dba_record_contains_var(m_rec, code, &found));
		return found != 0;
	}

	Var enq(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyEnq(dba_record_keyword_byname(parm.c_str()));
		else
			return varEnq(stringToVar(parm));
	}
	Var keyEnq(dba_keyword parameter)
	{
		dba_var var;
		checked(dba_record_key_enq(m_rec, parameter, &var));
		return Var(var);
	}
	Var varEnq(dba_varcode code)
	{
		dba_var var;
		checked(dba_record_var_enq(m_rec, code, &var));
		return Var(var);
	}
	int enqi(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyEnqi(dba_record_keyword_byname(parm.c_str()));
		else
			return varEnqi(stringToVar(parm));
	}
	int keyEnqi(dba_keyword parameter)
	{
		int res;
		checked(dba_record_key_enqi(m_rec, parameter, &res));
		return res;
	}
	int varEnqi(dba_varcode code)
	{
		int res;
		checked(dba_record_var_enqi(m_rec, code, &res));
		return res;
	}
	double enqd(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyEnqd(dba_record_keyword_byname(parm.c_str()));
		else
			return varEnqd(stringToVar(parm));
	}
	double keyEnqd(dba_keyword parameter)
	{
		double res;
		checked(dba_record_key_enqd(m_rec, parameter, &res));
		return res;
	}
	double varEnqd(dba_varcode code)
	{
		double res;
		checked(dba_record_var_enqd(m_rec, code, &res));
		return res;
	}
	const char* enqc(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyEnqc(dba_record_keyword_byname(parm.c_str()));
		else
			return varEnqc(stringToVar(parm));
	}
	const char* keyEnqc(dba_keyword parameter)
	{
		const char* res;
		checked(dba_record_key_enqc(m_rec, parameter, &res));
		return res;
	}
	const char* varEnqc(dba_varcode code)
	{
		const char* res;
		checked(dba_record_var_enqc(m_rec, code, &res));
		return res;
	}
	std::string enqs(const std::string& parm)
	{
		if (parm[0] != 'B')
			return keyEnqs(dba_record_keyword_byname(parm.c_str()));
		else
			return varEnqs(stringToVar(parm));
	}
	std::string keyEnqs(dba_keyword parameter)
	{
		const char* res;
		checked(dba_record_key_enqc(m_rec, parameter, &res));
		return res;
	}
	std::string varEnqs(dba_varcode code)
	{
		const char* res;
		checked(dba_record_var_enqc(m_rec, code, &res));
		return res;
	}

	void set(const std::string& parm, Var& var)
	{
		if (parm[0] != 'B')
			keySet(dba_record_keyword_byname(parm.c_str()), var);
		else
			varSet(stringToVar(parm), var);
	}
	void keySet(dba_keyword parameter, Var& var)
	{
		checked(dba_record_key_set(m_rec, parameter, var.var()));
	}
	void varSet(dba_varcode code, Var& var)
	{
		checked(dba_record_var_set(m_rec, code, var.var()));
	}
	void set(Var& var) { varSet(var); }
	void varSet(Var& var)
	{
		checked(dba_record_var_set_direct(m_rec, var.var()));
	}

	void set(const std::string& parm, int value) { seti(parm, value); }
	void seti(const std::string& parm, int value)
	{
		if (parm[0] != 'B')
			keySeti(dba_record_keyword_byname(parm.c_str()), value);
		else
			varSeti(stringToVar(parm), value);
	}
	void keySet(dba_keyword parameter, int value) { keySeti(parameter, value); }
	void keySeti(dba_keyword parameter, int value)
	{
		checked(dba_record_key_seti(m_rec, parameter, value));
	}
	void varSet(dba_varcode code, int value) { varSeti(code, value); }
	void varSeti(dba_varcode code, int value)
	{
		checked(dba_record_var_seti(m_rec, code, value));
	}
	void set(const std::string& parm, double value) { setd(parm, value); }
	void setd(const std::string& parm, double value)
	{
		if (parm[0] != 'B')
			keySetd(dba_record_keyword_byname(parm.c_str()), value);
		else
			varSetd(stringToVar(parm), value);
	}
	void keySet(dba_keyword parameter, double value) { keySetd(parameter, value); }
	void keySetd(dba_keyword parameter, double value)
	{
		checked(dba_record_key_setd(m_rec, parameter, value));
	}
	void varSet(dba_varcode code, double value) { varSetd(code, value); }
	void varSetd(dba_varcode code, double value)
	{
		checked(dba_record_var_setd(m_rec, code, value));
	}
	void set(const std::string& parm, const char* value) { setc(parm, value); }
	void setc(const std::string& parm, const char* value)
	{
		if (parm[0] != 'B')
			keySetc(dba_record_keyword_byname(parm.c_str()), value);
		else
			varSetc(stringToVar(parm), value);
	}
	void keySet(dba_keyword parameter, const char* value) { keySetc(parameter, value); }
	void keySetc(dba_keyword parameter, const char* value)
	{
		checked(dba_record_key_setc(m_rec, parameter, value));
	}
	void varSet(dba_varcode code, const char* value) { varSetc(code, value); }
	void varSetc(dba_varcode code, const char* value)
	{
		checked(dba_record_var_setc(m_rec, code, value));
	}
	void set(const std::string& parm, const std::string& value) { sets(parm, value); }
	void sets(const std::string& parm, const std::string& value)
	{
		if (parm[0] != 'B')
			keySets(dba_record_keyword_byname(parm.c_str()), value);
		else
			varSets(stringToVar(parm), value);
	}
	void keySet(dba_keyword parameter, const std::string& value) { keySets(parameter, value); }
	void keySets(dba_keyword parameter, const std::string& value)
	{
		checked(dba_record_key_setc(m_rec, parameter, value.c_str()));
	}
	void varSet(dba_varcode code, const std::string& value) { varSets(code, value); }
	void varSets(dba_varcode code, const std::string& value)
	{
		checked(dba_record_var_setc(m_rec, code, value.c_str()));
	}

	void setFromString(const std::string& assignment)
	{
		checked(dba_record_set_from_string(m_rec, assignment.c_str()));
	}

	void unset(const std::string& parm)
	{
		if (parm[0] != 'B')
			keyUnset(dba_record_keyword_byname(parm.c_str()));
		else
			varUnset(stringToVar(parm));
	}
	void keyUnset(dba_keyword parameter)
	{
		checked(dba_record_key_unset(m_rec, parameter));
	}
	void varUnset(dba_varcode code)
	{
		checked(dba_record_var_unset(m_rec, code));
	}

	void setAnaContext()
	{
		checked(dba_record_set_ana_context(m_rec));
	}

	const dba_record rec() const
	{
		return m_rec;
	}
	dba_record rec()
	{
		return m_rec;
	}

	iterator begin()
	{
		return RecordIterator(m_rec);
	}
	iterator end()
	{
		return RecordIterator(m_rec, 0);
	}

	void dumpToStderr();
};

}

#endif
