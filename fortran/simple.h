#ifndef FDBA_SIMPLE_H
#define FDBA_SIMPLE_H

/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

namespace dballef
{

struct API
{
	static const signed char missing_byte;
	static const int missing_int;
	static const float missing_float;
	static const double missing_double;

	virtual ~API() {}

	/**
	 * Reset the database contents, loading default report informations from a file.
	 *
	 * It only works in rewrite mode.
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param repinfofile
	 *   CSV file with the default report informations.  See dba_reset()
	 *   documentation for the format of the file.  If it is NULL or "\0", the
	 *   system default is used.
	 * @return
	 *   The error indicator for the function
	 */
	virtual void scopa(const char* repinfofile = 0) = 0;

	/**@name enq*
	 * @anchor enq
	 * Functions used to read the output values of the DBALLE action routines
	 * @{
	 */

	/**
	 * Read one integer value from the output record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   Where the value will be returned
	 * @return
	 *   The error indicator for the function
	 */
	virtual int enqi(const char* param) = 0;

	/**
	 * Read one byte value from the output record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   Where the value will be returned
	 * @return
	 *   The error indicator for the function
	 */
	virtual signed char enqb(const char* param) = 0;

	/**
	 * Read one real value from the output record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   Where the value will be returned
	 * @return
	 *   The error indicator for the function
	 */
	virtual float enqr(const char* param) = 0;

	/**
	 * Read one real*8 value from the output record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   Where the value will be returned
	 * @return
	 *   The error indicator for the function
	 */
	virtual double enqd(const char* param) = 0;

	/**
	 * Read one character value from the output record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   Where the value will be returned
	 * @return
	 *   The error indicator for the function
	 */
	virtual const char* enqc(const char* param) = 0;

	/*@}*/

	/**@name set*
	 * @anchor set
	 * Functions used to read the input values for the DBALLE action routines
	 *@{*/

	/**
	 * Set one integer value into the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   The value to assign to the parameter
	 * @return
	 *   The error indicator for the function
	 */
	virtual void seti(const char* param, int value) = 0;

	/**
	 * Set one byte value into the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   The value to assign to the parameter
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setb(const char* param, signed char value) = 0;

	/**
	 * Set one real value into the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   The value to assign to the parameter
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setr(const char* param, float value) = 0;

	/**
	 * Set one real*8 value into the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   The value to assign to the parameter
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setd(const char* param, double value) = 0;

	/**
	 * Set one character value into the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @param value
	 *   The value to assign to the parameter
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setc(const char* param, const char* value) = 0;

	/**
	 * Shortcut function to set query parameters to the anagraphical context
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setcontextana() = 0;

	/**
	 * Shortcut function to read level data.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval ltype
	 *   Level type from the output record
	 * @retval l1
	 *   L1 from the output record
	 * @retval l2
	 *   L2 from the output record
	 * @return
	 *   The error indicator for the function
	 */
	virtual void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) = 0;

	/**
	 * Shortcut function to set level data.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param ltype
	 *   Level type to set in the input record
	 * @param l1
	 *   L1 to set in the input record
	 * @param l2
	 *   L2 to set in the input record
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setlevel(int ltype1, int l1, int ltype2, int l2) = 0;

	/**
	 * Shortcut function to read time range data.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval ptype
	 *   P indicator from the output record
	 * @retval p1
	 *   P1 from the output record
	 * @retval p2
	 *   P2 from the output record
	 * @return
	 *   The error indicator for the function
	 */
	virtual void enqtimerange(int& ptype, int& p1, int& p2) = 0;

	/**
	 * Shortcut function to set time range data.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param ptype
	 *   P indicator to set in the input record
	 * @param p1
	 *   P1 to set in the input record
	 * @param p2
	 *   P2 to set in the input record
	 * @return
	 *   The error indicator for the function
	 */
	virtual void settimerange(int ptype, int p1, int p2) = 0;

