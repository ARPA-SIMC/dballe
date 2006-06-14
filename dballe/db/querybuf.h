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

#ifndef DBA_DB_QUERYBUF_H
#define DBA_DB_QUERYBUF_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup db
 * Implementation of an efficient string buffer for composing database queries
 */

#include <dballe/err/dba_error.h>

struct _dba_querybuf;

/**
 * Efficient string buffer for composing database queries
 */
typedef struct _dba_querybuf* dba_querybuf;
	
/**
 * Create a query buffer
 *
 * @param maxsize
 *   The maximum size of the query string.  Since dba_querybuf does not do
 *   dynamic resize of the buffer, it needs the maximum size specified upfront
 * @retval buf
 *   The query buffer
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_querybuf_create(int maxsize, dba_querybuf* buf);

/**
 * Delete a dba_querybuf
 *
 * @param buf
 *   The querybuf to delete
 */
void dba_querybuf_delete(dba_querybuf buf);

/**
 * Reset the querybuf to contain the empty string
 *
 * @param buf
 *   The buffer to operate on
 */
void dba_querybuf_reset(dba_querybuf buf);

/**
 * Get the string created so far
 *
 * @param buf
 *   The buffer to operate on
 * @return
 *   A pointer to the string created so far with the querybuf
 */
const char* dba_querybuf_get(dba_querybuf buf);

/**
 * Get the size of the string created so far
 *
 * @param buf
 *   The buffer to operate on
 * @return
 *   The length of the string created so far with the querybuf, not including
 *   the trailing null character
 */
int dba_querybuf_size(dba_querybuf buf);

/**
 * Append a string to the querybuf
 *
 * @param buf
 *   The buffer to operate on
 * @param str
 *   The string to append
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_querybuf_append(dba_querybuf buf, const char* str);

/**
 * Append a formatted string to the querybuf
 *
 * @param buf
 *   The buffer to operate on
 * @param fmt
 *   The string to append, which will be formatted in printf style
 * @return
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_querybuf_appendf(dba_querybuf buf, const char* fmt, ...);

#ifdef  __cplusplus
}
#endif

#endif
