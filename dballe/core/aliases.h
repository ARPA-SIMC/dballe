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

/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve (does not need to be null-terminated)
 * @param len
 *   The length of the string
 * @return
 *   The varcode corresponding to the aliase, or 0 if no variable has the given
 *   alias.
 */
dba_varcode dba_varcode_alias_resolve_substring(const char* alias, int len);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
