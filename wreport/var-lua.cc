/*
 * wreport/var-lua - Lua bindings to wreport variables
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

#include "config.h"
#include "var.h"

#ifdef HAVE_LUA
extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

namespace wreport {

Var* Var::lua_check(lua_State* L, int idx)
{
        Var** v = (Var**)luaL_checkudata(L, idx, "dballe.var");
	return (v != NULL) ? *v : NULL;
}

static int dbalua_var_enqi(lua_State *L)
{
	Var* var = Var::lua_check(L, 1);
	try {
		if (var->value() != NULL)
			lua_pushinteger(L, var->enqi());
		else
			lua_pushnil(L);
	} catch (std::exception& e) {
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 1;
}

static int dbalua_var_enqd(lua_State *L)
{
	Var* var = Var::lua_check(L, 1);
	try {
		if (var->value() != NULL)
			lua_pushnumber(L, var->enqd());
		else
			lua_pushnil(L);
	} catch (std::exception& e) {
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 1;
}

static int dbalua_var_enqc(lua_State *L)
{
	Var* var = Var::lua_check(L, 1);
	try {
		const char* res = var->value();
		if (res != NULL)
			lua_pushstring(L, res);
		else
			lua_pushnil(L);
	} catch (std::exception& e) {
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 1;
}

static int dbalua_var_code(lua_State *L)
{
	static char fcodes[] = "BRCD";
	Var* var = Var::lua_check(L, 1);
	char buf[10];
	snprintf(buf, 10, "%c%02d%03d", fcodes[WR_VAR_F(var->code())], WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
	lua_pushstring(L, buf);
	return 1;
}

static int dbalua_var_tostring(lua_State *L)
{
	Var* var = Var::lua_check(L, 1);
	try {
		const char* res = var->value();
		if (res == NULL)
			lua_pushstring(L, "(undef)");
		else {
			Varinfo info = var->info();
			if (info->is_string() || info->scale == 0)
				lua_pushstring(L, res);
			else
			{
				double val = var->enqd();
				char buf[25];
				snprintf(buf, 25, "%.*f", info->scale > 0 ? info->scale : 0, val);
				lua_pushstring(L, buf);
			}
		}
	} catch (std::exception& e) {
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 1;
}


static const struct luaL_reg dbalua_var_lib [] = {
	{ "code", dbalua_var_code },
        { "enqi", dbalua_var_enqi },
        { "enqd", dbalua_var_enqd },
        { "enqc", dbalua_var_enqc },
        { "__tostring", dbalua_var_tostring },
        {NULL, NULL}
};

void Var::lua_push(lua_State* L)
{
        // The 'Var' object is a userdata that holds a pointer to this Var structure
        Var** s = (Var**)lua_newuserdata(L, sizeof(Var*));
        *s = this;

        // Set the metatable for the userdata
        if (luaL_newmetatable(L, "dballe.var"))
        {
                // If the metatable wasn't previously created, create it now
                lua_pushstring(L, "__index");
                lua_pushvalue(L, -2);  /* pushes the metatable */
                lua_settable(L, -3);  /* metatable.__index = metatable */

                // Load normal methods
                luaL_register(L, NULL, dbalua_var_lib);
        }

        lua_setmetatable(L, -2);
}

#else
void Var::lua_push(lua_State* L)
{
	try error_unimplemented("DB-All.e compiled without Lua support");
}
Var* Var::lua_check(lua_State* L, int idx)
{
	try error_unimplemented("DB-All.e compiled without Lua support");
}
#endif

}
