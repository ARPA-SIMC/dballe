/*
 * wreport/var - Store a value and its informations
 *
 * Copyright (C) 2005,2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>		/* strtod, getenv */
#include <string.h>		/* strncmp */
#include <ctype.h>		/* isspace */
#include <math.h>		/* rint */
#include <assert.h>		/* assert */

#include "var.h"
#include "vartable.h"
#include "fast.h"
#include "conv.h"

using namespace std;

namespace wreport {

#if 0
Var::Var(Varcode code)
	: m_info(Vartable::query_local(code)), m_value(NULL), m_attrs(NULL)
{
}

Var::Var(Varcode code, int val)
	: m_info(Vartable::query_local(code)), m_value(NULL), m_attrs(NULL)
{
	seti(val);
}

Var::Var(Varcode code, double val)
	: m_info(Vartable::query_local(code)), m_value(NULL), m_attrs(NULL)
{
	setd(val);
}

Var::Var(Varcode code, const char* val)
	: m_info(Vartable::query_local(code)), m_value(NULL), m_attrs(NULL)
{
	setc(val);
}
#endif

Var::Var(Varinfo info)
	: m_info(info), m_value(NULL), m_attrs(NULL)
{
}

Var::Var(Varinfo info, int val)
	: m_info(info), m_value(NULL), m_attrs(NULL)
{
	seti(val);
}

Var::Var(Varinfo info, double val)
	: m_info(info), m_value(NULL), m_attrs(NULL)
{
	setd(val);
}

Var::Var(Varinfo info, const char* val)
	: m_info(info), m_value(NULL), m_attrs(NULL)
{
	setc(val);
}

Var::Var(const Var& var)
	: m_info(var.m_info), m_value(NULL), m_attrs(NULL)
{
	/* Copy the value */
	if (var.m_value != NULL)
		setc(var.m_value);

	/* Copy the attributes */
	if (var.m_attrs)
		m_attrs = new Var(*var.m_attrs);
}

Var::Var(Varinfo info, const Var& var)
	: m_info(info), m_value(NULL), m_attrs(NULL)
{
	copy_val(var);
}


Var& Var::operator=(const Var& var)
{
	// Deal with a = a;
	if (&var == this) return *this;

	// Copy info
	m_info = var.m_info;

	// Copy value
	if (m_value != NULL)
	{
		delete[] m_value;
		m_value = NULL;
	}
	if (var.m_value != NULL)
		setc(var.m_value);

	// Copy attributes
	copy_attrs(var);
}


Var::~Var()
{
	if (m_value != NULL)
		delete[] m_value;
	if (m_attrs)
		delete m_attrs;
}

bool Var::operator==(const Var& var) const
{
	// FIXME: fails if the code is the same but one has alterations
	if (code() != var.code()) return false;
	if (m_value == NULL && var.m_value == NULL) return true;
	if (m_value == NULL || var.m_value == NULL) return false;

	// Compare value
	if (m_info->is_string() || m_info->scale == 0)
	{
		if (strcmp(m_value, var.m_value) != 0)
			return false;
	}
	else
	{
		if (enqd() != var.enqd())
			return false;
	}

	// Compare attrs
	if (m_attrs == NULL && var.m_attrs == NULL) return true;
	if (m_attrs == NULL || var.m_attrs == NULL) return false;
	return *m_attrs == *var.m_attrs;
}

Varcode Var::code() const throw ()
{
	return m_info->var;
}

Varinfo Var::info() const throw ()
{
	return m_info;
}

const char* Var::value() const throw ()
{
	return m_value;
}

void Var::clear_attrs()
{
	if (m_attrs != NULL)
		delete m_attrs;
	m_attrs = 0;
}

static inline void fail_if_undef(const Varinfo& info, const char* val, const char* err)
{
	if (val == NULL)
		error_notfound::throwf("%s: B%02d%03d (%s) is not defined",
					err, WR_VAR_X(info->var), WR_VAR_Y(info->var), info->desc);
}

// Ensure that we're working with a numeric value
static inline void fail_if_string(const Varinfo& info, const char* err)
{
	if (info->is_string())
		error_type::throwf("%s: B%02d%03d (%s) is of type string",
				err, WR_VAR_X(info->var), WR_VAR_Y(info->var), info->desc);
}

int Var::enqi() const
{
	fail_if_undef(m_info, m_value, "enqi");
	fail_if_string(m_info, "enqi");
	return strtol(m_value, 0, 10);
}

double Var::enqd() const
{
	fail_if_undef(m_info, m_value, "enqd");
	fail_if_string(m_info, "enqd");
	return m_info->decode_int(strtol(m_value, 0, 10));
}

const char* Var::enqc() const
{
	fail_if_undef(m_info, m_value, "enqc");
	return m_value;
}

void Var::seti(int val)
{
	fail_if_string(m_info, "seti");

	/* Guard against overflows */
	if (val < m_info->imin || val > m_info->imax)
	{
		unset();
		error_domain::throwf("Value %i is outside the range [%i,%i] for B%02d%03d (%s)",
				val, m_info->imin, m_info->imax,
				WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var), m_info->desc);
	}
	
