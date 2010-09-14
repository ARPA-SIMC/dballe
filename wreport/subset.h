/*
 * wreport/subset - Data subset for BUFR and CREX messages
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

#ifndef WREPORT_SUBSET_H
#define WREPORT_SUBSET_H

/** @file
 * @ingroup bufrex
 * Handling of a BUFR/CREX data subset as a list of decoded variables.
 */

#include <wreport/var.h>
#include <wreport/vartable.h>
#include <wreport/dtable.h>
#include <vector>

namespace wreport {

/**
 * Represent a BUFR/CREX data subset as a list of decoded variables
 */
struct Subset : public std::vector<Var> {
	/// dba_vartable used to lookup B table codes
	const Vartable* btable;

	/// Decoded variables
	std::vector<Var> vars;

	/**
	 * Create a new BUFR/CREX subset.
	 *
	 * @param btable
	 *   Reference to the B table to use to create variables.
	 */
	Subset(const Vartable* btable);
	~Subset();

	/// Store a decoded variable in the message, to be encoded later.
	void store_variable(const Var& var);

	/**
	 * Store a new variable in the message, copying it from an already existing
	 * variable.
	 *
	 * @param code
	 *   The Varcode of the variable to add.  See @ref varinfo.h
	 * @param var
	 *   The variable holding the value for the variable to add.
	 */
	void store_variable(Varcode code, const Var& var);

	/**
	 * Store a new variable in the message, providing its value as an int
	 *
	 * @param code
	 *   The ::dba_varcode of the variable to add.  See @ref vartable.h
	 * @param val
	 *   The value for the variable
	 */
	void store_variable_i(Varcode code, int val);

	/**
	 * Store a new variable in the message, providing its value as a double
	 *
	 * @param code
	 *   The ::dba_varcode of the variable to add.  See @ref vartable.h
	 * @param val
	 *   The value for the variable
	 */
	void store_variable_d(Varcode code, double val);

	/**
	 * Store a new variable in the message, providing its value as a string
	 *
	 * @param code
	 *   The ::dba_varcode of the variable to add.  See @ref vartable.h
	 * @param val
	 *   The value for the variable
	 */
	void store_variable_c(Varcode code, const char* val);

	/**
	 * Store a new, undefined variable in the message
	 *
	 * @param code
	 *   The ::dba_varcode of the variable to add.  See @ref vartable.h
	 */
	void store_variable_undef(Varcode code);

	/// Compute the differences between two wreport subsets
	unsigned diff(const Subset& s2, FILE* out) const;
};

#if 0
/**
 * Compute and append a data present bitmap
 *
 * @param subset
 *   The message to operate on
 * @param ccode
 *   The C code that uses this bitmap
 * @param size
 *   The size of the bitmap
 * @param attr
 *   The code of the attribute that the bitmap will represent.  See @ref vartable.h
 * @retval count
 *   The number of attributes that will be encoded (for which the dpb has '+')
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufrex_subset_append_dpb(bufrex_subset subset, dba_varcode ccode, int size, dba_varcode attr, int* count);

/**
 * Append a fixed-size data present bitmap with all zeros
 *
 * @param subset
 *   The message to operate on
 * @param ccode
 *   The C code that uses this bitmap
 * @param size
 *   The size of the bitmap
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufrex_subset_append_fixed_dpb(bufrex_subset subset, dba_varcode ccode, int size);

/**
 * Scan the first 'size' variables appending the attribute 'attr' when found.
 *
 * The delayed replicator factor with the number of attributes found will also
 * be appended before the attributes.
 *
 * @param subset
 *   The message to operate on
 * @param size
 *   The number of variables to scan
 * @param attr
 *   The code of the attribute to look for.  See @ref vartable.h
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufrex_subset_append_attrs(bufrex_subset subset, int size, dba_varcode attr);
#endif


}

/* vim:set ts=4 sw=4: */
#endif
