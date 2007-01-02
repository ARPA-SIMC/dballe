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

#ifndef DBA_VAR_H
#define DBA_VAR_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement ::dba_var, an encapsulation of a measured variable.
 */


#include <dballe/core/dba_error.h>
#include <dballe/core/dba_vartable.h>
#include <stdio.h>

struct _dba_var;
/**
 * Holds a DBALLE variable
 *
 * A ::dba_var contains:
 * \li a ::dba_varcode identifying what is measured
 * \li a measured value, that can be an integer, double or string depending on
 *     the ::dba_varcode
 * \li zero or more attributes, in turn represented by ::dba_var structures
 */
typedef struct _dba_var* dba_var;

struct _dba_var_attr;
/**
 * Cursor for iterating through the attributes of a dba_var
 */
typedef struct _dba_var_attr* dba_var_attr_iterator;


/**
 * Create a new dba_var
 *
 * @param info
 *   The dba_varinfo that describes the variable
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_create(dba_varinfo info, dba_var* var);

/**
 * Create a new dba_var, setting it to an integer value
 *
 * @param info
 *   The dba_varinfo that describes the variable
 * @param val
 *   The initial value for the variable
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_createi(dba_varinfo info, int val, dba_var* var);

/**
 * Create a new dba_var, setting it to a double value
 *
 * @param info
 *   The dba_varinfo that describes the variable
 * @param val
 *   The initial value for the variable
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_created(dba_varinfo info, double val, dba_var* var);

/**
 * Create a new dba_var, setting it to a character value
 *
 * @param info
 *   The dba_varinfo that describes the variable
 * @param val
 *   The initial value for the variable
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_createc(dba_varinfo info, const char* val, dba_var* var);

/**
 * Create a variable with informations from the local table
 *
 * @param code
 *   The dba_varcode that identifies the variable in the local B table.
 * @retval var
 *   The variable created.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_create_local(dba_varcode code, dba_var* var);

/**
 * Make an exact copy of a dba_var
 *
 * @param source
 *   The variable to copy
 * @retval dest
 *   The new copy of source.  It will need to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy(dba_var source, dba_var* dest);

/**
 * Delete a dba_var
 *
 * @param var
 *   The variable to delete
 */
void dba_var_delete(dba_var var);

/**
 * Check if two variables contains the same data
 *
 * @param var1
 *   First variable to compare
 * @param var2
 *   Second variable to compare
 * @returns
 *   1 if the two variables have the same data, 0 otherwise
 */
int dba_var_equals(dba_var var1, dba_var var2);

/**
 * Get the value of a dba_var, as an integer
 *
 * @param var
 *   The variable to query
 * @retval val
 *   The resulting value
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_enqi(dba_var var, int* val);

/**
 * Get the value of a dba_var, as a double
 *
 * @param var
 *   The variable to query
 * @retval val
 *   The resulting value
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_enqd(dba_var var, double* val);

/**
 * Get the value of a dba_var, as a string
 *
 * @param var
 *   The variable to query
 * @retval val
 *   The resulting value
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_enqc(dba_var var, const char** val);

/**
 * Set the value of a dba_var, from an integer value
 *
 * @param var
 *   The variable to set
 * @param val
 *   The value to set
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_seti(dba_var var, int val);

/**
 * Set the value of a dba_var, from a double value
 *
 * @param var
 *   The variable to set
 * @param val
 *   The value to set
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_setd(dba_var var, double val);

/**
 * Set the value of a dba_var, from a string value
 *
 * @param var
 *   The variable to set
 * @param val
 *   The value to set
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_setc(dba_var var, const char* val);

/**
 * Unset the value of a dba_var
 *
 * @param var
 *   The variable to unset
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_unset(dba_var var);

/**
 * Query variable attributes
 *
 * @param var
 *   The variable to query
 * @param code
 *   The dba_varcode of the attribute requested
 * @retval attr
 *   A pointer to the attribute if it exists, else NULL.  The pointer points to
 *   the internal representation and must not be deallocated by the caller.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_enqa(dba_var var, dba_varcode code, dba_var* attr);

/**
 * Set an attribute of the variable.  An existing attribute with the same
 * ::dba_varcode will be replaced.
 *
 * @param var
 *   The variable to work on
 * @param attr
 *   The attribute to add.  It will be copied inside var, and memory management
 *   will still be in charge of the caller.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_seta(dba_var var, dba_var attr);

/**
 * Set an attribute of the variable.  An existing attribute with the same
 * ::dba_varcode will be replaced.
 *
 * @param var
 *   The variable to work on
 * @param attr
 *   The attribute to add.  It will be used directly, and var will take care of
 *   its memory management.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_seta_nocopy(dba_var var, dba_var attr);

/**
 * Remove the attribute with the given code
 * 
 * @param var
 *   The variable to work on
 * @param code
 *   The dba_varcode of the attribute to remove
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_unseta(dba_var var, dba_varcode code);

/**
 * Remove all attributes from the variable
 * @param var
 *   The variable to work on
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
void dba_var_clear_attrs(dba_var var);

/**
 * Retrieve the dba_varcode for a variable.  This function cannot fail, as
 * dba_var always have a varcode value.
 *
 * @param var
 *   Variable to query
 * @returns
 *   The dba_varcode for the variable
 */
