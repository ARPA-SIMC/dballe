#ifndef DBALLE_CPP_BUFREX_H
#define DBALLE_CPP_BUFREG_H

#include <dballe/bufrex/msg.h>
#include <dballe++/var.h>

namespace dballe {

class BufrexSubset
{
	// No need for memory management here, as we're always part of a bufrex_msg
	bufrex_subset m_sub;

public:
	BufrexSubset(bufrex_subset sub) : m_sub(sub) {}

	void reset();
	void truncate(int idx);
	void appendvar(dba_varcode code, const Var& var);
	void appendvar(const Var& var);

	bool empty() const { return m_sub->vars_count == 0; }
	size_t size() const { return m_sub->vars_count; }

	Var operator[](int idx) const { return getVar(idx); }
	Var getVar(int idx) const;

	void appendi(dba_varcode code, int val)
	{
		checked(bufrex_subset_store_variable_i(m_sub, code, val));
	}
	void appendd(dba_varcode code, double val)
	{
		checked(bufrex_subset_store_variable_d(m_sub, code, val));
	}
	void appendc(dba_varcode code, const char* val)
	{
		checked(bufrex_subset_store_variable_c(m_sub, code, val));
	}
	void appends(dba_varcode code, const std::string& val)
	{
		checked(bufrex_subset_store_variable_c(m_sub, code, val.c_str()));
	}
	void appendu(dba_varcode code)
	{
		checked(bufrex_subset_store_variable_undef(m_sub, code));
	}

	// Overloaded version
	void append(dba_varcode code, const Var& var) { appendvar(code, var); }
	void append(const Var& var) { appendvar(var); }
	void append(dba_varcode code, int val) { appendi(code, val); }
	void append(dba_varcode code, double val) { appendd(code, val); }
	void append(dba_varcode code, const char* val) { appendc(code, val); }
	void append(dba_varcode code, const std::string& val) { appends(code, val); }
	void append(dba_varcode code) { appendu(code); }
};

/**
 * Wrap a dba_var
 */
class Bufrex
{
	bufrex_msg m_msg;

	Bufrex(bufrex_msg msg);
public:
	/// Smart pointer assignment
	Bufrex(const Bufrex& var);
	~Bufrex();

	/// Smart pointer assignment
	Bufrex& operator=(const Bufrex& var);

	static Bufrex createBUFR(int centre, int subcentre,
					         int mastertable, int localtable,
					         bool compressed = false);

	void setTemplate(int type, int subtype, int localsubtype);
	void setEdition(int edition);
	void setTime(int year, int month, int day, int hour, int minute, int second);

	void appendDatadesc(dba_varcode code);

	std::string encode();

	bool empty() const { return m_msg->subsets_count == 0; }
	size_t size() const { return m_msg->subsets_count; }

	//ConstSubset operator[](int idx) const { return getSubset(idx); }
	BufrexSubset operator[](int idx) { return getSubset(idx); }
	//ConstSubset getSubset(int idx) const;
	BufrexSubset getSubset(int idx);

	BufrexSubset append();

/*
	void set(Var& var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2)
	{
		checked(dba_msg_set(m_msg, var.var(), code, ltype, l1, l2, pind, p1, p2));
	}

	void setd(dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2)
	{
		checked(dba_msg_setd(m_msg, code, val, conf, ltype, l1, l2, pind, p1, p2));
	}
*/

	/// Access the underlying bufrex_msg
	const bufrex_msg msg() const
	{
		return m_msg;
	}
	/// Access the underlying bufrex_msg
	bufrex_msg msg()
	{
		return m_msg;
	}
};

}

#endif
