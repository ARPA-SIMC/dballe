#ifndef DBALLE_CPP_RECORD_H
#define DBALLE_CPP_RECORD_H

#include <dballe/core/record.h>

#include <dballe++/var.h>

namespace dballe {

/**
 * Iterate the variables inside a record
 */
class RecordIterator
{
	dba_record m_rec;
        dba_keyword m_kwd;
	dba_record_cursor m_cur;

public:
	RecordIterator(dba_record rec);
	RecordIterator(dba_record rec, bool)
                : m_rec(rec), m_kwd(DBA_KEY_COUNT), m_cur(0) {}

	// Iterator methods
	// operator->() is missing for simplicity's sake

	bool operator==(const RecordIterator& rc);
	bool operator!=(const RecordIterator& rc);
	RecordIterator& operator++();
	RecordIterator operator++(int);
	Var operator*() const;

        bool isKeyword() const;
        dba_keyword keyword() const { return m_kwd; }
        const char* keywordName() const;

	// Bindings-friendly methods
	
	bool valid() const { return m_kwd != DBA_KEY_COUNT || m_cur != NULL; }
	bool next() { ++(*this); return valid(); }
	Var var() const { return **this; }
};

/**
 * Iterate the variables inside a record
 */
class RecordVarIterator
{
	dba_record m_rec;
	dba_record_cursor m_cur;

public:
	RecordVarIterator(dba_record rec)
		: m_rec(rec), m_cur(dba_record_iterate_first(rec)) {}
	RecordVarIterator(dba_record rec, dba_record_cursor cur)
		: m_rec(rec), m_cur(cur) {}

	// Iterator methods
	// operator->() is missing for simplicity's sake

