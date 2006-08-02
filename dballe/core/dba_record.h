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

#ifndef DBA_RECORD_H
#define DBA_RECORD_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/err/dba_error.h>
#include <dballe/core/dba_var.h>

typedef enum _dba_keyword {
	DBA_KEY_ERROR		= -1,
	DBA_KEY_PRIORITY	=  0,
	DBA_KEY_PRIOMAX		=  1,
	DBA_KEY_PRIOMIN		=  2,
	DBA_KEY_REP_COD		=  3,
	DBA_KEY_REP_MEMO	=  4,
	DBA_KEY_ANA_ID		=  5,
	DBA_KEY_BLOCK		=  6,
	DBA_KEY_STATION		=  7,
	DBA_KEY_MOBILE		=  8,
	DBA_KEY_IDENT		=  9,
	DBA_KEY_LAT			= 10,
	DBA_KEY_LON			= 11,
	DBA_KEY_LATMAX		= 12,
	DBA_KEY_LATMIN		= 13,
	DBA_KEY_LONMAX		= 14,
	DBA_KEY_LONMIN		= 15,
	DBA_KEY_DATETIME	= 16,
	DBA_KEY_YEAR		= 17,
	DBA_KEY_MONTH		= 18,
	DBA_KEY_DAY			= 19,
	DBA_KEY_HOUR		= 20,
	DBA_KEY_MIN			= 21,
	DBA_KEY_SEC			= 22,
	DBA_KEY_YEARMAX		= 23,
	DBA_KEY_YEARMIN		= 24,
	DBA_KEY_MONTHMAX	= 25,
	DBA_KEY_MONTHMIN	= 26,
	DBA_KEY_DAYMAX		= 27,
	DBA_KEY_DAYMIN		= 28,
	DBA_KEY_HOURMAX		= 29,
	DBA_KEY_HOURMIN		= 30,
	DBA_KEY_MINUMAX		= 31,
	DBA_KEY_MINUMIN		= 32,
	DBA_KEY_SECMAX		= 33,
	DBA_KEY_SECMIN		= 34,
	DBA_KEY_LEVELTYPE	= 35,
	DBA_KEY_L1			= 36,
	DBA_KEY_L2			= 37,
	DBA_KEY_PINDICATOR	= 38,
	DBA_KEY_P1			= 39,
	DBA_KEY_P2			= 40,
	DBA_KEY_VAR			= 41,
	DBA_KEY_VARLIST		= 42,
	DBA_KEY_CONTEXT_ID	= 43,
	DBA_KEY_QUERY		= 44,
	DBA_KEY_ANA_FILTER	= 45,
	DBA_KEY_DATA_FILTER	= 46,
	DBA_KEY_ATTR_FILTER	= 47,
	DBA_KEY_LIMIT		= 48,
	DBA_KEY_VAR_RELATED	= 49,
	DBA_KEY_COUNT		= 50,
} dba_keyword;

/* Shortcuts for commonly used variables */
#define DBA_VAR_BLOCK		DBA_VAR(0,  1,   1)
#define DBA_VAR_STATION		DBA_VAR(0,  1,   2)
#define DBA_VAR_NAME		DBA_VAR(0,  1,  19)
#define DBA_VAR_HEIGHT		DBA_VAR(0,  7,   1)
#define DBA_VAR_HEIGHTBARO	DBA_VAR(0,  7,  31)
#define DBA_VAR_DATA_ID		DBA_VAR(0, 33, 195)

struct _dba_record;
struct _dba_item;

/** Opaque structure representing a DBALLE record.
 *
 * A DBALLE record is a container for one observation of meteorological value,
 * that includes anagraphical informations, physical location of the
 * observation in time and space, and all the observed variables.
 *
 * This object is created with dba_record_create() and deleted with
 * dba_record_delete().
 */
typedef struct _dba_record* dba_record;

/**
 * Opaque structure representing a cursor used to iterate a dba_record.
 *
 * This object is a pointer to internal structures that does not need to be
 * explicitly created or deallocated.
 */
typedef struct _dba_item* dba_record_cursor;

/**
 * Return informations about a keyword
 *
 * @param keyword
 *   The keyword to look for informations about
 * @retval info
 *   The ::dba_varinfo structure with the informations.
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err dba_record_keyword_info(dba_keyword keyword, dba_varinfo* info);

/**
 * Get the dba_keyword corresponding to the given name
 *
 * @param tag
 *   The name to query.
 * @returns
 *   The corresponding dba_keyword, or DBA_KEY_ERROR if tag does not match a
 *   valid keyword.
 */
dba_keyword dba_record_keyword_byname(const char* tag);

/**
 * Create a new record
 *
 * @retval rec
 *   The record variable to initialize.
 * 
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_create(dba_record* rec);

/**
 * Delete an existing record, freeing all the resources used by it
 *
 * @param rec
 *   The record to delete.
 */
void dba_record_delete(dba_record rec);

/**
 * Remove all data from the record
 *
 * @param rec
 *   The record to empty.
 */
void dba_record_clear(dba_record rec);

