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
