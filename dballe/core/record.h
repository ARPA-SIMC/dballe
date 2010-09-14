/*
 * dballe/record - groups of related variables
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

#ifndef DBA_RECORD_H
#define DBA_RECORD_H

/** @file
 * @ingroup core
 * Implement a storage object for a group of related observation data
 */

#include <dballe/core/var.h>
#include <vector>

namespace dballe {

/**
 * Keyword used to quickly access context and query information from a record.
 */
enum _dba_keyword {
	DBA_KEY_ERROR		= -1,
	DBA_KEY_PRIORITY	=  0,
	DBA_KEY_PRIOMAX		=  1,
	DBA_KEY_PRIOMIN		=  2,
	DBA_KEY_REP_COD		=  3,
	DBA_KEY_REP_MEMO	=  4,
	DBA_KEY_ANA_ID		=  5,
	DBA_KEY_MOBILE		=  6,
	DBA_KEY_IDENT		=  7,
	DBA_KEY_LAT			=  8,
	DBA_KEY_LON			=  9,
	DBA_KEY_LATMAX		= 10,
	DBA_KEY_LATMIN		= 11,
	DBA_KEY_LONMAX		= 12,
	DBA_KEY_LONMIN		= 13,
	DBA_KEY_YEAR		= 14,
	DBA_KEY_MONTH		= 15,
	DBA_KEY_DAY			= 16,
	DBA_KEY_HOUR		= 17,
	DBA_KEY_MIN			= 18,
	DBA_KEY_SEC			= 19,
	DBA_KEY_YEARMAX		= 20,
	DBA_KEY_YEARMIN		= 21,
	DBA_KEY_MONTHMAX	= 22,
	DBA_KEY_MONTHMIN	= 23,
	DBA_KEY_DAYMAX		= 24,
	DBA_KEY_DAYMIN		= 25,
	DBA_KEY_HOURMAX		= 26,
	DBA_KEY_HOURMIN		= 27,
	DBA_KEY_MINUMAX		= 28,
	DBA_KEY_MINUMIN		= 29,
	DBA_KEY_SECMAX		= 30,
	DBA_KEY_SECMIN		= 31,
	DBA_KEY_LEVELTYPE1	= 32,
	DBA_KEY_L1			= 33,
	DBA_KEY_LEVELTYPE2	= 34,
	DBA_KEY_L2			= 35,
	DBA_KEY_PINDICATOR	= 36,
	DBA_KEY_P1			= 37,
	DBA_KEY_P2			= 38,
	DBA_KEY_VAR			= 39,
	DBA_KEY_VARLIST		= 40,
	DBA_KEY_CONTEXT_ID	= 41,
	DBA_KEY_QUERY		= 42,
	DBA_KEY_ANA_FILTER	= 43,
	DBA_KEY_DATA_FILTER	= 44,
	DBA_KEY_ATTR_FILTER	= 45,
	DBA_KEY_LIMIT		= 46,
	DBA_KEY_VAR_RELATED	= 47,
	DBA_KEY_COUNT		= 48,
};
/** @copydoc ::_dba_keyword */
typedef enum _dba_keyword dba_keyword;

/* TODO: should be deleted after checking if dbavm uses them */
#if 0
/* Shortcuts for commonly used variables */
#define DBA_VAR_BLOCK		DBA_VAR(0,  1,   1)
#define DBA_VAR_STATION		DBA_VAR(0,  1,   2)
#define DBA_VAR_NAME		DBA_VAR(0,  1,  19)
#define DBA_VAR_HEIGHT		DBA_VAR(0,  7,   1)
#define DBA_VAR_HEIGHTBARO	DBA_VAR(0,  7,  31)
#define DBA_VAR_DATA_ID		DBA_VAR(0, 33, 195)
#endif

/** DB-All.E record.
 *
 * A Record is a container for one observation of meteorological values, that
 * includes anagraphical informations, physical location of the observation in
 * time and space, and all the observed variables.
 */
class Record
{
protected:
	/* The storage for the core keyword data */
	Var* keydata[DBA_KEY_COUNT];

	/* The variables */
	std::vector<Var*> vars;

	/// Find an item by wreport::Varcode, returning -1 if not found
	int find_item(wreport::Varcode code) const throw ();