/**
 * Remove all variables from the record, leaving the keywords intact.
 *
 * @param rec
 *   The record to operate on.
 */
void dba_record_clear_vars(dba_record rec);

/**
 * Copy all data from the record source into dest.  At the end of the function,
 * dest will contain the same data as source.
 *
 * @param dest
 *   The record to copy data into.
 * @param source
 *   The record to copy data from.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_copy(dba_record dest, dba_record source);

/**
 * Copy all data from the record source into dest.  At the end of the function,
 * dest will contain its previous values, plus the values in source.  If a
 * value is present both in source and in dest, the one in dest will be
 * overwritten.
 *
 * @param dest
 *   The record to copy data into.
 * @param source
 *   The record to copy data from.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_add(dba_record dest, dba_record source);

/**
 * Copy in dest only those fields that change source1 into source2.
 *
 * If a field has been deleted from source1 to source2, it will not be copied
 * in dest.
 *
 * @param dest
 *   The record to copy data into.
 * @param source1
 *   The original record to compute the changes from.
 * @param source2
 *   The new record that has changes over source1.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_difference(dba_record dest, dba_record source1, dba_record source2);

/**
 * Look at the value of a parameter, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The parameter to get the value for.
 * @return
 *   A const pointer to the internal variable, or NULL if the variable has not
 *   been found.
 */
dba_var dba_record_key_peek(dba_record rec, dba_keyword parameter);

/**
 * Look at the value of a parameter, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @return
 *   A const pointer to the internal variable, or NULL if the variable has not
 *   been found.
 */
dba_var dba_record_var_peek(dba_record rec, dba_varcode code);

/**
 * Look at the raw value of a keyword in the record, without raising errors.
 *
 * @deprecated This function is not to be considered a stable part of the API,
 * but it is used by higher level to have a value lookup function that does not
 * trigger error callbacks if nothing is found.
 *
 * @param rec
 *   The record to get the value from.
 *
 * @param parameter
 *   The keyword to get the value for.
 *
 * @return
 *   The raw string value, or NULL if the keyword has no value.
 */
const char* dba_record_key_peek_value(dba_record rec, dba_keyword parameter);

/**
 * Look at the raw value of a variable in the record, without raising errors.
 *
 * @deprecated This function is not to be considered a stable part of the API,
 * but it is used by higher level to have a value lookup function that does not
 * trigger error callbacks if nothing is found.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @return
 *   The raw string value, or NULL if the variable has no value.
 */
const char* dba_record_var_peek_value(dba_record rec, dba_varcode code);

/**
 * Check if a keyword is set
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The key to check.
 * @retval found
 *   true if the record contains a value for the parameter, else false.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_contains_key(dba_record rec, dba_keyword parameter, int* found);

/**
 * Check if a variable is set
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to check.
 * @retval found
 *   true if the record contains a value for the parameter, else false.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_contains_var(dba_record rec, dba_varcode code, int* found);

/**
 * Get the value of a parameter, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The parameter to get the value for.
 * @retval var
 *   A copy of the internal dba_var with the parameter.  You need to deallocate
 *   it with dba_var_delete().
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_key_enq(dba_record rec, dba_keyword parameter, dba_var* var);

/**
 * Get the value of a parameter, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @retval var
 *   A copy of the internal dba_var with the parameter.  You need to deallocate
 *   it with dba_var_delete().
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_var_enq(dba_record rec, dba_varcode code, dba_var* var);

/**
 * Get the value of a parameter, as an unscaled integer.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The parameter to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_enqi(dba_record rec, dba_keyword parameter, int* value);

/**
 * Get the value of a parameter, as an unscaled integer.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_enqi(dba_record rec, dba_varcode code, int* value);

/**
 * Get the value of a parameter, correctly scaled, in double precision.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The parameter to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_enqd(dba_record rec, dba_keyword parameter, double* value);

/**
 * Get the value of a parameter, correctly scaled, in double precision.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_enqd(dba_record rec, dba_varcode code, double* value);

/**
 * Get the value of a parameter.
 *
 * The function will return a string representation of the uncaled number if
 * the value is a number instead of a string.
 *
 * @param rec
 *   The record to get the value from.
 * @param parameter
 *   The parameter to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_enqc(dba_record rec, dba_keyword parameter, const char** value);

/**
 * Get the value of a parameter.
 *
 * The function will return a string representation of the uncaled number if
 * the value is a number instead of a string.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.
 * @retval value
 *   The variable where the value should be stored.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_enqc(dba_record rec, dba_varcode code, const char** value);

/**
 * Set the value of a parameter, from a dba_var.  If dba_var has a value for a
 * different parameter, it will be converted.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param parameter
 *   The parameter to set the value for.  It can be the code of a WMO variable
 *   prefixed by "B" (such as \c "B01023") or a keyword among the ones defined
 *   in \ref dba_record_keywords
 * @param var
 *   A the dba_var with the parameter which will be copied inside the record.
 *   The record will copy the variable and will not take ownership of it:
 *   memory management will remain in charge of the caller.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_key_set(dba_record rec, dba_keyword parameter, dba_var var);

/**
 * Set the value of a parameter, from a dba_var.  If dba_var has a value for a
 * different parameter, it will be converted.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to set the value for.
 * @param var
 *   A the dba_var with the parameter which will be copied inside the record.
 *   The record will copy the variable and will not take ownership of it:
 *   memory management will remain in charge of the caller.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_var_set(dba_record rec, dba_varcode code, dba_var var);

/**
 * Set the value of a parameter, from a dba_var.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param var
 *   A the dba_var with the parameter which will be copied inside the record.
 *   The record will copy the variable and will not take ownership of it:
 *   memory management will remain in charge of the caller.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_var_set_direct(dba_record rec, dba_var var);

/**
 * Set the date, level and timerange values to match the anagraphical context.
 *
 * @param rec
 *   The record where the value is to be set.
 * @return
 *   The error indicator for the function (@see dba_err).
 */
