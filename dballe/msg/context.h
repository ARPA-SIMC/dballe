/*
 * DB-ALLe - Archive for punctual meteorological data
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

#ifndef DBA_MSG_LEVEL_H
#define DBA_MSG_LEVEL_H

/** @file
 * @ingroup msg
 *
 * Sorted storage for all the dba_msg_datum present on one level.
 */

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/msg/msg.h>
#include <stdio.h>

/**
 * Store an array of physical data all on the same level
 */
struct _dba_msg_context
{
	/** Type of the first level.  See @ref level_table. */
	int ltype1;
	/** L1 value of the level.  See @ref level_table. */
	int l1;
	/** Type of the second level.  See @ref level_table. */
	int ltype2;
	/** L2 value of the level.  See @ref level_table. */
	int l2;
	/** Time range type indicator.  See @ref trange_table. */
	int pind;
	/** Time range P1 indicator.  See @ref trange_table. */
	int p1;
	/** Time range P2 indicator.  See @ref trange_table. */
	int p2;

	/** Number of items in this level */
	int data_count;

	/**
	 * Number of items allocated (must always be greater than or equal to
	 * data_count
	 */
	int data_alloc;

	/**
	 * The array with the data, reallocated as needed
	 */
	dba_var* data;
};

/**
 * Create a new dba_msg_context
 *
 * @retval l
 *   The newly created level.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_context_create(int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2, dba_msg_context* l);

/**
 * Copy an existing level
 *
 * @param src
 *   The level to copy.
 * @retval dst
 *   The newly created duplicate.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_context_copy(dba_msg_context src, dba_msg_context* dst);

/**
 * Delete a dba_msg_context
 *
 * @param l
 *   The level to delete.
 */
void dba_msg_context_delete(dba_msg_context l);


/**
 * Compare two dba_msg_context strutures, for use in sorting.
 *
 * @param l1
 *   First dba_msg_context to compare
 * @param l2
 *   Second dba_msg_context to compare
 * @return
 *   -1 if l1 < l2, 0 if l1 == l2, 1 if l1 > l2
 */
int dba_msg_context_compare(const dba_msg_context l1, const dba_msg_context l2);

/**
 * Compare a dba_msg_context struture with some level information, for use in
 * sorting.
 *
 * @param l
 *   First dba_msg_context to compare
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   -1 if l < ltype,l1,l2; 0 if l == ltype,l1,l2; 1 if l > ltype,l1,l2
 */
int dba_msg_context_compare2(const dba_msg_context l, int ltype1, int l1, int ltype2, int l2, int pind, int p1, int p2);


/**
 * Add a dba_var to the level, taking over its memory management.
 *
 * @param l
 *   The level to add the variable to.
 * @param var
 *   The variable to add.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_context_set_nocopy(dba_msg_context l, dba_var var);

#if 0
dba_err dba_msg_context_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_set_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_context_set_nocopy_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_context_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_context_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
#endif

/**
 * Find a datum given its description
 *
 * @param l
 *   The level to query
 * @param code
 *   The ::dba_varcode of the variable to query.  See @ref vartable.h
 * @return
 *   The variable found, or NULL if it was not found.
 */
dba_var dba_msg_context_find(dba_msg_context l, dba_varcode code);

/** 
 * Find a datum given its shortcut ID
 *
 * @param l
 *   The level to query
 * @param id
 *   Shortcut ID of the value to set (see @ref vars.h)
 * @return
 *   The variable found, or NULL if it was not found.
 */
dba_var dba_msg_context_find_by_id(dba_msg_context l, int id);

/**
 * If this context is the right context for a vertical sounding significance
 * and contains a vertical sounding significance variable, return it. Else,
 * return NULL.
 */
dba_var dba_msg_context_find_vsig(dba_msg_context l);

/**
 * Dump all the contents of the level to the given stream
 *
 * @param l
 *   The level to dump
 * @param out
 *   The stream to dump the contents of the level to.
 */
void dba_msg_context_print(dba_msg_context l, FILE* out);

/**
 * Print the differences between two dba_msg_context to a stream
 *
 * @param l1
 *   First level to compare
 * @param l2
 *   Second level to compare
 * @retval diffs
 *   Integer variable that will be incremented by the number of differences
 *   found.
 * @param out
 *   The stream to dump a description of the differences to.
 */
void dba_msg_context_diff(dba_msg_context l1, dba_msg_context l2, int* diffs, FILE* out);


struct lua_State;

/**
 * Push the variable as an object in the lua stack
 */
dba_err dba_msg_context_lua_push(dba_msg_context var, struct lua_State* L);

/**
 * Check that the element at \a idx is a dba_msg_context
 *
 * @return the dba_msg_context element, or NULL if the check failed
 */
dba_msg_context dba_msg_context_lua_check(struct lua_State* L, int idx);

#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
