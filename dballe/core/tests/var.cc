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

#include <test-utils-core.h>
#include <dballe/core/var.h>
#include <math.h>

namespace tut {
using namespace tut_dballe;

struct var_shar
{
	var_shar()
	{
	}

	~var_shar()
	{
	}
};
TESTGRP(var);


// Test variable creation
template<> template<>
void to::test<1>()
{
	dba_var var;
	dba_varinfo info;

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 6, 1), &info));

	CHECKED(dba_var_create(info, &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure_equals(dba_var_value(var), (const char*)0);
	dba_var_delete(var);

	CHECKED(dba_var_createi(info, 123, &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, 123);
	CHECKED(dba_var_seti(var, -123));
	gen_ensure_var_equals(var, -123);
	dba_var_delete(var);

	CHECKED(dba_var_created(info, 123.456, &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, 123.456);
	CHECKED(dba_var_setd(var, -123.456));
	gen_ensure_var_equals(var, -123.456);
	dba_var_delete(var);

	CHECKED(dba_var_createc(info, "123", &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_info(var), info);
	gen_ensure(dba_var_value(var) != 0);
	gen_ensure_var_equals(var, "123");
	dba_var_delete(var);

	CHECKED(dba_var_create_local(DBA_VAR(0, 6, 1), &var));
	gen_ensure(var != NULL);
	gen_ensure_equals(dba_var_code(var), DBA_VAR(0, 6, 1));
	gen_ensure_equals(dba_var_value(var), (const char*)0);
	dba_var_delete(var);
}	

// Get and set values
template<> template<>
void to::test<2>()
{

}

// Test variable copy
template<> template<>
void to::test<3>()
{
	dba_var var = NULL, var1 = NULL, attr = NULL;
	
	CHECKED(dba_var_create_local(DBA_VAR(0, 6, 1), &var));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(var, 234));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 7), &attr));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(attr, 75));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_create_local(DBA_VAR(0, 33, 15), &attr));
	gen_ensure(var != NULL);
	CHECKED(dba_var_seti(attr, 45));
	CHECKED(dba_var_seta_nocopy(var, attr));

	CHECKED(dba_var_enqa(var, DBA_VAR(0, 33, 7), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 75);

	CHECKED(dba_var_enqa(var, DBA_VAR(0, 33, 15), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 45);


	CHECKED(dba_var_copy(var, &var1));
	gen_ensure(var1 != NULL);
	gen_ensure_var_equals(var1, 234);

	// Also check dba_var_equals
	gen_ensure(dba_var_equals(var, var1));
	gen_ensure(dba_var_equals(var1, var));

	CHECKED(dba_var_enqa(var1, DBA_VAR(0, 33, 7), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 75);

	CHECKED(dba_var_enqa(var1, DBA_VAR(0, 33, 15), &attr));
	gen_ensure(attr != NULL);
	gen_ensure_var_equals(attr, 45);

	// Fiddle with the attribute and make sure dba_var_equals notices
	CHECKED(dba_var_seti(attr, 10));
	gen_ensure(!dba_var_equals(var, var1));
	gen_ensure(!dba_var_equals(var1, var));


	dba_var_delete(var);
	dba_var_delete(var1);
}

// Test missing checks
template<> template<>
void to::test<4>()
{
	dba_var var;
	dba_varinfo info;
	dba_err err;

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 12, 3), &info));
	CHECKED(dba_var_create(info, &var));

	err = dba_var_setd(var, logf(0));
	gen_ensure_equals(err, DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), 6);
	gen_ensure(dba_var_value(var) == NULL);

	err = dba_var_setd(var, logf(0)/logf(0));
	gen_ensure_equals(err, DBA_ERROR);
	gen_ensure_equals(dba_error_get_code(), 6);
	gen_ensure(dba_var_value(var) == NULL);

	dba_var_delete(var);
}

// Test ranges
template<> template<>
void to::test<5>()
{
	dba_var var;
	dba_varinfo info;
	dba_err err;

	CHECKED(dba_varinfo_query_local(DBA_VAR(0, 33, 198), &info));
	CHECKED(dba_var_create(info, &var));

	double val;
	CHECKED(dba_var_setd(var, 1));
	CHECKED(dba_var_enqd(var, &val));
	gen_ensure_equals(val, 1);

	CHECKED(dba_var_setd(var, -1));
	CHECKED(dba_var_enqd(var, &val));
	gen_ensure_equals(val, -1);

	dba_var_delete(var);
}

}

/* vim:set ts=4 sw=4: */