	bool operator==(const RecordVarIterator& rc)
	{
		return m_cur == rc.m_cur;
	}
	bool operator!=(const RecordVarIterator& rc)
	{
		return m_cur != rc.m_cur;
	}
	RecordVarIterator& operator++()
	{
		m_cur = dba_record_iterate_next(m_rec, m_cur);
		return *this;
	}
	RecordVarIterator operator++(int)
	{
		RecordVarIterator res(*this);
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

/**
 * Wrap a dba_record
 */
class Record
{
	dba_record m_rec;

	/// Parse a string with a variable description (like "B01012") into a
	/// dba_varcode
	dba_varcode stringToVar(const std::string& str) const
	{
		const char* s = str.c_str();
		return DBA_STRING_TO_VAR(s + 1);
	}

public:
	typedef Var value_type;
	typedef RecordIterator iterator;
	typedef RecordVarIterator var_iterator;

	/// Wraps an existing dba_record, taking charge of memory allocation
	Record(dba_record rec) : m_rec(rec) {}

	/// Copy constructor
	Record(const Record& rec)
	{
		checked(dba_record_create(&m_rec));
		checked(dba_record_copy(m_rec, rec.rec()));
	}
	/// Create an empty record
	Record()
	{
		checked(dba_record_create(&m_rec));
	}
	~Record()
	{
		dba_record_delete(m_rec);
	}

	/// Assignment with value copy semantics
	Record& operator=(const Record& rec)
	{
		if (m_rec != rec.rec())
			checked(dba_record_copy(m_rec, rec.rec()));
		return *this;
	}

	/// Create a copy of this record
	Record copy() const
	{
		return Record(*this);
	}

	/// Add to this record the contents of another record
	void add(const Record& rec)
	{
		checked(dba_record_add(m_rec, rec.rec()));
	}

	/// Create a record with only those fields that change this record into
	/// the given record
	Record difference(const Record& rec) const
	{
		Record res;
		checked(dba_record_difference(res.rec(), m_rec, rec.rec()));
		return res;
	}

	/// Check if the two records have the same content
	bool equals(const Record& rec) const
	{
		return dba_record_equals(m_rec, rec.rec()) == 1;
	}

	/// Completely empty the record
	void clear() { dba_record_clear(m_rec); }
	/// Remove all the variables from the record, but leave the context
	/// information
	void clearVars() { dba_record_clear_vars(m_rec); }

	/// Check if the record contains the given parameter or value
	bool contains(const std::string& parm) const;
	/// Check if the record contains the given parameter
	bool keyContains(dba_keyword parameter) const
	{
		int found;
		checked(dba_record_contains_key(m_rec, parameter, &found));
		return found != 0;
	}
	/// Check if the record contains the given value
	bool varContains(dba_varcode code) const
	{
		int found;
		checked(dba_record_contains_var(m_rec, code, &found));
		return found != 0;
	}

	/// Get the Var representation of a parameter or value
	Var enq(const std::string& parm) const;
        /// enq as operator[]
        Var operator[](const std::string& parm) const { return enq(parm); }
	/// Get the Var representation of a parameter
	Var keyEnq(dba_keyword parameter) const
	{
		dba_var var;
		checked(dba_record_key_enq(m_rec, parameter, &var));
		return Var(var);
	}
        /// keyEnq as operator[]
        Var operator[](dba_keyword parameter) const { return keyEnq(parameter); }
	/// Get the Var representation of a value
	Var varEnq(dba_varcode code) const
	{
		dba_var var;
		checked(dba_record_var_enq(m_rec, code, &var));
		return Var(var);
	}
        /// varEnq as operator[]
        Var operator[](dba_varcode code) const { return varEnq(code); }
	/// Get the unscaled integer representation of a parameter or value
	int enqi(const std::string& parm) const;
	/// Get the unscaled integer representation of a parameter
	int keyEnqi(dba_keyword parameter) const
	{
		int res;
		checked(dba_record_key_enqi(m_rec, parameter, &res));
		return res;
	}
	/// Get the unscaled integer representation of a value
	int varEnqi(dba_varcode code) const
	{
		int res;
		checked(dba_record_var_enqi(m_rec, code, &res));
		return res;
	}
	/// Get the double representation of a parameter or value
	double enqd(const std::string& parm) const;
	/// Get the double representation of a parameter
	double keyEnqd(dba_keyword parameter) const
	{
		double res;
		checked(dba_record_key_enqd(m_rec, parameter, &res));
		return res;
	}
	/// Get the double representation of a value
	double varEnqd(dba_varcode code) const
	{
		double res;
		checked(dba_record_var_enqd(m_rec, code, &res));
		return res;
	}
	/// Get the string representation of a parameter or value
	const char* enqc(const std::string& parm) const;
	/// Get the string representation of a parameter
	const char* keyEnqc(dba_keyword parameter) const
	{
		const char* res;
		checked(dba_record_key_enqc(m_rec, parameter, &res));
		return res;
	}
	/// Get the string representation of a value
	const char* varEnqc(dba_varcode code) const
	{
		const char* res;
		checked(dba_record_var_enqc(m_rec, code, &res));
		return res;
	}
	/// Get the string representation of a parameter or value
	std::string enqs(const std::string& parm) const;
	/// Get the string representation of a parameter
	std::string keyEnqs(dba_keyword parameter) const
	{
		const char* res;
		checked(dba_record_key_enqc(m_rec, parameter, &res));
		return res;
	}
	/// Get the string representation of a value
	std::string varEnqs(dba_varcode code) const
	{
		const char* res;
		checked(dba_record_var_enqc(m_rec, code, &res));
		return res;
	}

	/// Set a parameter or value from a Var
	void set(const std::string& parm, const Var& var);
	/// Set a parameter from a Var
	void keySet(dba_keyword parameter, const Var& var)
	{
		checked(dba_record_key_set(m_rec, parameter, var.var()));
	}
	/// Set a value from a Var
	void varSet(dba_varcode code, const Var& var)
	{
		checked(dba_record_var_set(m_rec, code, var.var()));
	}
	/// Set a value from a Var
	void set(const Var& var) { varSet(var); }
	/// Set a value from a Var
	void varSet(const Var& var)
	{
		checked(dba_record_var_set_direct(m_rec, var.var()));
	}

	/// Set a parameter or value from an unscaled int
	void set(const std::string& parm, int value) { seti(parm, value); }
	/// Set a parameter or value from an unscaled int
	void seti(const std::string& parm, int value);
	/// Set a parameter from an unscaled int
	void keySet(dba_keyword parameter, int value) { keySeti(parameter, value); }
	/// Set a parameter from an unscaled int
	void keySeti(dba_keyword parameter, int value)
	{
		checked(dba_record_key_seti(m_rec, parameter, value));
	}
	/// Set a value from an unscaled int
	void varSet(dba_varcode code, int value) { varSeti(code, value); }
	/// Set a value from an unscaled int
	void varSeti(dba_varcode code, int value)
	{
		checked(dba_record_var_seti(m_rec, code, value));
	}
	/// Set a parameter or value from a double
	void set(const std::string& parm, double value) { setd(parm, value); }
	/// Set a parameter or value from a double
	void setd(const std::string& parm, double value);
	/// Set a parameter from a double
	void keySet(dba_keyword parameter, double value) { keySetd(parameter, value); }
	/// Set a parameter from a double
	void keySetd(dba_keyword parameter, double value)
	{
		checked(dba_record_key_setd(m_rec, parameter, value));
	}
	/// Set a value from a double
	void varSet(dba_varcode code, double value) { varSetd(code, value); }
	/// Set a value from a double
	void varSetd(dba_varcode code, double value)
	{
		checked(dba_record_var_setd(m_rec, code, value));
	}
	/// Set a parameter or value from a string
	void set(const std::string& parm, const char* value) { setc(parm, value); }
	/// Set a parameter or value from a string
	void setc(const std::string& parm, const char* value);
	/// Set a parameter from a string
	void keySet(dba_keyword parameter, const char* value) { keySetc(parameter, value); }
	/// Set a parameter from a string
	void keySetc(dba_keyword parameter, const char* value)
	{
		checked(dba_record_key_setc(m_rec, parameter, value));
	}
	/// Set a value from a string
	void varSet(dba_varcode code, const char* value) { varSetc(code, value); }
	/// Set a value from a string
	void varSetc(dba_varcode code, const char* value)
	{
		checked(dba_record_var_setc(m_rec, code, value));
	}
	/// Set a parameter or value from a string
	void set(const std::string& parm, const std::string& value) { sets(parm, value); }
	/// Set a parameter or value from a string
	void sets(const std::string& parm, const std::string& value);
	/// Set a parameter from a string
	void keySet(dba_keyword parameter, const std::string& value) { keySets(parameter, value); }
	/// Set a parameter from a string
	void keySets(dba_keyword parameter, const std::string& value)
	{
		checked(dba_record_key_setc(m_rec, parameter, value.c_str()));
	}
	/// Set a value from a string
	void varSet(dba_varcode code, const std::string& value) { varSets(code, value); }
	/// Set a value from a string
	void varSets(dba_varcode code, const std::string& value)
	{
		checked(dba_record_var_setc(m_rec, code, value.c_str()));
	}

	/// Set a record parameter or value from a string in the form
	/// "parm=val" or "Bxxyyy=val"
	void setFromString(const std::string& assignment)
	{
		checked(dba_record_set_from_string(m_rec, assignment.c_str()));
	}

	/// Unset a parameter or value
	void unset(const std::string& parm);
	/// Unset a parameter
	void keyUnset(dba_keyword parameter)
	{
		checked(dba_record_key_unset(m_rec, parameter));
	}
	/// Unset a value
	void varUnset(dba_varcode code)
	{
		checked(dba_record_var_unset(m_rec, code));
	}

	/// Set the record parameters to represent the pseudoana context
	void setAnaContext()
	{
		checked(dba_record_set_ana_context(m_rec));
	}

	/// Return the underlying dba_record
	const dba_record rec() const
	{
		return m_rec;
	}
	/// Return the underlying dba_record
	dba_record rec()
	{
		return m_rec;
	}

	/**
	 * Iterators on all the contents
	 * @{
	 */
	iterator begin()
	{
		return RecordIterator(m_rec);
	}
	iterator end()
	{
		return RecordIterator(m_rec, false);
	}
	/** @} */

	/**
	 * Iterators on the values
	 * @{
	 */
	var_iterator varbegin()
	{
		return RecordVarIterator(m_rec);
	}
	var_iterator varend()
	{
		return RecordVarIterator(m_rec, 0);
	}
	/** @} */

	/// Dump the record contents to standard error
	void dumpToStderr();
};

}

#endif
