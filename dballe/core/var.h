#ifndef DBALLE_CORE_VAR_H
#define DBALLE_CORE_VAR_H

/** @file
 * Shortcut functions to work with wreport::Var in DB-All.e
 */

#include <dballe/var.h>
#include <set>

namespace dballe {

/**
 * Convenience functions to quickly create variables from the local B table
 */

/// Resolve a comma-separated varcode list performing careful validation, inserting results in \a out
void resolve_varlist(const std::string& varlist, std::set<wreport::Varcode>& out);

/// Resolve a comma-separated varcode list performing careful validation, calling \a dest on each result
void resolve_varlist(const std::string& varlist, std::function<void(wreport::Varcode)> out);

/// Create a new Var, copying \a var and all its attributes except the unset ones
std::unique_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var);

/**
 * Create a new Var with code \a code, copying the value from \a var and all
 * its attributes except the unset ones
 */
std::unique_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var, wreport::Varcode code);

/**
 * Format the code to its string representation
 *
 * The string will be written to buf, which must be at least 7 bytes long
 */
void format_code(wreport::Varcode code, char* buf);

/// Return \a code, or its DB-All.e equivalent
wreport::Varcode map_code_to_dballe(wreport::Varcode code);

}

#endif
