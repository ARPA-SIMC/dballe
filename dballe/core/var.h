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


#include <wreport/var.h>
#include <memory>
#include <string>

namespace dballe {

/**
 * Convenience functions to quickly create variables from the local B table
 */

/// Return a Varinfo entry from the local B table
wreport::Varinfo varinfo(wreport::Varcode code);

/**
 * Resolve a varcode name to a varcode proper.
 */
wreport::Varcode resolve_varcode(const char* name);

/// Resolve a varcode name to a varcode proper, performing careful validation
wreport::Varcode resolve_varcode_safe(const char* name);

/// Create a new Var, from the local B table, with undefined value
static inline wreport::Var var(wreport::Varcode code) { return wreport::Var(varinfo(code)); }

/// Create a new Var, from the local B table, with value
template<typename T>
wreport::Var var(wreport::Varcode code, const T& val) { return wreport::Var(varinfo(code), val); }


/// Create a new Var, from the local B table, with undefined value
static inline std::auto_ptr<wreport::Var> newvar(wreport::Varcode code)
{
	return std::auto_ptr<wreport::Var>(new wreport::Var(varinfo(code)));
}

/// Create a new Var, from the local B table, with value
template<typename T>
std::auto_ptr<wreport::Var> newvar(wreport::Varcode code, const T& val)
{
	return std::auto_ptr<wreport::Var>(new wreport::Var(varinfo(code), val));
}

/// Create a new Var, copying \a var and all its attributes except the unset ones
std::auto_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var);

/**
 * Create a new Var with code \a code, copying the value from \a var and all
 * its attributes except the unset ones
 */
std::auto_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var, wreport::Varcode code);

/**
 * Format the code to its string representation
 *
 * The string will be written to buf, which must be at least 7 bytes long
 */
void format_code(wreport::Varcode code, char* buf);

/// Format the code to its string representation
std::string format_code(wreport::Varcode code);

/// Return \a code, or its DB-All.e equivalent
wreport::Varcode map_code_to_dballe(wreport::Varcode code);

}

#endif
/* vim:set ts=4 sw=4: */
