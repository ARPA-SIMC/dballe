#ifndef DBALLE_CPP_VAR_H
#define DBALLE_CPP_VAR_H

#include <dballe/core/var.h>

#include <dballe++/error.h>

namespace dballe {

class Varinfo
{
	dba_varinfo m_info;
	
public:
	Varinfo(dba_varinfo info) : m_info(info) {}

	dba_varcode var() const { return m_info->var; }
	const char* desc() const { return m_info->desc; }
	const char* unit() const { return m_info->unit; }
	int scale() const { return m_info->scale; }
	int ref() const { return m_info->ref; }
	int len() const { return m_info->len; }
	int bit_ref() const { return m_info->bit_ref; }
	int bit_len() const { return m_info->bit_len; }
	bool is_string() const { return m_info->is_string != 0; }

	static Varinfo create(dba_varcode code);
};

class Var
{
	dba_var m_var;

public:
	/// Wraps an existing dba_var, taking charge of memory allocation
	Var(dba_var var) : m_var(var) {}
	Var(const Var& var)
	{
		checked(dba_var_copy(var.var(), &m_var));
	}
	Var(dba_varcode code)
	{
		checked(dba_var_create_local(code, &m_var));
	}
	Var(dba_varinfo info)
	{
		checked(dba_var_create(info, &m_var));
	}
	Var(dba_varinfo info, int val)
	{
		checked(dba_var_createi(info, val, &m_var));
	}
	Var(dba_varinfo info, double val)
	{
		checked(dba_var_created(info, val, &m_var));
	}
	Var(dba_varinfo info, const char* val)
	{
		checked(dba_var_createc(info, val, &m_var));
	}
	Var(dba_varinfo info, const std::string& val)
	{
		checked(dba_var_createc(info, val.c_str(), &m_var));
	}
	~Var()
	{
		if (m_var)
			dba_var_delete(m_var);
	}

	Var& operator=(const Var& var)
	{
		dba_var_delete(m_var);
		m_var = 0;
		checked(dba_var_copy(var.var(), &m_var));
		return *this;
	}

	Var copy() const
	{
		dba_var res;
		checked(dba_var_copy(m_var, &res));
		return Var(res);
	}

	bool equals(const Var& var) const
	{
		return dba_var_equals(m_var, var.var()) == 1;
	}

	int enqi()
	{
		int res;
		checked(dba_var_enqi(m_var, &res));
		return res;
	}
	double enqd()
	{
		double res;
		checked(dba_var_enqd(m_var, &res));
		return res;
	}
	const char* enqc()
	{
		const char* res;
		checked(dba_var_enqc(m_var, &res));
		return res;
	}
	std::string enqs()
	{
		const char* res;
		checked(dba_var_enqc(m_var, &res));
		return res;
	}

	void set(int val)
	{
		checked(dba_var_seti(m_var, val));
	}
	void set(double val)
	{
		checked(dba_var_setd(m_var, val));
	}
	void set(const char* val)
	{
		checked(dba_var_setc(m_var, val));
	}
	void set(const std::string& val)
	{
		checked(dba_var_setc(m_var, val.c_str()));
	}

	void unset()
	{
		checked(dba_var_unset(m_var));
	}

	dba_varcode code()
	{
		return dba_var_code(m_var);
	}
	
	Varinfo info()
	{
		return Varinfo(dba_var_info(m_var));
	}

	std::string format(const std::string& nullValue = "(undef)")
	{
		if (dba_var_value(m_var) == NULL)
			return nullValue;
		else if (dba_var_info(m_var)->is_string)
			return dba_var_value(m_var);
		else
		{
			dba_varinfo info = dba_var_info(m_var);
			char buf[20];
			snprintf(buf, 20, "%.*f", info->scale > 0 ? info->scale : 0, enqd());
			return buf;
		}
	}
	
	const char* raw()
	{
		return dba_var_value(m_var);
	}

	const dba_var var() const
	{
		return m_var;
	}
	dba_var var()
	{
		return m_var;
	}
};

}

#endif
