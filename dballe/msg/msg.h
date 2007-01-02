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

#ifndef DBA_MSG_H
#define DBA_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup msg
 * Encoding-independent in-memory representation for any kind of weather
 * report.
 */

#include <dballe/msg/level.h>
#include <stdio.h>

/**
 * Supported weather data types.
 */
typedef enum { MSG_GENERIC, MSG_SYNOP, MSG_PILOT, MSG_TEMP, MSG_TEMP_SHIP, MSG_AIREP, MSG_AMDAR, MSG_ACARS, MSG_SHIP, MSG_BUOY, MSG_METAR, MSG_SAT } dba_msg_type;

/** */
struct _dba_msg
{
	dba_msg_type type;

	int data_count;
	int data_alloc;
	dba_msg_level* data;
};
typedef struct _dba_msg* dba_msg;

/** */
const char* dba_msg_type_name(dba_msg_type type);


/** */
dba_err dba_msg_create(dba_msg* msg);

/** */
void dba_msg_print(dba_msg msg, FILE* out);

/** */
void dba_msg_diff(dba_msg msg1, dba_msg msg2, int* diffs, FILE* out);

/** */
void dba_msg_delete(dba_msg buoy);


/** */
dba_err dba_msg_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);

/** */
dba_err dba_msg_set_nocopy(dba_msg msg, dba_var var, int ltype, int l1, int l2, int pind, int p1, int p2);

/** */
dba_err dba_msg_set_by_id(dba_msg msg, dba_var var, int id);

/** */
dba_err dba_msg_set_nocopy_by_id(dba_msg msg, dba_var var, int id);

/** */
dba_err dba_msg_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);

/** */
dba_err dba_msg_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);

/** */
dba_err dba_msg_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);


/** */
dba_msg_level dba_msg_find_level(dba_msg msg, int ltype, int l1, int l2);

/** */
dba_msg_datum dba_msg_find(dba_msg msg, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);

/** */
dba_msg_datum dba_msg_find_by_id(dba_msg msg, int id);


/** */
dba_msg_type dba_msg_get_type(dba_msg msg);


/** */
dba_msg_type dba_msg_type_from_repcod(int repcod);

/** */
int dba_msg_repcod_from_type(dba_msg_type type);


/** */
dba_err dba_msg_sounding_pack_levels(dba_msg msg, dba_msg* dst);

/** */
dba_err dba_msg_sounding_unpack_levels(dba_msg msg, dba_msg* dst);

#include <dballe/msg/vars.h>

#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