	/// Find an item by wreport::Varcode, raising an exception if not found
	Var& get_item(wreport::Varcode code);

	/// Find an item by wreport::Varcode, raising an exception if not found
	const Var& get_item(wreport::Varcode code) const;

	/// Remove an item by wreport::Varcode
	void remove_item(wreport::Varcode code);

public:
	Record();
	Record(const Record& rec);
	~Record();

	Record& operator=(const Record& rec);

	bool operator==(const Record& rec) const;

	/// Remove all data from the record
	void clear();

	/// Remove all variables from the record, leaving the keywords intact
	void clear_vars();

	/**
	 * Copy all data from the record source into dest.  At the end of the function,
	 * dest will contain its previous values, plus the values in source.  If a
	 * value is present both in source and in dest, the one in dest will be
	 * overwritten.
	 *
	 * @param source
	 *   The record to copy data from.
	 */
	void add(const Record& source);

	/**
	 * Set the record to contain only those fields that change source1 into source2.
	 *
	 * If a field has been deleted from source1 to source2, it will not be copied
	 * in dest.
	 *
	 * @param source1
	 *   The original record to compute the changes from.
	 * @param source2
	 *   The new record that has changes over source1.
	 */
	void set_to_difference(const Record& source1, const Record& source2);

	/**
	 * Look at the value of a parameter
	 *
	 * @return
	 *   A const pointer to the internal variable, or NULL if the variable has not
	 *   been found.
	 */
	const Var* key_peek(dba_keyword parameter) const throw ();

	/**
	 * Look at the value of a variable
	 *
	 * @return
	 *   A const pointer to the internal variable, or NULL if the variable has not
	 *   been found.
	 */
	const Var* var_peek(wreport::Varcode code) const throw ();

	/**
	 * Look at the raw value of a keyword in the record, without raising errors.
	 *
	 * @param parameter
	 *   The keyword to get the value for.
	 * @return
	 *   The raw string value, or NULL if the keyword has no value.
	 */
	const char* key_peek_value(dba_keyword parameter) const throw ();

	/**
	 * Look at the raw value of a variable in the record, without raising errors.
	 *
	 * @param code
	 *   The variable to get the value for.  See @ref vartable.h
	 * @return
	 *   The raw string value, or NULL if the variable has no value.
	 */
	const char* var_peek_value(wreport::Varcode code) const throw ();






	/**
	 * Return the name of a dba_keyword
	 *
	 * @return
	 *   The keyword name, or NULL if keyword is not a valid keyword
	 */
	static const char* keyword_name(dba_keyword keyword);

	/**
	 * Return informations about a keyword
	 *
	 * @return
	 *   The wreport::Varinfo structure with the informations.
	 */
	static wreport::Varinfo keyword_info(dba_keyword keyword);

	/**
	 * Get the dba_keyword corresponding to the given name
	 *
	 * @returns
	 *   The corresponding dba_keyword, or DBA_KEY_ERROR if tag does not match a
	 *   valid keyword.
	 */
	static dba_keyword keyword_byname(const char* tag);

