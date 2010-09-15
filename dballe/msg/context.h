/*
 * msg/context - Hold variables with the same physical context
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_MSG_CONTEXT_H
#define DBA_MSG_CONTEXT_H

/** @file
 * @ingroup msg
 *
 * Sorted storage for all the dba_msg_datum present on one level.
 */

#include <dballe/core/var.h>
#include <dballe/msg/defs.h>
#include <vector>
#include <memory>

struct lua_State;

namespace dballe {
namespace msg {

/**
 * Store an array of physical data all on the same level
 */
class Context
{
protected:
    /**
     * Return the index of the var `code' in l, or -1 if it was not found
     */
    int find_index(wreport::Varcode code) const;

public:
    Level level;
    Trange trange;

	/**
	 * The variables in this context
	 */
	std::vector<wreport::Var*> data;

	Context(const Level& lev, const Trange& tr);
    Context(const Context& c);
	~Context();

    Context& operator=(const Context& src);

    /**
     * Compare two dba_msg_context strutures, for use in sorting.
     *
     * @param ctx
     *   First context to compare
     * @return
     *   -1 if l1 < l2, 0 if l1 == l2, 1 if l1 > l2
     */
    int compare(const Context& ctx) const;

    /**
     * Compare a Context struture with level and time range information, for
     * use in sorting.
     *
     * @return
     *   -1 if l < ltype,l1,l2; 0 if l == ltype,l1,l2; 1 if l > ltype,l1,l2
     */
    int compare(const Level& lev, const Trange& tr) const;

    /**
     * Add a Var to the level
     *
     * If a variable exists with the same code, it is replaced
     *
     * @param var
     *   The variable to add or replace.
     */
    void set(const wreport::Var& var);

    /**
     * Add a Var to the level
     *
     * If a variable exists with the same code, it is replaced
     *
     * The Context will take ownership of memory management for \a var
     *
     * @param var
     *   The variable to add or replace.
     */
    void set(std::auto_ptr<wreport::Var> var);

    /**
     * Find a variable given its varcode
     *
     * @param code
     *   The wreport::Varcode of the variable to query.  See @ref vartable.h
     * @return
     *   The variable found, or NULL if it was not found.
     */
    const wreport::Var* find(wreport::Varcode code) const;

    /**
     * Find a variable given its varcode
     *
     * @param code
     *   The wreport::Varcode of the variable to query.  See @ref vartable.h
     * @return
     *   The variable found, or NULL if it was not found.
     */
    wreport::Var* edit(wreport::Varcode code);

    /** 
     * Find a variable given its shortcut ID
     *
     * @param id
     *   Shortcut ID of the value to set (see @ref vars.h)
     * @return
     *   The variable found, or NULL if it was not found.
     */
    const wreport::Var* find_by_id(int id) const;

    /**
     * If this context is the right context for a vertical sounding significance
     * and contains a vertical sounding significance variable, return it. Else,
     * return NULL.
     */
    const wreport::Var* find_vsig() const;

    /**
     * Dump all the contents of the context to the given stream
     *
     * @param out
     *   The stream to dump the contents of the level to.
     */
    void print(FILE* out) const;

    /**
     * Print the differences between two Context to a stream
     *
     * @param ctx
     *   Context to compare with this one
     * @param out
     *   The stream to dump a description of the differences to.
     * @return
     *   Number of differences found.
     */
    unsigned diff(const Context& ctx, FILE* out) const;

    /**
     * Push the variable as an object in the lua stack
     */
    void lua_push(struct lua_State* L);

    /**
     * Check that the element at \a idx is a dba_msg_context
     *
     * @return the dba_msg_context element, or NULL if the check failed
     */
    static Context* lua_check(struct lua_State* L, int idx);
};



#if 0
dba_err dba_msg_context_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_set_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_context_set_nocopy_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_context_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
#endif

#if 0



#endif

}
}

// vim:set ts=4 sw=4:
#endif
