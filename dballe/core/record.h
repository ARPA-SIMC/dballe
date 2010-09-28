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
	wreport::Var* keydata[DBA_KEY_COUNT];

	/* The variables */
	std::vector<wreport::Var*> m_vars;

	/// Find an item by wreport::Varcode, returning -1 if not found
	int find_item(wreport::Varcode code) const throw ();

	/// Find an item by wreport::Varcode, raising an exception if not found
	wreport::Var& get_item(wreport::Varcode code);

	/// Find an item by wreport::Varcode, raising an exception if not found
	const wreport::Var& get_item(wreport::Varcode code) const;

	/// Remove an item by wreport::Varcode
	void remove_item(wreport::Varcode code);

public:
	Record();
	Record(const Record& rec);
	~Record();

	Record& operator=(const Record& rec);

	bool operator==(const Record& rec) const;
	bool operator!=(const Record& rec) const { return !operator==(rec); }

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
	 * Return true if all elements of \a subset are present in this record,
	 * with the same value
	 */
	bool contains(const Record& subset) const;

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
	const wreport::Var* key_peek(dba_keyword parameter) const throw ();

	/**
	 * Look at the value of a variable
	 *
	 * @return
	 *   A const pointer to the internal variable, or NULL if the variable has not
	 *   been found.
	 */
	const wreport::Var* var_peek(wreport::Varcode code) const throw ();

	/**
	 * Get the variable for an item
	 *
	 * @param name
	 *   The name of the item to get the value for
	 */
	const wreport::Var* peek(const char* name) const;

	/// Shortcut for key_peek
	const wreport::Var* peek(dba_keyword parameter) const throw () { return key_peek(parameter); }

	/// Shortcut for var_peek
	const wreport::Var* peek(wreport::Varcode code) const throw () { return var_peek(code); }

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
	 * Get the string value for an item
	 *
	 * @param name
	 *   The name of the item to get the value for
	 */
	const char* peek_value(const char* name) const;

	/**
	 * Return the Var for a key, throwing an error it if it missing
	 */
	const wreport::Var& key(dba_keyword parameter) const;

	/**
	 * Return the Var for a variable, throwing an error it if it missing
	 */
	const wreport::Var& var(wreport::Varcode code) const;

	/**
	 * Return the Var for a key, creating it if it missing
	 */
	wreport::Var& key(dba_keyword parameter);

	/**
	 * Return the Var for a variable, creating it if it missing
	 */
	wreport::Var& var(wreport::Varcode code);

	/// Shortcuts
	// @{
	const wreport::Var& get(dba_keyword parameter) const { return key(parameter); }
	const wreport::Var& get(wreport::Varcode code) const { return var(code); }
	const wreport::Var& get(const char* name) const;
	wreport::Var& get(dba_keyword parameter) { return key(parameter); }
	wreport::Var& get(wreport::Varcode code) { return var(code); }
	wreport::Var& get(const char* name);
	const wreport::Var& operator[](dba_keyword parameter) const { return key(parameter); }
	const wreport::Var& operator[](wreport::Varcode code) const { return var(code); }
	const wreport::Var& operator[](const char* name) const { return get(name); }
	wreport::Var& operator[](dba_keyword parameter) { return key(parameter); }
	wreport::Var& operator[](wreport::Varcode code) { return var(code); }
	wreport::Var& operator[](const char* name) { return get(name); }
	template<typename P, typename V>
	void set(const P& field, const V& val) { get(field).set(val); }
	void set(const wreport::Var& var) { get(var.code()).set(var); }
	void unset(dba_keyword parameter) { key_unset(parameter); }
	void unset(wreport::Varcode code) { var_unset(code); }
	void unset(const char* name);
	// @}

	/**
	 * Set the date, level and timerange values to match the anagraphical context.
	 */
	void set_ana_context();

	/**
	 * Return the vector with the variables
	 */
	const std::vector<wreport::Var*>& vars() const;

	/**
	 * Remove a parameter from the record.
	 *
	 * @param parameter
	 *   The parameter to remove.
	 */
	void key_unset(dba_keyword parameter);

	/**
	 * Remove a parameter from the record.
	 *
	 * @param code
	 *   The variable to remove.  See @ref vartable.h
	 */
	void var_unset(wreport::Varcode code);

	/**
	 * Parse the date extremes set in the dba_record.
	 *
	 * This function will examine the values yearmin, monthmin, daymin, hourmin,
	 * minumin, secmin, yearmax, monthmax, daymax, hourmax, minumax, secmax, year,
	 * month, day, hour, min and sec, and will compute the two datetime extremes
	 * that bound the interval they represent.
	 *
	 * @retval minvalues
	 *   An array of 6 integers that will be filled with the minimum year, month,
	 *   day, hour, minute and seconds.
	 * @retval maxvalues
	 *   An array of 6 integers that will be filled with the maximum year, month,
	 *   day, hour, minute and seconds.
	 */
	void parse_date_extremes(int* minvalues, int* maxvalues) const;

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
	void set_from_string(const char* str);

	/**
	 * Print the contents of this record to the given file descriptor
	 *
	 * @param out
	 *   The output file descriptor
	 */
	void print(FILE* out) const;

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
#endif

}

/* vim:set ts=4 sw=4: */
#endif
