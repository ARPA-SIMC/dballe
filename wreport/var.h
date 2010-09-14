/*
 * wreport/var - Store a value and its informations
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

#ifndef WREPORT_VAR_H
#define WREPORT_VAR_H

/** @file
 * @ingroup core
 * Implement ::dba_var, an encapsulation of a measured variable.
 */


#include <wreport/error.h>
#include <wreport/varinfo.h>
#include <stdio.h>
#include <memory>

struct lua_State;

namespace wreport {

/**
 * Holds a wreport variable
 *
 * A wreport::Var contains:
 * \li a wreport::Varcode identifying what is measured.  See @ref vartable.h
 * \li a measured value, that can be an integer, double or string depending on
 *     the wreport::Varcode
 * \li zero or more attributes, in turn represented by ::dba_var structures
 */
class Var
{
protected:
	/// Metadata about the variable
	Varinfo m_info;

	/// Value of the variable
	char* m_value;

	/// Attribute list (ordered by Varcode)
	Var* m_attrs;

public:
#if 0
	/// Create a new Var, from the local B table, with undefined value
	Var(Varcode code);

	/// Create a new Var, from the local B table, with integer value
	Var(Varcode code, int val);

	/// Create a new Var, from the local B table, with double value
	Var(Varcode code, double val);

	/// Create a new Var, from the local B table, with string value
	Var(Varcode code, const char* val);
#endif

	/// Create a new Var, with undefined value
	Var(Varinfo info);

	/// Create a new Var, with integer value
	Var(Varinfo info, int val);

	/// Create a new Var, with double value
	Var(Varinfo info, double val);

	/// Create a new Var, with character value
	Var(Varinfo info, const char* val);

	/// Copy constructor
	Var(const Var& var);

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
	Var(Varinfo info, const Var& var);

	~Var();
	
	/// Assignment
	Var& operator=(const Var& var);

	/// Equality
	bool operator==(const Var& var) const;
	bool operator!=(const Var& var) const { return !operator==(var); }

	/// Retrieve the Varcode for a variable
	Varcode code() const throw ();

	/// Get informations about the variable
	Varinfo info() const throw ();

	/// Retrieve the internal string representation of the value for a variable.
	const char* value() const throw ();

	/// Get the value as an integer
	int enqi() const;

	/// Get the value as a double
	double enqd() const;

	/// Get the value as a string
	const char* enqc() const;

	/// Set the value from an integer value
	void seti(int val);

	/// Set the value from a double value
	void setd(double val);

	/// Set the value from a string value
	void setc(const char* val);

	/// Unset the value of a dba_var
	void unset();

	/// Remove all attributes
	void clear_attrs();

	/**
	 * Query variable attributes
	 *
	 * @param code
	 *   The wreport::Varcode of the attribute requested.  See @ref vartable.h
	 * @returns attr
	 *   A pointer to the attribute if it exists, else NULL.  The pointer points to
	 *   the internal representation and must not be deallocated by the caller.
	 */
	const Var* enqa(Varcode code) const;

	/**
	 * Set an attribute of the variable.  An existing attribute with the same
	 * wreport::Varcode will be replaced.
	 *
	 * @param attr
	 *   The attribute to add.  It will be copied inside var, and memory management
	 *   will still be in charge of the caller.
	 */
	void seta(const Var& attr);

	/**
	 * Set an attribute of the variable.  An existing attribute with the same
	 * wreport::Varcode will be replaced.
	 *
	 * @param attr
	 *   The attribute to add.  It will be used directly, and var will take care of
	 *   its memory management.
	 */
	void seta(std::auto_ptr<Var> attr);

	/// Remove the attribute with the given code
	void unseta(Varcode code);

	/**
	 * Get the next attribute in the attribute list
	 *
	 * Example attribute iteration:
	 *
	 * for (const Var* a = var.next_attr(); a != NULL; a = a->next_attr())
	 * 	// Do something with a
	 */
	const Var* next_attr() const;

	/**
	 * Set the value from another variable, performing conversions if
	 * needed.
	 *
	 * The attributes of \a src will also be copied
	 */
	void copy_val(const Var& src);

	/**
	 * Copy all the attributes from another variable
	 *
	 * @param src
	 *   The variable with the attributes to copy.
	 */
	void copy_attrs(const Var& src);


	/**
	 * Print the variable to an output stream
	 *
	 * @param out
	 *   The output stream to use for printing
	 */
	void print(FILE* out) const;

	/**
	 * Print the difference between two variables to an output stream.
	 * If there is no difference, it does not print anything.
	 *
	 * @param var
	 *   The variable to compare with this one
	 * @param out
	 *   The output stream to use for printing
	 * @returns
	 *   The number of differences found and reported
	 */
	unsigned diff(const Var& var, FILE* out) const;


	/**
	 * Push the variable as an object in the lua stack
	 */
	void lua_push(struct lua_State* L);

	/**
	 * Check that the element at \a idx is a Var
	 *
	 * @return the Var element, or NULL if the check failed
	 */
	static Var* lua_check(struct lua_State* L, int idx);
};

}

#endif
/* vim:set ts=4 sw=4: */
