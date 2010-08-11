/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <test-utils-lua.h>
#include <dballe/core/var.h>

namespace tut {
using namespace tut_dballe;

struct lua_shar
{
	lua_shar()
	{
	}

	~lua_shar()
	{
	}
};
TESTGRP(lua);


// Test variable access
template<> template<>
void to::test<1>()
{
	dba_var var;
	CHECKED(dba_var_create_local(DBA_VAR(0, 12, 1), &var));
	CHECKED(dba_var_setd(var, 12.3));

	Lua test(
		"function test() \n"
		"  if var:code() ~= 'B12001' then return 'code is '..var:code()..' instead of B12001' end \n"
		"  if var:enqi() ~= 123 then return 'enqi is '..var:enqi()..' instead of 123' end \n"
		"  if var:enqd() ~= 12.3 then return 'enqd is '..var:enqd()..' instead of 12.3' end \n"
		"  if var:enqc() ~= '123' then return 'enqc is '..var:enqc()..' instead of 123' end \n"
		"end \n"
	);

	// Push the variable as a global
	dba_var_lua_push(var, test.L);
	lua_setglobal(test.L, "var");

	// Check that we can retrieve it
	lua_getglobal(test.L, "var");
	dba_var var1 = dba_var_lua_check(test.L, 1);
	lua_pop(test.L, 1);
	gen_ensure(var == var1);
	
	gen_ensure_equals(test.run(), "");
}	

}

/* vim:set ts=4 sw=4: */
