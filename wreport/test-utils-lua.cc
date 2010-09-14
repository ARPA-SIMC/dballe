/**
 * Copyright (C) 2008--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-lua.h"
#include <stdexcept>
#include <ostream>

using namespace std;

namespace wreport {
namespace tests {

Lua::Lua(const std::string& src) : L(NULL)
{
	// Initialise the lua logic
	L = lua_open();

	// NOTE: This one is optional: only use it for debugging
	#if LUA_VERSION_NUM >= 501
	luaL_openlibs(L);
	#else
	luaopen_base(L);              /* opens the basic library */
	luaopen_table(L);             /* opens the table library */
	luaopen_io(L);                /* opens the I/O library */
	luaopen_string(L);            /* opens the string lib. */
	luaopen_math(L);              /* opens the math lib. */
	luaopen_loadlib(L);           /* loadlib function */
	luaopen_debug(L);             /* debug library  */
	lua_settop(L, 0);
	#endif

	if (!src.empty())
		loadString(src);
}

Lua::~Lua()
{
	if (L) lua_close(L);
}

/// Load the test code from the given file
void Lua::loadFile(const std::string& fname)
{
	/// Load the prettyprinting functions
	m_filename = fname;

	if (luaL_loadfile(L, fname.c_str()))
		throw std::runtime_error(lua_tostring(L, -1));

	create_lua_object();
}

/// Load the test code from the given string containing Lua source code
void Lua::loadString(const std::string& buf)
{
	m_filename = "memory buffer";

	if (luaL_loadbuffer(L, buf.data(), buf.size(), m_filename.c_str()))
		throw std::runtime_error(lua_tostring(L, -1));

	create_lua_object();
}

/// Runs the parsed code to let it define the 'test' function we are going
/// to use
void Lua::create_lua_object()
{
	if (lua_pcall(L, 0, 0, 0))
	{
		string error = lua_tostring(L, -1);
		lua_pop(L, 1);
		throw std::runtime_error(error);
	}

	// ensure that there is a 'test' function
	lua_getglobal(L, "test");
	int type = lua_type(L, -1);
	if (type == LUA_TNIL)
		throw std::runtime_error("code did not define a function called 'test'");
	if (type != LUA_TFUNCTION)
		throw std::runtime_error("the 'test' variable is not a function");

	// Pop the 'test' function from the stack
	lua_pop(L, 1);
}

// From src/lbaselib.c in lua 5.1
/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int lua_ostream_print(lua_State *L)
{
	// Access the closure parameters
	int outidx = lua_upvalueindex(1);
	void* ud = lua_touserdata(L, outidx);
	if (!ud)
		return luaL_error(L, "lua_report_print must be called as a closure with one userdata");
	std::ostream& out = **(std::ostream**)ud;

	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to "
					LUA_QL("print"));
		if (i>1) out << '\t';
		out << s;
		lua_pop(L, 1);  /* pop result */
	}
	out << endl;
	return 0;
}
		
void Lua::captureOutput(std::ostream& buf)
{
	// Create a C closure with the print function and the ostream to use
	std::ostream** s = (std::ostream**)lua_newuserdata(L, sizeof(std::ostream*));
	*s = &buf;
	lua_pushcclosure(L, lua_ostream_print, 1);

	// redefine print
	lua_setglobal(L, "print");
}

std::string Lua::run()
{
	// Call the function without arguments
	lua_getglobal(L, "test");

	if (lua_pcall(L, 0, 1, 0))
	{
		string error = lua_tostring(L, -1);
		lua_pop(L, 1);
		throw std::runtime_error(error);
	}
	const char* res = lua_tostring(L, -1);
	lua_pop(L, 1);
	return res == NULL ? string() : res;
}

}
}

// vim:set ts=4 sw=4:
