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
#ifndef WREPORT_TEST_UTILS_LUA_H
#define WREPORT_TEST_UTILS_LUA_H

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>
#include <iosfwd>

namespace wreport {
namespace tests {

struct Lua
{
	lua_State *L;
	std::string m_filename;

	Lua(const std::string& src = std::string());
	~Lua();

	/// Load the test code from the given file
	void loadFile(const std::string& fname);

	/// Load the test code from the given string containing Lua source code
	void loadString(const std::string& buf);

	/// Runs the parsed code to let it define the 'test' function we are going
	/// to use
	void create_lua_object();
		
	/// Send Lua's print output to an ostream
	void captureOutput(std::ostream& buf);

	/// Run the 'test' function and return its result, as a string
	std::string run();
};

}
}

#endif
