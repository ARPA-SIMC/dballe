/*
 * dballe/var - DB-All.e specialisation of wreport variable
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_CORE_VAR_H
#define DBALLE_CORE_VAR_H

/** @file
 * @ingroup core
 * Implement ::dba_var, an encapsulation of a measured variable.
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
/* vim:set ts=4 sw=4: */
