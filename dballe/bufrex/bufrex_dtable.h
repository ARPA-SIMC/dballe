#ifndef BUFREX_DTABLE_H
#define BUFREX_DTABLE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Implement fast access to information about WMO expansion tables D.
 */

#include <dballe/core/dba_var.h>
#include <dballe/bufrex/bufrex_opcode.h>

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
 *   The error status (@see dba_err)
 */
dba_err bufrex_dtable_create(const char* id, bufrex_dtable* table);

/**
 * Query the bufrex_dtable
 *
 * @param table
 *   bufrex_dtable to query
 * @param var
 *   entry code (i.e. the XXYYY number in BXXYYY)
 * @param res
 *   the bufrex_opcode chain that contains the expansion elements
 *   (must be deallocated by the caller using bufrex_opcode_delete)
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_dtable_query(bufrex_dtable table, dba_varcode var, bufrex_opcode* res);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
