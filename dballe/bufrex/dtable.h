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

#ifndef BUFREX_DTABLE_H
#define BUFREX_DTABLE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Implement fast access to information about WMO expansion tables D.
 */

#include <dballe/core/vartable.h>
#include <dballe/bufrex/opcode.h>

/**
 * Opaque structure representing a bufrex_dtable object
 */
typedef struct _bufrex_dtable* bufrex_dtable;

/**
 * Create a new bufrex_dtable structure
 *
 * @retval table
 *   The bufrex_dtable structure that can be used to access the table.  It is a
 *   pointer to a local shared cache that is guaranteed to live until the end
 *   of the program, and it does not need to be deallocated.
 *
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_dtable_create(const char* id, bufrex_dtable* table);

/**
 * Query the bufrex_dtable
 *
 * @param table
 *   bufrex_dtable to query
 * @param var
 *   entry code (i.e. DXXYYY as a ::dba_varcode DBA_VAR(3, xx, yyy).
 * @param res
 *   the bufrex_opcode chain that contains the expansion elements
 *   (must be deallocated by the caller using bufrex_opcode_delete)
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_dtable_query(bufrex_dtable table, dba_varcode var, bufrex_opcode* res);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
