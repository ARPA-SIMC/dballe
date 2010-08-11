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

#include "config.h"
#include "var.h"

#ifdef HAVE_LUA
#include <lauxlib.h>
#include <lualib.h>

dba_var dba_var_lua_check(lua_State* L, int idx)
{
        dba_var* v = (dba_var*)luaL_checkudata(L, idx, "dballe.var");
	return (v != NULL) ? *v : NULL;
}

static void dbalua_checked(lua_State* L, dba_err err)
{
	if (err != DBA_OK)
	{
		lua_pushstring(L, dba_error_get_message());
		lua_error(L);
	}
}

static int dbalua_var_enqi(lua_State *L)
{
	dba_var var = dba_var_lua_check(L, 1);
	if (dba_var_value(var) != NULL)
	{
		int res;
		dbalua_checked(L, dba_var_enqi(var, &res));
		lua_pushinteger(L, res);
	} else
		lua_pushnil(L);
	return 1;
}

static int dbalua_var_enqd(lua_State *L)
{
	dba_var var = dba_var_lua_check(L, 1);
	if (dba_var_value(var) != NULL)
	{
		double res;
		dbalua_checked(L, dba_var_enqd(var, &res));
		lua_pushnumber(L, res);
	} else
		lua_pushnil(L);
	return 1;
}

static int dbalua_var_enqc(lua_State *L)
{
	dba_var var = dba_var_lua_check(L, 1);
	const char* res = dba_var_value(var);
	if (res != NULL)
		lua_pushstring(L, res);
	else
		lua_pushnil(L);
	return 1;
}

static int dbalua_var_code(lua_State *L)
{
	dba_var var = dba_var_lua_check(L, 1);
	char buf[10];
	snprintf(buf, 10, "B%02d%03d", DBA_VAR_X(dba_var_code(var)), DBA_VAR_Y(dba_var_code(var)));
	lua_pushstring(L, buf);
	return 1;
}

static int dbalua_var_tostring(lua_State *L)
{
	dba_var var = dba_var_lua_check(L, 1);
	const char* res = dba_var_value(var);
	if (res == NULL)
		lua_pushstring(L, "(undef)");
	else {
		dba_varinfo info = dba_var_info(var);
		if (info->is_string || info->scale == 0)
			lua_pushstring(L, res);
		else
		{
			double val;
			char buf[25];
			dbalua_checked(L, dba_var_enqd(var, &val));
			snprintf(buf, 25, "%.*f", info->scale > 0 ? info->scale : 0, val);
			lua_pushstring(L, buf);
		}
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

dba_err dba_var_lua_push(dba_var var, lua_State* L)
{
        // The 'grib' object is a userdata that holds a pointer to this Grib structure
        dba_var* s = (dba_var*)lua_newuserdata(L, sizeof(dba_var));
        *s = var;

        // Set the metatable for the userdata
        if (luaL_newmetatable(L, "dballe.var"));
        {
                // If the metatable wasn't previously created, create it now
                lua_pushstring(L, "__index");
                lua_pushvalue(L, -2);  /* pushes the metatable */
                lua_settable(L, -3);  /* metatable.__index = metatable */

                // Load normal methods
                luaL_register(L, NULL, dbalua_var_lib);
        }

        lua_setmetatable(L, -2);

	return dba_error_ok();
}

#else
dba_err dba_var_lua_push(dba_var var, lua_State* L)
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
dba_var dba_var_lua_check(lua_State* L, int idx) const
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
#endif
