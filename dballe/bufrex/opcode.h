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

#ifndef BUFREX_OPCODE_H
#define BUFREX_OPCODE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Implementation of opcode chains, that are used to drive the encoding and
 * decoding process.
 */

#include <dballe/core/vartable.h>

/**
 * List node containing an entry of a BUFR or CREX data descriptor section.
 */
struct _bufrex_opcode
{
	/** Data descriptor as a B, C or D table entry.  See @ref vartable.h */
	dba_varcode val;
	/** Next data descriptor in the list */
	struct _bufrex_opcode* next;
};
/** @copydoc _bufrex_opcode */
typedef struct _bufrex_opcode* bufrex_opcode;

/**
 * Delete a bufrex_opcode chain.
 *
 * @param entry
 *   The chain to be deleted.  After the call, entry will be NULL.
 */
void bufrex_opcode_delete(bufrex_opcode* entry);

/**
 * Append an entry to an opcode chain
 *
 * @param entry
 *   opcode entry to append the entry to
 * @param value
 *   Value to append to the entry.  See @ref vartable.h
 * @return
 *   The error indicator for the function
 */
dba_err bufrex_opcode_append(bufrex_opcode* entry, dba_varcode value);

/**
 * Duplicate an opcode chain and insert it before an existing bufrex_opcode
 *
 * To insert a chain in a specific point of an existing chain, do this:
 * \code
 *   DBA_RUN_OR_RETURN(bufrex_opcode_prepend(&(oldnode->next), newchain));
 * \endcode
 * 
 * This can be used to duplicate opcode chains, as in:
 * \code
 *   bufrex_opcode newchain = NULL;
 *   DBA_RUN_OR_RETURN(bufrex_opcode_prepend(&newchain, oldchain));
 * \endcode
 *
 * @param dest
 *   The bufrex_opcode before which the chain is to be copied
 *
 * @param src
 *   The bufrex_opcode chain to copy
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_prepend(bufrex_opcode* dest, bufrex_opcode src);

/**
 * Attach a bufrex_opcode chain to the end of another
 *
 * @param op1
 *   The first bufrex_opcode chain to be joined
 * @param op2
 *   bufrex_opcode chain to be attached to the end of op1
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_join(bufrex_opcode* op1, bufrex_opcode op2);

/**
 * Detach the first element from a bufrex_opcode list
 *
 * @param chain
 *   The bufrex_opcode chain whose first element is to be detached.
 *
 * @param head
 *   Where will be stored the detached first element of the opcode list.
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_pop(bufrex_opcode* chain, bufrex_opcode* head);

/**
 * Detach the first `length' elements from a bufrex_opcode list
 *
 * @param chain
 *   The bufrex_opcode chain whose first element is to be detached.
 *
 * @param head
 *   Where will be stored the detached first element of the opcode list.
 *
 * @param length
 *   Number of items to detach
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_pop_n(bufrex_opcode* chain, bufrex_opcode* head, int length);

/**
 * Print the contents of the given chain to the given output stream.
 *
 * This function is used mainly for debugging purposes.
 *
 * @param chain
 *   The bufrex_opcode chain to print
 * @param outstream
 *   The output stream (a FILE* variable) to print to.  void* is used to avoid
 *   including stdio.h just for this debugging function.
 */
void bufrex_opcode_print(bufrex_opcode chain, void* outstream);

#if 0
/**
 * Return the next opcode in an opcode chain
 *
 * @param entry
 *   Element of an opcode chain
 *
 * @returns
 *   The element in the chain after `entry'
 */
#define bufrex_opcode_next(entry) ((entry)->next)
/*bufrex_opcode bufrex_opcode_next(bufrex_opcode entry);*/

/**
 * Copy the first `length' elements of an opcode chain
 *
 * @param entry
 *   The chain to be copied
 *
 * @param length
 *   Number of elements to copy
 * 
 * @returns
 *   The copy of the first `length' elements of the chain
 */
bufrex_opcode bufrex_opcode_copy_n(bufrex_opcode entry, int length);

/* void bufrex_opcode_print(bufrex_opcode entry, FILE* out); */
#endif

#ifdef  __cplusplus
}
#endif

#endif
