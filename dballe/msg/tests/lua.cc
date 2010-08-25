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

#include <test-utils-msg.h>
#include <dballe/core/test-utils-lua.h>
#include <dballe/msg/msg.h>

namespace tut {
using namespace tut_dballe;

struct lua_shar
{
	TestMsgEnv testenv;

	lua_shar()
	{
	}

	~lua_shar()
	{
	}
};
TESTGRP(lua);


// Test message access
template<> template<>
void to::test<1>()
{
	// Get a test message
	dba_msgs msgs = read_test_msg("bufr/obs0-1.22.bufr", BUFR);
	gen_ensure_equals(msgs->len, 1u);
	dba_msg msg = msgs->msgs[0];

	Lua test(
		"function test() \n"
		"  if msg:type() ~= 'synop' then return 'type is '..msg:type()..' instead of synop' end \n"
		"  if msg:size() ~= 17 then return 'size is '..msg:size()..' instead of 17' end \n"

		"  count = 0\n"
		"  msg:foreach(function(x) count = count + 1 end)\n"
		"  if count ~= 17 then return 'count is '..count..' instead of 17' end\n"

		"  count = 0\n"
		"  msg:foreach(function(x) count = count + x:size() end)\n"
		"  if count ~= 43 then return 'count is '..count..' instead of 43' end\n"

		"  count = 0\n"
		"  msg:foreach(function(x) x:foreach(function(y) count = count + 1 end) end)\n"
		"  if count ~= 43 then return 'count is '..count..' instead of 43' end\n"

		"  context = nil\n"
		"  msg:foreach(function(x) context=x end)\n"
		"  if context.ltype1 == nil then return 'context.ltype1 is nil' end\n"
		"  if context.l1 == nil then return 'context.l1 is nil' end\n"
		"  if context.ltype2 == nil then return 'context.ltype2 is nil' end\n"
		"  if context.l2 == nil then return 'context.l2 is nil' end\n"
		"  if context.pind == nil then return 'context.pind is nil' end\n"
		"  if context.p1 == nil then return 'context.p1 is nil' end\n"
		"  if context.p2 == nil then return 'context.p2 is nil' end\n"

		"  var = msg:find('temp_2m')\n"
		"  if var == nil then return 'temp_2m is nil' end\n"
		"  if var:enqd() ~= 289.2 then return 'temp_2m is '..var:enqd()..' instead of 289.2' end\n"

		"  var = msg:find('B12101', 103, 2000, 0, 0, 254, 0, 0)\n"
		"  if var == nil then return 'B12001 is nil' end\n"
		"  if var:enqd() ~= 289.2 then return 'B12001 is '..var:enqd()..' instead of 289.2' end\n"
		"end \n"
	);

	// Push the variable as a global
	dba_msg_lua_push(msg, test.L);
	lua_setglobal(test.L, "msg");

	// Check that we can retrieve it
	lua_getglobal(test.L, "msg");
	dba_msg msg1 = dba_msg_lua_check(test.L, 1);
	lua_pop(test.L, 1);
	gen_ensure(msg == msg1);
	
	gen_ensure_equals(test.run(), "");
}	

}

/* vim:set ts=4 sw=4: */
