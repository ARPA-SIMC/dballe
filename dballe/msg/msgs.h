/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
 */

#include <dballe/msg/msg.h>

/** Dynamic array of dba_msg */
struct _dba_msgs
{
	/**
	 * Array of dba_msg, reallocated when needed, doubling the allocated space
	 * every time
	 */
	dba_msg* msgs;
	/** Number of elements of msgs that are in use */
	int len;
	/** Number of elements of msgs that are allocated */
	int alloclen;
};
/** @copydoc _dba_msgs */
typedef struct _dba_msgs* dba_msgs;

/**
 * Create a new message array.
 *
 * @retval msgs
 *   The newly created dba_msgs.
 * @returns
 *   The error indicator for the function (See @ref error.h)
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
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msgs_append_acquire(dba_msgs msgs, dba_msg msg);

/**
 * Dump all the contents of the message to the given stream
 *
 * @param msgs
 *   The dba_msgs to dump
 * @param out
 *   The stream to dump the contents of the dba_msg to.
 */
void dba_msgs_print(dba_msgs msgs, FILE* out);

/**
 * Print the differences between two dba_msgs to a stream
 *
 * @param msgs1
 *   First dba_msgs to compare
 * @param msgs2
 *   Second dba_msgs to compare
 * @retval diffs
 *   Integer variable that will be incremented by the number of differences
 *   found.
 * @param out
 *   The stream to dump a description of the differences to.
 */
void dba_msgs_diff(dba_msgs msgs1, dba_msgs msgs2, int* diffs, FILE* out);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