	/* Set the value */
	if (m_value == NULL &&
		(m_value = new char[m_info->len + 2]) == NULL)
		throw error_alloc("allocating space for Var value");

	// FIXME: not thread safe
	/* We add 1 to the length to cope with the '-' sign */
	strcpy(m_value, itoa(val, m_info->len + 1));
	/*snprintf(var->value, var->info->len + 2, "%d", val);*/
}

void Var::setd(double val)
{
	fail_if_string(m_info, "setd");

	/* Guard against NaNs */
	if (isnan(val))
	{
		unset();
		error_domain::throwf("Value %g is outside the range [%g,%g] for B%02d%03d (%s)",
				val, m_info->dmin, m_info->dmax,
				WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var), m_info->desc);
	}
	
	/* Guard against overflows */
	if (val < m_info->dmin || val > m_info->dmax)
	{
		unset();
		error_domain::throwf("Value %g is outside the range [%g,%g] for B%02d%03d (%s)",
				val, m_info->dmin, m_info->dmax,
				WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var), m_info->desc);
	}

	/* Set the value */
	if (m_value == NULL && 
		(m_value = new char[m_info->len + 2]) == NULL)
		throw error_alloc("allocating space for Var value");

	// FIXME: not thread safe
	strcpy(m_value, itoa(m_info->encode_int(val), m_info->len + 1));
	/*snprintf(var->value, var->info->len + 2, "%ld", (long)dba_var_encode_int(val, var->info));*/
}

void Var::setc(const char* val)
{
	/* Set the value */
	if (m_value == NULL &&
		(m_value = new char[m_info->len + 2]) == NULL)
		throw error_alloc("allocating space for Var value");

	/* Guard against overflows */
	int len = strlen(val);
	/* Tweak the length to account for the extra leading '-' allowed for
	 * negative numeric values */
	if (!m_info->is_string() && val[0] == '-')
		--len;
	if (len > m_info->len)
	{
		unset();
		error_domain::throwf("Value \"%s\" is too long for B%02d%03d (%s): maximum length is %d",
				val, WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var), m_info->desc, m_info->len);
	}

	strncpy(m_value, val, m_info->len + 1);
	m_value[m_info->len + 1] = 0;
}

void Var::unset()
{
	if (m_value != NULL) free(m_value);
	m_value = NULL;
}

const Var* Var::enqa(Varcode code) const
{
	for (const Var* cur = m_attrs; cur != NULL && cur->code() <= code; cur = cur->m_attrs)
		if (cur->code() == code)
			return cur;
	return NULL;
}

void Var::seta(const Var& attr)
{
	seta(auto_ptr<Var>(new Var(attr)));
}

void Var::seta(auto_ptr<Var> attr)
{
	// Ensure that the attribute does not have attributes of its own
	attr->clear_attrs();

	if (m_attrs == NULL || m_attrs->code() > attr->code())
	{
		// Append / insert
		attr->m_attrs = m_attrs;
		m_attrs = attr.release();
	}
	else if (m_attrs->code() == attr->code())
	{
		// Replace existing
		attr->m_attrs = m_attrs->m_attrs;
		Var* old_attrs = m_attrs;
		m_attrs = attr.release();
		old_attrs->m_attrs = NULL;
		delete old_attrs;
	}
	else
		// Recursively proceed along the chain
		m_attrs->seta(attr);
}

void Var::unseta(Varcode code)
{
	if (m_attrs == NULL || m_attrs->code() > code)
		// Past the end, nothing to do
		return;
	else if (m_attrs->code() == code)
	{
		// Got the right item, unlink and delete it
		Var* old_attrs = m_attrs;
		m_attrs = m_attrs->m_attrs;
		old_attrs->m_attrs = NULL;
		delete old_attrs;
	}
	else
		// Recursively proceed along the chain
		m_attrs->unseta(code);
}

const Var* Var::next_attr() const
{
	return m_attrs;
}

