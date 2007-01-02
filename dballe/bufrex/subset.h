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

#ifndef DBALLE_BUFREX_SUBSET_H
#define DBALLE_BUFREX_SUBSET_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Handling of a BUFR/CREX data subset as a list of decoded variables.
 */

#include <dballe/core/var.h>
#include <dballe/bufrex/dtable.h>

/**
 * Represent a BUFR/CREX data subset as a list of decoded variables
 */
struct _bufrex_subset {
	/** dba_vartable used to lookup B table codes (reference to the one in
	 * bufrex_raw: memory management is done by bufrex_raw) */
	dba_vartable btable;

	/** Decoded variables */
	dba_var* vars;
	/** Number of decoded variables */
	int vars_count;
	/** Size (in dba_var*) of the buffer allocated for vars */
	int vars_alloclen;

};
typedef struct _bufrex_subset* bufrex_subset;

/**
 * Create a new BUFR/CREX subset.
 *
 * @param btable
 *   Reference to the B table to use to create variables.
 * @retval subset
 *   The newly created ::bufrex_subset.
 * @return
 *   The error indicator for the function.  @see dba_err.
 */
dba_err bufrex_subset_create(dba_vartable btable, bufrex_subset* subset);

/**
 * Deallocate a bufrex_subset.
 */
void bufrex_subset_delete(bufrex_subset subset);

/**
 * Clear the subset, removing all the variables from it.
 */
void bufrex_subset_reset(bufrex_subset subset);

/**
 * Store a decoded variable in the message, to be encoded later.
 *
 * The function will take ownership of the dba_var, and when the message is
 * destroyed or reset, ::dba_var_delete() will be called on it.
 *
 * \param subset
 *   The message that will hold the variable
 * \param var
 *   The variable to store in the message.  The message will take ownership of
 *   memory management for the variable, which will be deallocated when the
 *   message is deleted or reset.
 * \return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable(bufrex_subset subset, dba_var var);

/**
 * Store a new variable in the message, copying it from an already existing
 * variable.
 *
 * @param subset
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param var
 *   The variable holding the value for the variable to add
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable_var(bufrex_subset subset, dba_varcode code, dba_var var);

/**
 * Store a new variable in the message, providing its value as an int
 *
 * @param subset
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable_i(bufrex_subset subset, dba_varcode code, int val);

/**
 * Store a new variable in the message, providing its value as a double
 *
 * @param subset
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable_d(bufrex_subset subset, dba_varcode code, double val);

/**
 * Store a new variable in the message, providing its value as a string
 *
 * @param subset
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable_c(bufrex_subset subset, dba_varcode code, const char* val);

/**
 * Store a new, undefined variable in the message
 *
 * @param subset
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_store_variable_undef(bufrex_subset subset, dba_varcode code);

/**
 * Add the attribute 'addr' to the last variable that was previously stored.
 *
 * The attribute is copied into the variable, so memory management of it will
 * still belong to the caller.
 *
 * @param subset
 *   The message to operate on
 * @param attr
 *   The attribute to copy in the last variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_add_attr(bufrex_subset subset, dba_var attr); 

/**
 * Copy all the attributes from 'var' into the last variable that was
 * previously stored.
 *
 * @param subset
 *   The message to operate on
 * @param var
 *   The variable with the attributes to copy
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_add_attrs(bufrex_subset subset, dba_var var); 

/**
 * Copy decoded variables that are attributes as attributes in the decoded
 * variables they refer to.
 *
 * @param subset
 *   The message to operate on
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_apply_attributes(bufrex_subset subset);

/**
 * Compute and append a data present bitmap
 *
 * @param subset
 *   The message to operate on
 * @param size
 *   The size of the bitmap
 * @param attr
 *   The code of the attribute that the bitmap will represent
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_append_dpb(bufrex_subset subset, int size, dba_varcode attr);

/**
 * Append a fixed-size data present bitmap with all zeros
 *
 * @param subset
 *   The message to operate on
 * @param size
 *   The size of the bitmap
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_append_fixed_dpb(bufrex_subset subset, int size);

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
 *   The code of the attribute to look for
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_append_attrs(bufrex_subset subset, int size, dba_varcode attr);

/**
 * Scan the first 'size' variables appending the attribute 'attr' in any case.
 *
 * Exactly 'size' attributes will be appended, possibly with value 'undef' when
 * they are not present.  No delayed replicator factor is appended.
 *
 * @param subset
 *   The message to operate on
 * @param size
 *   The number of variables to scan
 * @param attr
 *   The code of the attribute to look for
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_subset_append_fixed_attrs(bufrex_subset subset, int size, dba_varcode attr);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
