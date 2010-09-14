/*
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

#include <test-utils-wreport.h>
#include <test-utils-lua.h>
#include <wreport/var.h>

using namespace wreport;

namespace tut {

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
	const Vartable* table = Vartable::get("B0000000000000014000");
	Var var(table->query(WR_VAR(0, 12, 101)), 12.3);

	tests::Lua test(
		"function test() \n"
		"  if var:code() ~= 'B12101' then return 'code is '..var:code()..' instead of B12101' end \n"
		"  if var:enqi() ~= 1230 then return 'enqi is '..var:enqi()..' instead of 1230' end \n"
		"  if var:enqd() ~= 12.3 then return 'enqd is '..var:enqd()..' instead of 12.3' end \n"
		"  if var:enqc() ~= '1230' then return 'enqc is '..var:enqc()..' instead of 1230' end \n"
		"end \n"
	);

	// Push the variable as a global
	var.lua_push(test.L);
	lua_setglobal(test.L, "var");

	// Check that we can retrieve it
	lua_getglobal(test.L, "var");
	Var* pvar = Var::lua_check(test.L, 1);
	lua_pop(test.L, 1);
	ensure(&var == pvar);
	
	ensure_equals(test.run(), "");
}	

}

/* vim:set ts=4 sw=4: */
