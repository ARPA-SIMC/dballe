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

struct _dba_msg_datum
{
	dba_var var;
	int pind, p1, p2;
};
typedef struct _dba_msg_datum* dba_msg_datum;

/** */
dba_err dba_msg_datum_create(dba_msg_datum* d, int pind, int p1, int p2);

/** */
dba_err dba_msg_datum_copy(dba_msg_datum src, dba_msg_datum* dst);

/** */
void dba_msg_datum_delete(dba_msg_datum d);

/** */
int dba_msg_datum_compare(const dba_msg_datum d1, const dba_msg_datum d2);

/** */
int dba_msg_datum_compare2(dba_msg_datum d, dba_varcode code, int pind, int p1, int p2);


#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
