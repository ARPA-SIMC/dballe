/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/msg/datum.h>
#include <stdio.h>

struct _dba_msg_level
{
	int ltype, l1, l2;

	int data_count;
	int data_alloc;
	dba_msg_datum* data;
};
typedef struct _dba_msg_level* dba_msg_level;

dba_err dba_msg_level_create(dba_msg_level* l, int ltype, int l1, int l2);
dba_err dba_msg_level_copy(dba_msg_level src, dba_msg_level* dst);
void dba_msg_level_delete(dba_msg_level l);

int dba_msg_level_compare(const dba_msg_level l1, const dba_msg_level l2);
int dba_msg_level_compare2(const dba_msg_level l, int ltype, int l1, int l2);

dba_err dba_msg_level_set_nocopy(dba_msg_level l, dba_var var, int pind, int p1, int p2);

#if 0
dba_err dba_msg_level_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_level_set_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_level_set_nocopy_by_id(dba_msg msg, dba_var var, int id);
dba_err dba_msg_level_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_level_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
dba_err dba_msg_level_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);
#endif

dba_msg_datum dba_msg_level_find(dba_msg_level l, dba_varcode code, int pind, int p1, int p2);
dba_msg_datum dba_msg_level_find_by_id(dba_msg_level l, int id);

void dba_msg_level_print(dba_msg_level l, FILE* out);
void dba_msg_level_diff(dba_msg_level l1, dba_msg_level l2, int* diffs, FILE* out);

#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