dba_varcode dba_var_code(dba_var var);

/**
 * Get informations about the variable
 * 
 * @param var
 *   The variable to query informations for
 * @returns info
 *   The dba_varinfo for the variable
 */
dba_varinfo dba_var_info(dba_var var);

/**
 * Retrieve the internal string representation of the value for a variable.
 *
 * @param var
 *   Variable to query
 * @returns
 *   A const pointer to the internal string representation, or NULL if the
 *   variable is not defined.
 */
const char* dba_var_value(dba_var var);

/**
 * Start iterating through all the attributes of a variable
 *
 * @param var
 *   The variable to work on
 * @returns
 *   The ::dba_var_attr_iterator to use to iterate the attributes
 */
dba_var_attr_iterator dba_var_attr_iterate(dba_var var);

/**
 * Advance a ::dba_var_attr_iterator to point to the next attribute
 *
 * @param iter
 *   The iterator to work on
 * @returns
 *   The iterator to the next attribute, or NULL if there are no more
 *   attributes
 */
dba_var_attr_iterator dba_var_attr_iterator_next(dba_var_attr_iterator iter);

/**
 * Get the attribute pointed by a ::dba_var_attr_iterator
 *
 * @param iter
 *   The iterator to work on
 * @returns
 *   The attribute currently pointed by the iterator
 */
dba_var dba_var_attr_iterator_attr(dba_var_attr_iterator iter);


/**
 * Copy a value from a variable to another, performing conversions if needed
 *
 * @param dest
 *   The variable to write the value to
 * @param orig
 *   The variable to read the value from
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy_val(dba_var dest, dba_var orig);

/**
 * Copy all the attributes from one variable to another.
 *
 * @param dest
 *   The variable that will hold the attributes.
 * @param src
 *   The variable with the attributes to copy.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_copy_attrs(dba_var dest, dba_var src);

/**
 * Convert a variable to an equivalent variable using different informations
 *
 * @param orig
 *   The variable to convert
 * @param info
 *   The ::dba_varinfo describing the target of the conversion
 * @retval conv
 *   The converted variable.  It needs to be deallocated using
 *   dba_var_delete().
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err dba_var_convert(dba_var orig, dba_varinfo info, dba_var* conv);

/**
 * Encode a double value into an integer value using varinfo encoding
 * informations
 *
 * @param fval
 *   Value to encode
 * @param info
 *   dba_varinfo structure to use for the encoding informations
 * @returns
 *   The double value encoded as an integer
 */
int dba_var_encode_int(double fval, dba_varinfo info);

/**
 * Decode a double value from integer value using varinfo encoding
 * informations
 *
 * @param val
 *   Value to decode
 * @param info
 *   dba_varinfo structure to use for the encoding informations
 * @returns
 *   The decoded double value
 */
double dba_var_decode_int(int val, dba_varinfo info);

/**
 * Print the variable to an output stream
 *
 * @param var
 *   The variable to print
 * @param out
 *   The output stream to use for printing
 */
void dba_var_print(dba_var var, FILE* out);

/**
 * Print the difference between two variables to an output stream.
 * If there is no difference, it does not print anything.
 *
 * @param var1
 *   The first variable to compare
 * @param var2
 *   The second variable to compare
 * @retval diffs
 *   Incremented by 1 if the variables differ
 * @param out
 *   The output stream to use for printing
 */
void dba_var_diff(dba_var var1, dba_var var2, int* diffs, FILE* out);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
