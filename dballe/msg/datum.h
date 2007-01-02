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

#ifndef DBA_MSG_DATUM_H
#define DBA_MSG_DATUM_H

/** @file
 * @ingroup msg
 *
 * Store a dba_var together with time range metadata.
 */

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/var.h>

/**
 * Representation of one physical value
 */
struct _dba_msg_datum
{
	/** dba_var with the value and its metadata */
	dba_var var;

	int pind;
	int p1;
	int p2;
};
/** @copydoc _dba_msg_datum */
typedef struct _dba_msg_datum* dba_msg_datum;

/**
 * Create a new dba_msg_datum
 *
 * @retval d
 *   The newly created datum.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_datum_create(int pind, int p1, int p2, dba_msg_datum* d);

/**
 * Copy an existing datum
 *
 * @param src
 *   The datum to copy.
 * @retval dst
 *   The newly created duplicate.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_datum_copy(dba_msg_datum src, dba_msg_datum* dst);

/**
 * Delete a dba_msg_datum
 *
 * @param d
 *   The datum to delete.
 */
void dba_msg_datum_delete(dba_msg_datum d);

/**
 * Compare two dba_msg_datum strutures, for use in sorting.
 *
 * @param d1
 *   First dba_msg_datum to compare
 * @param d2
 *   Second dba_msg_datum to compare
 * @return
 *   -1 if d1 < d2, 0 if d1 == d2, 1 if d1 > d2
 */
int dba_msg_datum_compare(const dba_msg_datum d1, const dba_msg_datum d2);

/**
 * Compare a dba_msg_datum struture with some datum information, for use in
 * sorting.
 *
 * @param d
 *   First dba_msg_datum to compare
 * @param code
 *   Variable code of the second datum in the comparison
 * @return
 *   -1 if p < code,ptype,p1,p2; 0 if p == code,ptype,p1,p2; 1 if p > code,ptype,p1,p2
 */
int dba_msg_datum_compare2(dba_msg_datum d, dba_varcode code, int pind, int p1, int p2);


#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