	/**
	 * Get the dba_keyword corresponding to the given name
	 *
	 * @param tag
	 *   The name to query.
	 * @param len
	 *   The length of the name in tag.
	 * @returns
	 *   The corresponding dba_keyword, or DBA_KEY_ERROR if tag does not match a
	 *   valid keyword.
	 */
	static dba_keyword keyword_byname_len(const char* tag, int len);
};

#if 0
/**
 * Opaque structure representing a cursor used to iterate a dba_record.
 *
 * This object is a pointer to internal structures that does not need to be
 * explicitly created or deallocated.
 */
typedef struct _dba_item* dba_record_cursor;





/**
 * Check if a keyword or value is set
 *
 * @param rec
 *   The record to get the value from.
 * @param name
 *   The name of the item to check.
 * @retval found
 *   true if the record contains a value for the parameter, else false.
 * @return
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_contains(dba_record rec, const char* name, int* found);

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
 *   The error indicator for the function (See @ref error.h).
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
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_contains_var(dba_record rec, dba_varcode code, int* found);

/**
 * Get the value of an item, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param name
 *   The name of the item to get the value for.
 * @retval var
 *   A copy of the internal dba_var with the parameter.  You need to deallocate
 *   it with dba_var_delete().
 * @return
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_enq(dba_record rec, const char* name, dba_var* var);

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
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_key_enq(dba_record rec, dba_keyword parameter, dba_var* var);

/**
 * Get the value of a parameter, as dba_var.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.  See @ref vartable.h
 * @retval var
 *   A copy of the internal dba_var with the parameter.  You need to deallocate
 *   it with dba_var_delete().
 * @return
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_var_enq(dba_record rec, dba_varcode code, dba_var* var);

/**
 * Get the value of an item, as an unscaled integer.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param name
 *   The name of the item to get the value for.
 * @retval value
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_enqi(dba_record rec, const char* name, int* value, int* found);

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
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_enqi(dba_record rec, dba_keyword parameter, int* value, int* found);

/**
 * Get the value of a parameter, as an unscaled integer.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.  See @ref vartable.h
 * @retval value
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_enqi(dba_record rec, dba_varcode code, int* value, int* found);

/**
 * Get the value of an item, correctly scaled, in double precision.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param name
 *   The name of the item to get the value for.
 * @retval value
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_enqd(dba_record rec, const char* name, double* value, int* found);

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
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_key_enqd(dba_record rec, dba_keyword parameter, double* value, int* found);

/**
 * Get the value of a parameter, correctly scaled, in double precision.
 *
 * The function will fail if the value is a string instead of a number.
 *
 * @param rec
 *   The record to get the value from.
 * @param code
 *   The variable to get the value for.  See @ref vartable.h
 * @retval value
 *   The variable where the value, if found, should be stored.
 * @retval found
 *   1 if the value is set, 0 otherwise.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_enqd(dba_record rec, dba_varcode code, double* value, int* found);

/**
 * Get the value of an item.
 *
 * The function will return a string representation of the uncaled number if
 * the value is a number instead of a string.
 *
 * @param rec
 *   The record to get the value from.
 * @param name
 *   The name of the item to get the value for.
 * @retval value
 *   The variable where the value, if found, should be stored.  It will be set
 *   to NULL if the value is unset.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_enqc(dba_record rec, const char* name, const char** value);

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
 *   The variable where the value, if found, should be stored.  It will be set
 *   to NULL if the value is unset.
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
 *   The variable to get the value for.  See @ref vartable.h
 * @retval value
 *   The variable where the value, if found, should be stored.  It will be set
 *   to NULL if the value is unset.
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
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_key_set(dba_record rec, dba_keyword parameter, dba_var var);

/**
 * Set the value of a parameter, from a dba_var.  If dba_var has a value for a
 * different parameter, it will be converted.
 *
 * @param rec
 *   The record where the value is to be set.
 * @param code
 *   The variable to set the value for.  See @ref vartable.h
 * @param var
 *   A the dba_var with the parameter which will be copied inside the record.
 *   The record will copy the variable and will not take ownership of it:
 *   memory management will remain in charge of the caller.
 * @return
 *   The error indicator for the function (See @ref error.h).
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
 *   The error indicator for the function (See @ref error.h).
 */
dba_err dba_record_var_set_direct(dba_record rec, dba_var var);

/**
 * Set the date, level and timerange values to match the anagraphical context.
 *
 * @param rec
 *   The record where the value is to be set.
 * @return
 *   The error indicator for the function (See @ref error.h).
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
 *   The variable to set the value for.  See @ref vartable.h
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
 *   The variable to set the value for.  See @ref vartable.h
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
 *   The variable to set the value for.  See @ref vartable.h
 * @param value
 *   The value to set.  If the parameter is numeric, value will be taken as a
 *   string representing the unscaled integer with the value.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_var_setc(dba_record rec, dba_varcode code, const char* value);

/**
 * Set a value in the record according to an assignment encoded in a string.
 *
 * String can use keywords, aliases and varcodes.  Examples: ana_id=3,
 * name=Bologna, B12012=32.4
 *
 * @param rec
 *   The record where the value is to be set.
 * @param str
 *   The string containing the assignment.
 * @return
 *   The error indicator for the function.
 */
dba_err dba_record_set_from_string(dba_record rec, const char* str);

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
 *   The variable to remove.  See @ref vartable.h
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
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_record_parse_date_extremes(dba_record rec, int* minvalues, int* maxvalues);

#endif

}

/* vim:set ts=4 sw=4: */
#endif
