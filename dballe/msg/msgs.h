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

#ifndef DBA_MSG_MSGS_H
#define DBA_MSG_MSGS_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup msg
 * Dynamic array of dba_msg
 *
 * 
 */

#include <dballe/msg/dba_msg.h>

/** Dynamic array of dba_msg */
struct _dba_msgs
{
	dba_msg* msgs;
	int len;
	int alloclen;
};
typedef struct _dba_msgs* dba_msgs;

/**
 * Create a new message array.
 *
 * @retval msgs
 *   The newly created dba_msgs.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_msgs_create(dba_msgs *msgs);

/**
 * Delete a message array, and all messages it contains.
 *
 * @param msgs
 *   The message array to delete.
 */
void dba_msgs_delete(dba_msgs msgs);

/**
 * Append a message to the array, taking over its memory management.
 *
 * @param msgs
 *   The message array to which the message is to be appended.
 * @param msg
 *   The message to append.  The dba_msgs array will take over memory
 *   management for it.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_msgs_append_acquire(dba_msgs msgs, dba_msg msg);

/**
 * Dump a dba_msgs to the given FILE*
 */
void dba_msgs_print(dba_msgs msgs, FILE* out);

/**
 * Compute and show the differences between two dba_msgs
 */
void dba_msgs_diff(dba_msgs msgs1, dba_msgs msgs2, int* diffs, FILE* out);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
