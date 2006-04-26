#ifndef DBALLE_CORE_ALIASES_H
#define DBALLE_CORE_ALIASES_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Resolve aliases to variable codes
 */

#include <dballe/core/dba_var.h>


/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve
 * @return
 *   The varcode corresponding to the aliase, or 0 if no variable has the given
 *   alias.
 */
dba_varcode dba_varcode_alias_resolve(const char* alias);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
