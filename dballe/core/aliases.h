/*
 * wreport/aliases - Aliases for commonly used variable codes
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef WREPORT_CORE_ALIASES_H
#define WREPORT_CORE_ALIASES_H

/** @file
 * @ingroup core
 * Resolve aliases to variable codes
 */

#include <wreport/varinfo.h>

namespace wreport {

/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve
 * @return
 *   The varcode corresponding to the aliase, or 0 if no variable has the given
 *   alias.  See @ref vartable.h
 */
Varcode varcode_alias_resolve(const char* alias);

/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve (does not need to be null-terminated)
 * @param len
 *   The length of the string
 * @return
 *   The varcode corresponding to the aliase, or 0 if no variable has the given
 *   alias.  See @ref vartable.h
 */
Varcode varcode_alias_resolve_substring(const char* alias, int len);

}

/* vim:set ts=4 sw=4: */
#endif
