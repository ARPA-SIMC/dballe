/*
 * dballe/var - DB-All.e specialisation of wreport variable
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

#ifndef DBALLE_CORE_VAR_H
#define DBALLE_CORE_VAR_H

/** @file
 * @ingroup core
 * Implement ::dba_var, an encapsulation of a measured variable.
 */


#include <wreport/var.h>

namespace dballe {

/**
 * wreport variable with extra constructors using the local B table
 */
class Var : public wreport::Var
{
public:
	/// Create a new Var, from the local B table, with undefined value
	Var(wreport::Varcode code);

	/// Create a new Var, from the local B table, with integer value
	Var(wreport::Varcode code, int val);

	/// Create a new Var, from the local B table, with double value
	Var(wreport::Varcode code, double val);

	/// Create a new Var, from the local B table, with string value
	Var(wreport::Varcode code, const char* val);

	/// Create a new Var, with undefined value
	Var(wreport::Varinfo info) : wreport::Var(info) {}

	/// Create a new Var, with integer value
	Var(wreport::Varinfo info, int val) : wreport::Var(info, val) {}

	/// Create a new Var, with double value
	Var(wreport::Varinfo info, double val) : wreport::Var(info, val) {}

	/// Create a new Var, with character value
	Var(wreport::Varinfo info, const char* val) : wreport::Var(info, val) {}

	/// Copy constructor
	Var(const Var& var) : wreport::Var(var) {}

	/**
	 * Create a new Var with the value from another one
	 *
	 * Conversions are applied if necessary
	 *
	 * @param info
	 *   The wreport::Varinfo describing the variable to create
	 * @param orig
	 *   The variable with the value to use
	 */
	Var(wreport::Varinfo info, const Var& var) : wreport::Var(info, var) {}
};

}

#endif
/* vim:set ts=4 sw=4: */
