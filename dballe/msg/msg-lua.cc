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

#include "config.h"
#include "msg.h"
#include "context.h"
#include "dballe/msg/vars.h"
#include "wreport/utils/lua.h"

using namespace wreport;
using namespace std;

namespace dballe {

#ifdef HAVE_LUA

static Varcode dbalua_to_varcode(lua_State* L, int idx)
{
	const char* str = lua_tostring(L, idx);
	if (str == NULL)
		return 0;
	else
		return descriptor_code(str);
}

static int dbalua_msg_type(lua_State *L)
{
	Msg* msg = Msg::lua_check(L, 1);
	lua_pushstring(L, msg_type_name(msg->type));
	return 1;
}

static int dbalua_msg_size(lua_State *L)
{
	Msg* msg = Msg::lua_check(L, 1);
	lua_pushinteger(L, msg->data.size());
	return 1;
}

static int dbalua_msg_foreach(lua_State *L)
{
	Msg* msg = Msg::lua_check(L, 1);
	for (size_t i = 0; i < msg->data.size(); ++i)
	{
		lua_pushvalue(L, -1);
		msg->data[i]->lua_push(L);
		/* Do the call (1 argument, 0 results) */
		if (lua_pcall(L, 1, 0, 0) != 0)
			lua_error(L);
	}
	return 0;
}

static int dbalua_msg_find(lua_State *L)
{
	Msg* msg = Msg::lua_check(L, 1);
	Var* res = NULL;
	if (lua_gettop(L) == 2)
	{
		// By ID
		size_t len;
		const char* name = lua_tolstring(L, 2, &len);
		int id = resolve_var_substring(name, len);
		res = msg->edit_by_id(id);
	} else {
		// By all details
		Varcode code = dbalua_to_varcode(L, 2);
		Level level;
		level.ltype1 = lua_isnil(L, 3) ? MISSING_INT : lua_tointeger(L, 3);
		level.l1 = lua_isnil(L, 4) ? MISSING_INT : lua_tointeger(L, 4);
		level.ltype2 = lua_isnil(L, 5) ? MISSING_INT : lua_tointeger(L, 5);
		level.l2 = lua_isnil(L, 6) ? MISSING_INT : lua_tointeger(L, 6);
        Trange trange;
		trange.pind = lua_isnil(L, 7) ? MISSING_INT : lua_tointeger(L, 7);
		trange.p1 = lua_isnil(L, 8) ? MISSING_INT : lua_tointeger(L, 8);
		trange.p2 = lua_isnil(L, 9) ? MISSING_INT : lua_tointeger(L, 9);
		res = msg->edit(code, level, trange);
	}
	if (res == NULL)
		lua_pushnil(L);
	else
		res->lua_push(L);
	return 1;
}

static int dbalua_msg_tostring(lua_State *L)
{
	Msg* msg = Msg::lua_check(L, 1);
	int varcount = 0;
	for (size_t i = 0; i < msg->data.size(); ++i)
		varcount += msg->data[i]->data.size();
	lua_pushfstring(L, "dba_msg(%s, %d ctx, %d vars)", 
		msg_type_name(msg->type), msg->data.size(), varcount);
	return 1;
}

static const struct luaL_Reg dbalua_msg_lib [] = {
        { "type", dbalua_msg_type },
        { "size", dbalua_msg_size },
        { "foreach", dbalua_msg_foreach },
        { "find", dbalua_msg_find },
        { "__tostring", dbalua_msg_tostring },
        {NULL, NULL}
};

void Msg::lua_push(struct lua_State* L)
{
    lua::push_object(L, this, "dballe.msg", dbalua_msg_lib);
}

Msg* Msg::lua_check(struct lua_State* L, int idx)
{
    Msg** v = (Msg**)luaL_checkudata(L, idx, "dballe.msg");
    return (v != NULL) ? *v : NULL;
}


static int dbalua_msg_context_size(lua_State *L)
{
	msg::Context* ctx = msg::Context::lua_check(L, 1);
	lua_pushinteger(L, ctx->data.size());
	return 1;
}

static int dbalua_msg_context_foreach(lua_State *L)
{
	msg::Context* ctx = msg::Context::lua_check(L, 1);
	for (size_t i = 0; i < ctx->data.size(); ++i)
	{
		lua_pushvalue(L, -1);
		ctx->data[i]->lua_push(L);
		/* do the call (1 argument, 0 results) */
		if (lua_pcall(L, 1, 0, 0) != 0)
			lua_error(L);
	}
	return 0;
}

static int dbalua_msg_context_tostring(lua_State *L)
{
	msg::Context* ctx = msg::Context::lua_check(L, 1);
	lua_pushfstring(L, "dba_msg_context(%d vars)", ctx->data.size());
	return 1;
}

#define make_accessor(name, acc) static int dbalua_msg_context_##name(lua_State *L) { \
    msg::Context* ctx = msg::Context::lua_check(L, 1); \
	lua_pushinteger(L, ctx->acc); \
	return 1; \
}
make_accessor(ltype1, level.ltype1)
make_accessor(l1, level.l1)
make_accessor(ltype2, level.ltype2)
make_accessor(l2, level.l2)
make_accessor(pind, trange.pind)
make_accessor(p1, trange.p1)
make_accessor(p2, trange.p2)
#undef make_accessor


static const struct luaL_Reg dbalua_msg_context_lib [] = {
        { "ltype1", dbalua_msg_context_ltype1 },
        { "l1", dbalua_msg_context_l1 },
        { "ltype2", dbalua_msg_context_ltype2 },
        { "l2", dbalua_msg_context_l2 },
        { "pind", dbalua_msg_context_pind },
        { "p1", dbalua_msg_context_p1 },
        { "p2", dbalua_msg_context_p2 },
        { "size", dbalua_msg_context_size },
        { "foreach", dbalua_msg_context_foreach },
        { "__tostring", dbalua_msg_context_tostring },
        {NULL, NULL}
};

void msg::Context::lua_push(struct lua_State* L)
{
    lua::push_object(L, this, "dballe.msg.context", dbalua_msg_context_lib);
}

msg::Context* msg::Context::lua_check(struct lua_State* L, int idx)
{
    msg::Context** v = (msg::Context**)luaL_checkudata(L, idx, "dballe.msg.context");
	return (v != NULL) ? *v : NULL;
}

#else

void Msg::lua_push(struct lua_State* L)
{
	throw error_unimplemented("DB-All.e compiled without Lua support");
}
Msg* Msg::lua_check(struct lua_State* L, int idx)
{
	throw error_unimplemented("DB-All.e compiled without Lua support");
}
void msg::Context::lua_push(struct lua_State* L)
{
	throw error_unimplemented("DB-All.e compiled without Lua support");
}
msg::Context* msg::Context::lua_check(struct lua_State* L, int idx)
{
	throw error_unimplemented("DB-All.e compiled without Lua support");
}

#endif

} // namespace dballe