dba_err dba_record_set_ana_context(dba_record rec);

/**
 * Set the value of a parameter.
 *
 * The function will fail if the keyword is a string instead of a number.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param parameter
 *   The parameter to set the value for.
 * @param value
 *   The value to set, as an unscaled integer.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_seti(dba_record rec, dba_keyword parameter, int value);

/**
 * Set the value of a parameter.
 *
 * The function will fail if the variable is a string instead of a number.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to set the value for.
 * @param value
 *   The value to set, as an unscaled integer.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_seti(dba_record rec, dba_varcode code, int value);

/**
 * Set the value of a parameter, in double precision.
 *
 * The function will fail if the keyword is a string instead of a number.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param parameter
 *   The parameter to set the value for.
 * @param value
 *   The value to set.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_setd(dba_record rec, dba_keyword parameter, double value);

/**
 * Set the value of a parameter, in double precision.
 *
 * The function will fail if the variable is a string instead of a number.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to set the value for.
 * @param value
 *   The value to set.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_setd(dba_record rec, dba_varcode code, double value);

/**
 * Set the value of a parameter.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param parameter
 *   The parameter to set the value for.
 * @param value
 *   The value to set.  If the parameter is numeric, value will be taken as a
 *   string representing the unscaled integer with the value.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_setc(dba_record rec, dba_keyword parameter, const char* value);

/**
 * Set the value of a parameter.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to set the value for.
 * @param value
 *   The value to set.  If the parameter is numeric, value will be taken as a
 *   string representing the unscaled integer with the value.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_setc(dba_record rec, dba_varcode code, const char* value);

/**
 * Remove a parameter from the record.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param parameter
 *   The parameter to remove.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_unset(dba_record rec, dba_keyword parameter);

/**
 * Remove a parameter from the record.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to remove.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_unset(dba_record rec, dba_varcode code);

/**
 * Print the contents of this record to the given file descriptor
 *
 * @param rec
 *   The record to print
 * @param out
 *   The output file descriptor
 */
void dba_record_print(dba_record rec, FILE* out);

/**
 * Print the difference between two records to an output stream.
 * If there is no difference, it does not print anything.
 *
 * @param rec1
 *   The first record to compare
 * @param rec2
 *   The second record to compare
 * @retval diffs
 *   Incremented by 1 if the variables differ
 * @param out
 *   The output stream to use for printing
 */
void dba_record_diff(dba_record rec1, dba_record rec2, int* diffs, FILE* out);

/**
 * Start iterating through the values in a record
 *
 * @param rec
 *   The record to iterate on.
 * 
 * @return
 *   The cursor pointing to the first item in the record,
 *   or NULL if the record is empty.
 */
dba_record_cursor dba_record_iterate_first(dba_record rec);

/**
 * Continue iterating through the values in a record
 *
 * @param rec
 *   The record to iterate on.
 * 
 * @param cur
 *   The cursor returned by dba_record_iterate_first or dba_record_iterate_next
 * 
 * @return
 *   The cursor pointing to the next item in the record,
 *   or NULL if there are no more items.
 */
dba_record_cursor dba_record_iterate_next(dba_record rec, dba_record_cursor cur);

/**
 * Get the variable pointed by a dba_record_cursor
 * 
 * @param cur
 *   The cursor returned by dba_record_iterate_first or dba_record_iterate_next
 *
 * @return
 *   The variable pointed by the cursor
 */
dba_var dba_record_cursor_variable(dba_record_cursor cur);

/**
 * Parse the date extremes set in the dba_record.
 *
 * This function will examine the values yearmin, monthmin, daymin, hourmin,
 * minumin, secmin, yearmax, monthmax, daymax, hourmax, minumax, secmax, year,
 * month, day, hour, min and sec, and will compute the two datetime extremes
 * that bound the interval they represent.
 *
 * @param rec
 *   The record that holds the datetime specifications
 * @retval minvalues
 *   An array of 6 integers that will be filled with the minimum year, month,
 *   day, hour, minute and seconds.
 * @retval maxvalues
 *   An array of 6 integers that will be filled with the maximum year, month,
 *   day, hour, minute and seconds.
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err dba_record_parse_date_extremes(dba_record rec, int* minvalues, int* maxvalues);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