void Var::copy_val(const Var& src)
{
	if (src.value() == NULL)
	{
		unset();
	} else {
		if (m_info->is_string())
		{
			setc(src.value());
		} else {
			/* Convert and set the new value */
			setd(convert_units(src.info()->unit, m_info->unit, src.enqd()));
		}
	}

	/* Copy the attributes */
	copy_attrs(src);
}

void Var::copy_attrs(const Var& src)
{
	clear_attrs();
	if (src.m_attrs)
		m_attrs = new Var(*src.m_attrs);
}

void Var::print(FILE* out) const
{
	// Print info
	fprintf(out, "%d%02d%03d %-.64s(%s): ",
			WR_VAR_F(m_info->var), WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var),
			m_info->desc, m_info->unit);

	// Print value
	if (m_value == NULL)
		fprintf(out, "(undef)\n");
	else if (m_info->is_string() || m_info->scale == 0)
		fprintf(out, "%s\n", m_value);
	else
		fprintf(out, "%.*f\n", m_info->scale > 0 ? m_info->scale : 0, enqd());

	// Print attrs
	for (const Var* a = next_attr(); a; a = a->next_attr())
	{
		fputs("           ", out);
		a->print(out);
	}
}

unsigned Var::diff(const Var& var, FILE* out) const
{
	if (code() != var.code())
	{
		fprintf(out, "varcodes differ: first is %d%02d%03d'%s', second is %d%02d%03d'%s'\n",
			WR_VAR_F(m_info->var), WR_VAR_X(m_info->var), WR_VAR_Y(m_info->var), m_info->desc,
			WR_VAR_F(var.info()->var), WR_VAR_X(var.info()->var), WR_VAR_Y(var.info()->var), var.info()->desc);
		return 1;
	}
	if (m_value == NULL && var.value() == NULL)
		return 0;
	if (m_value == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] first value is NULL, second value is %s\n",
			WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()), m_info->desc,
			var.m_value);
		return 1;
	}
	if (var.m_value == NULL)
	{
		fprintf(out, "[%d%02d%03d %s] first value is %s, second value is NULL\n",
			WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()), m_info->desc,
			m_value);
		return 1;
	}
	if (m_info->is_string() || m_info->scale == 0)
	{
		if (strcmp(m_value, var.m_value) != 0)
		{
			fprintf(out, "[%d%02d%03d %s] values differ: first is \"%s\", second is \"%s\"\n",
				WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()), m_info->desc,
				m_value, var.m_value);
			return 1;
		}
	}
	else
	{
		double val1 = enqd(), val2 = var.enqd();
		if (val1 != val2)
		{
			fprintf(out, "[%d%02d%03d %s] values differ: first is %f, second is %f\n",
				WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()), m_info->desc,
				val1, val2);
			return 1;
		}
	}

	if ((m_attrs != 0) != (var.m_attrs != 0))
	{
		if (m_attrs)
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first has attributes, second does not\n",
				WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()),
				m_info->desc);
			return 1;
		}
		else
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first does not have attributes, second does\n",
				WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()),
				m_info->desc);
			return 1;
		}
	} else {
		int count1 = 0, count2 = 0;

		for (const Var* a = next_attr(); a; a = a->next_attr()) ++count1;
		for (const Var* a = var.next_attr(); a; a = a->next_attr()) ++count2;

		if (count1 != count2)
		{
			fprintf(out, "[%d%02d%03d %s] attributes differ: first has %d, second has %d\n",
				WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()),
				m_info->desc, count1, count2);
			return abs(count1 - count2);
		} else {
			/* Check attributes */
			const Var* a1 = next_attr();
			const Var* a2 = var.next_attr();
			for ( ; a1 && a2; a1 = a1->next_attr(), a2 = a2->next_attr())
			{
				Varcode extracode = 0;
				if (a1->code() < a2->code())
					extracode = a1->code();
				else if (a2->code() < a1->code())
					extracode = a2->code();
				if (extracode)
				{
					fprintf(out, "[%d%02d%03d %s] attributes differ: attribute %d%02d%03d exists only on first\n",
						WR_VAR_F(code()), WR_VAR_X(code()), WR_VAR_Y(code()),
						m_info->desc, 
						WR_VAR_F(extracode), WR_VAR_X(extracode), WR_VAR_Y(extracode));
					return 1;
				}

				unsigned diff = a1->diff(*a2, out);
				if (diff)
				{
					fprintf(out, "  comparing attr of variable ");
					print(out);
					return diff;
				}
			}
		}
	}
	return 0;
}

}

/* vim:set ts=4 sw=4: */
