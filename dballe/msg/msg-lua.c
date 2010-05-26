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
#include "msg.h"
#include "context.h"

#ifdef HAVE_LUA
#include <lauxlib.h>
#include <lualib.h>

static void dbalua_checked(lua_State* L, dba_err err)
{
	if (err != DBA_OK)
	{
		lua_pushstring(L, dba_error_get_message());
		lua_error(L);
	}
}

static int dbalua_msg_type(lua_State *L)
{
	dba_msg msg = dba_msg_lua_check(L, 1);
	lua_pushstring(L, dba_msg_type_name(msg->type));
	return 1;
}

static int dbalua_msg_size(lua_State *L)
{
	dba_msg msg = dba_msg_lua_check(L, 1);
	lua_pushinteger(L, msg->data_count);
	return 1;
}

static int dbalua_msg_foreach(lua_State *L)
{
	dba_msg msg = dba_msg_lua_check(L, 1);
	int i;
	for (i = 0; i < msg->data_count; ++i)
	{
		lua_pushvalue(L, -1);
		dba_msg_context_lua_push(msg->data[i], L);
		/* do the call (1 argument, 0 results) */
		if (lua_pcall(L, 1, 0, 0) != 0)
			lua_error(L);
	}
	return 0;
}

// TODO: dba_var dba_msg_find(dba_msg msg, dba_varcode code, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2);
// TODO: dba_var dba_msg_find_by_id(dba_msg msg, int id);

static int dbalua_msg_tostring(lua_State *L)
{
	dba_msg msg = dba_msg_lua_check(L, 1);
	int i, varcount = 0;
	for (i = 0; i < msg->data_count; ++i)
		varcount += msg->data[i]->data_count;
	lua_pushfstring(L, "dba_msg(%s, %d ctx, %d vars)", 
		dba_msg_type_name(msg->type), msg->data_count, varcount);
	return 1;
}

static const struct luaL_reg dbalua_msg_lib [] = {
        { "type", dbalua_msg_type },
        { "size", dbalua_msg_size },
        { "foreach", dbalua_msg_foreach },
        { "__tostring", dbalua_msg_tostring },
        {NULL, NULL}
};

dba_err dba_msg_lua_push(dba_msg var, lua_State* L)
{
        // The 'grib' object is a userdata that holds a pointer to this Grib structure
        dba_msg* s = (dba_msg*)lua_newuserdata(L, sizeof(dba_msg));
        *s = var;

        // Set the metatable for the userdata
        if (luaL_newmetatable(L, "dballe.msg"));
        {
                // If the metatable wasn't previously created, create it now
                lua_pushstring(L, "__index");
                lua_pushvalue(L, -2);  /* pushes the metatable */
                lua_settable(L, -3);  /* metatable.__index = metatable */

                // Load normal methods
                luaL_register(L, NULL, dbalua_msg_lib);
        }

        lua_setmetatable(L, -2);

	return dba_error_ok();
}

dba_msg dba_msg_lua_check(lua_State* L, int idx)
{
        dba_msg* v = (dba_msg*)luaL_checkudata(L, idx, "dballe.msg");
	return (v != NULL) ? *v : NULL;
}


static int dbalua_msg_context_size(lua_State *L)
{
	dba_msg_context ctx = dba_msg_context_lua_check(L, 1);
	lua_pushinteger(L, ctx->data_count);
	return 1;
}

static int dbalua_msg_context_foreach(lua_State *L)
{
	dba_msg_context ctx = dba_msg_context_lua_check(L, 1);
	int i;
	for (i = 0; i < ctx->data_count; ++i)
	{
		lua_pushvalue(L, -1);
		dba_var_lua_push(ctx->data[i], L);
		/* do the call (1 argument, 0 results) */
		if (lua_pcall(L, 1, 0, 0) != 0)
			lua_error(L);
	}
	return 0;
}

static int dbalua_msg_context_tostring(lua_State *L)
{
	dba_msg_context ctx = dba_msg_context_lua_check(L, 1);
	lua_pushfstring(L, "dba_msg_context(%d vars)", ctx->data_count);
	return 1;
}

static const struct luaL_reg dbalua_msg_context_lib [] = {
        { "size", dbalua_msg_context_size },
        { "foreach", dbalua_msg_context_foreach },
        { "__tostring", dbalua_msg_context_tostring },
        {NULL, NULL}
};

dba_err dba_msg_context_lua_push(dba_msg_context var, lua_State* L)
{
        // The object is a userdata that holds a pointer to this dba_msg_context structure
        dba_msg_context* s = (dba_msg_context*)lua_newuserdata(L, sizeof(dba_msg_context));
        *s = var;

        // Set the metatable for the userdata
        if (luaL_newmetatable(L, "dballe.msg.context"));
        {
                // If the metatable wasn't previously created, create it now
                lua_pushstring(L, "__index");
                lua_pushvalue(L, -2);  /* pushes the metatable */
                lua_settable(L, -3);  /* metatable.__index = metatable */

                // Load normal methods
                luaL_register(L, NULL, dbalua_msg_context_lib);
        }

        lua_setmetatable(L, -2);

	return dba_error_ok();
}

dba_msg_context dba_msg_context_lua_check(lua_State* L, int idx)
{
        dba_msg_context* v = (dba_msg_context*)luaL_checkudata(L, idx, "dballe.msg.context");
	return (v != NULL) ? *v : NULL;
}

#else
dba_err dba_msg_lua_push(dba_msg var, lua_State* L)
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
dba_msg dba_msg_lua_check(lua_State* L, int idx)
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
dba_err dba_msg_context_lua_push(dba_msg_context var, lua_State* L)
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
dba_msg_context dba_msg_context_lua_check(lua_State* L, int idx)
{
	return dba_error_unimplemented("DB-All.e compiled without Lua support");
}
#endif
