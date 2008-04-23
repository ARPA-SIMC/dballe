#ifndef DBALLE_CPP_MSG_H
#define DBALLE_CPP_MSG_H

#include <dballe/msg/msg.h>

#include <dballe++/var.h>

namespace dballe {

/**
 * Wrap a dba_var
 */
class Msg
{
	dba_msg m_msg;

	/// Copy constructor
	Msg(const Msg& var);
	/// Assignment with copy semantics
	Msg& operator=(const Msg& var);
public:
	/// Wraps an existing dba_var, taking charge of memory allocation
	Msg();
	~Msg();

	void set(Var& var, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
	{
		checked(dba_msg_set(m_msg, var.var(), code, ltype1, l1, ltype2, l2, pind, p1, p2));
	}

	void setd(dba_varcode code, double val, int conf, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2)
	{
		checked(dba_msg_setd(m_msg, code, val, conf, ltype1, l1, ltype2, l2, pind, p1, p2));
	}

	/// Access the underlying dba_var
	const dba_msg msg() const
	{
		return m_msg;
	}
	/// Access the underlying dba_var
	dba_msg msg()
	{
		return m_msg;
	}

	std::string encodeBUFR(int type, int subtype, int localsubtype);
};

}

#endif