	/**
	 * Shortcut function to read date information.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval year
	 *   Year from the output record
	 * @retval month
	 *   Month the output record
	 * @retval day
	 *   Day the output record
	 * @retval hour
	 *   Hour the output record
	 * @retval min
	 *   Minute the output record
	 * @retval sec
	 *   Second the output record
	 * @return
	 *   The error indicator for the function
	 */
	virtual void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) = 0;

	/**
	 * Shortcut function to set date information.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param year
	 *   Year to set in the input record
	 * @param month
	 *   Month to set in the input
	 * @param day
	 *   Day to set in the input
	 * @param hour
	 *   Hour to set in the input
	 * @param min
	 *   Minute to set in the input
	 * @param sec
	 *   Second to set in the input
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setdate(int year, int month, int day, int hour, int min, int sec) = 0;

	/**
	 * Shortcut function to set minimum date for a query.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param year
	 *   Minimum year to set in the query
	 * @param month
	 *   Minimum month to set in the query
	 * @param day
	 *   Minimum day to set in the query
	 * @param hour
	 *   Minimum hour to set in the query
	 * @param min
	 *   Minimum minute to set in the query
	 * @param sec
	 *   Minimum second to set in the query
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setdatemin(int year, int month, int day, int hour, int min, int sec) = 0;

	/**
	 * Shortcut function to set maximum date for a query.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param year
	 *   Maximum year to set in the query
	 * @param month
	 *   Maximum month to set in the query
	 * @param day
	 *   Maximum day to set in the query
	 * @param hour
	 *   Maximum hour to set in the query
	 * @param min
	 *   Maximum minute to set in the query
	 * @param sec
	 *   Maximum second to set in the query
	 * @return
	 *   The error indicator for the function
	 */
	virtual void setdatemax(int year, int month, int day, int hour, int min, int sec) = 0;

	/*@}*/

	/**
	 * Remove one parameter from the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param parameter
	 *   Parameter to remove.  It can be the code of a WMO variable prefixed by \c
	 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
	 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
	 *   dba_record_keywords
	 * @return
	 *   The error indicator for the function
	 */
	virtual void unset(const char* param) = 0;

	/**
	 * Remove all parameters from the input record
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 */
	virtual void unsetall() = 0;

	/**
	 * Count the number of elements in the anagraphical storage, and start a new
	 * anagraphical query.
	 *
	 * Resulting anagraphical data can be retrieved with elencamele()
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @param count
	 *   The count of elements
	 * @return
	 *   The error indicator for the function
	 */
	virtual int quantesono() = 0;

	/**
	 * Iterate through the anagraphical data.
	 *
	 * Every invocation of this function will return a new anagraphical data, or
	 * fill fail with code DBA_ERR_NOTFOUND when there are no more anagraphical
	 * data available.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void elencamele() = 0;

	/**
	 * Submit a query to the database.
	 *
	 * The query results can be accessed with calls to dammelo.
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval count
	 *   Number of values returned by the function
	 * @return
	 *   The error indicator for the function
	 */
	virtual int voglioquesto() = 0;

	/**
	 * Iterate through the query results data.
	 *
	 * Every invocation of this function will return a new result, or fill fail
	 * with code DBA_ERR_NOTFOUND when there are no more results available.
	 * 
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval parameter
	 *   Contains the ID of the parameter retrieved by this fetch
	 * @return
	 *   The error indicator for the function
	 */
	virtual const char* dammelo() = 0;

	/**
	 * Insert a new item in the database.
	 *
	 * This function will fail if the database is open in data readonly mode, and
	 * it will refuse to overwrite existing values if the database is open in data
	 * add mode.
	 *
	 * If the database is open in pseudoana reuse mode, the pseudoana values
	 * provided on input will be used to create a pseudoana record if it is
	 * missing, but will be ignored if it is already present.  If it is open in
	 * pseudoana rewrite mode instead, the pseudoana values on input will be used
	 * to replace all the existing pseudoana values.
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void prendilo() = 0;

	/**
	 * Remove all selected items from the database.
	 *
	 * This function will fail unless the database is open in data rewrite mode.
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void dimenticami() = 0;

	/**@name QC functions
	 * @anchor qc
	 * Functions used to manipulate QC data.
	 *
	 * All these functions require some context data about the variable, which is
	 * automatically available when the variable just came as the result of an
	 * dammelo() or has just been inserted with an prendilo().
	 *@{
	 */

	virtual int voglioancora() = 0;

	/**
	 * Retrieve QC informations from the last variable returned by dammelo().
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @retval parameter
	 *   Contains the ID of the parameter retrieved by this fetch
	 * @return
	 *   The error indicator for the function
	 */
	virtual const char* ancora() = 0;

	/**
	 * Insert new QC informations for a variable of the current record.
	 *
	 * QC informations inserted are all those set by the functions seti(),
	 * setc(), setr(), setd(), using an asterisk in front of the
	 * variable name.
	 *
	 * Contrarily to prendilo(), this function resets all the QC informations
	 * (but only the QC informations) previously set in input, so the values to be
	 * inserted need to be explicitly set every time.
	 *
	 * This function will fail if the database is open in QC readonly mode, and it
	 * will refuse to overwrite existing values if the database is open in QC add
	 * mode.
	 *
	 * The variable referred by the QC informations can be specified in three ways:
	 * 
	 * \li by variable code, using ::setc(handle, "*var", "Bxxyyy")
	 * \li by variable id, using ::seti(handle, "*data_id", id)
	 * \li unspecified, will use the last variable returned by ::dammelo
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void critica() = 0;

	/**
	 * Remove QC informations for a variable of the current record.
	 *
	 * The QC informations to be removed are set with:
	 * \code
	 *   setc(handle, "*varlist", "*B33021,*B33003");
	 * \endcode
	 *
	 * The variable referred by the QC informations can be specified in three ways:
	 * 
	 * \li by variable code, using ::setc(handle, "*var", "Bxxyyy")
	 * \li by variable id, using ::seti(handle, "*data_id", id)
	 * \li unspecified, will use the last variable returned by ::dammelo
	 *
	 * @param handle
	 *   Handle to a DBALLE session
	 * @return
	 *   The error indicator for the function
	 */
	virtual void scusa() = 0;

	/*@}*/

	virtual char* spiegal(int ltype1, int l1, int ltype2, int l2) = 0;

	virtual char* spiegat(int ptype, int p1, int p2) = 0;

	virtual char* spiegab(const char* varcode, const char* value) = 0;
};

}

/* vim:set ts=4 sw=4: */
#endif
