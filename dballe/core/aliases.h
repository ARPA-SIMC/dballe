#ifndef WREPORT_CORE_ALIASES_H
#define WREPORT_CORE_ALIASES_H

/** @file
 * @ingroup core
 * Resolve aliases to variable codes
 */

#include <wreport/varinfo.h>
#include <string>

namespace dballe {

/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve
 * @return
 *   The varcode corresponding to the alias, or 0 if no variable has the given
 *   alias.
 */
wreport::Varcode varcode_alias_resolve(const char* alias);

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
wreport::Varcode varcode_alias_resolve_substring(const char* alias, int len);

/**
 * Resolve a variable alias.
 *
 * @param alias
 *   The alias to resolve
 * @return
 *   The varcode corresponding to the alias, or 0 if no variable has the given
 *   alias.
 */
wreport::Varcode varcode_alias_resolve(const std::string& alias);

}

#endif
