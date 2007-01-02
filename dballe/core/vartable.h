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

#ifndef DBA_VARTABLE_H
#define DBA_VARTABLE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Implement fast access to information about WMO variables.
 *
 * The measured value of a physical quantity has little meaning without
 * specifying what quantity it represents, what units are used to measure it,
 * and how many digits are significant for the value.
 *
 * This module provides access to all this metadata:
 *
 * \li \b ::dba_varcode represents what is the quantity measured, and takes
 *    values from the WMO B tables used for BUFR and CREX encodings.
 *    The ::DBA_VAR macro can be used to construct ::dba_varcode values, and the
 *    ::DBA_VAR_F, ::DBA_VAR_X and ::DBA_VAR_Y macros can be used to access the
 *    various parts of the dba_varcode.
 * \li \b ::dba_varinfo contains all the expanded information about a variable:
 *    its ::dba_varcode, description, measurement units, significant digits,
 *    minimum and maximum values it can have and other information useful for
 *    serialisation and deserialisation of values.
 *
 * There are many B tables with slight differences used by different
 * meteorological centre or equipment.  This module allows to access 
 * different vartables using dba_vartable_create().
 *
 * DB-All.e provides a default B table called the "local B table" which is used
 * for the encoding-independent values.  The local B table has the desirable
 * property of having unambiguous entries for the various physical values:
 * normal B tables can have more than one, for example low accuracy and
 * high accuracy latitudes.  The local B table can be queried using
 * dba_varinfo_query_local().
 *
 * ::dba_vartable and ::dba_varinfo have special memory management: they are never
 * deallocated.  This is a precise design choice to speed up passing and
 * copying ::dba_varinfo values, that are used very intensely as they accompany
 * all the physical values processed by DB-All.e and its components.
 * This behaviour should not be a cause of memory leaks, since a software would
 * only need to access a limited amount of B tables during its lifetime.
 *
 * To construct a ::dba_varcode value one needs to provide three numbers: F, X
 * and Y.
 *
 * \li \b F (2 bits) identifies the type of table entry represented by the
 * dba_varcode, and is always 0 for B tables.  Different values are only used
 * during encoding and decoding of BUFR and CREX messages and are not in use in
 * other parts of DB-All.e.
 * \li \b X (6 bits) identifies a section of the table.
 * \li \b Y (8 bits) identifies the value within the section.  
 *
 * The normal text representation of a ::dba_varcode for a WMO B table uses the
 * format Bxxyyy.
 *
 * See @ref local_b_table for the contents of the local B table and their
 * relative ::dba_varcode values.
 */


#include <dballe/core/error.h>

/**
 * Holds the WMO variable code of a variable
 */
typedef short unsigned int dba_varcode;

/**
 * Describes how a dba_varinfo has been altered: it is used for supporting
 * variables coming from BUFR and CREX messages that use C codes to alter
 * variable information.
 */
typedef short unsigned int dba_alteration;

/**
 * Holds the information about a DBALLE variable.
 *
 * It never needs to be deallocated, as all the dba_varinfo returned by DB-ALLe
 * are pointers to memory-cached versions that are guaranteed to exist for all
 * the lifetime of the program.
 */
struct _dba_varinfo
{
	/** The variable code.  See @ref DBA_VAR, DBA_VAR_X, DBA_VAR_Y. */
	dba_varcode var;
	/** The variable description. */
	char desc[64];
	/** The measurement unit of the variable. */
	char unit[24];
	/** The scale of the variable.  When the variable is represented as an
	 * integer, it is multiplied by 10**scale */
	int scale;
	/** The reference value for the variable.  When the variable is represented
	 * as an integer, and after scaling, it is added this value */
	int ref;
	/** The length in digits of the integer representation of this variable
	 * (after scaling and changing reference value) */
	int len;
	/** The reference value for bit-encoding.  When the variable is encoded in
	 * a bit string, it is added this value */
	int bit_ref;
	/** The length in bits of the variable when encoded in a bit string (after
	 * scaling and changing reference value) */
	int bit_len;
	/** True if the variable is a string; false if it is a numeric value */
	int is_string;
	/** Minimum unscaled value the field can have */
	int imin;
	/** Maximum unscaled value the field can have */
	int imax;
	/** Minimum scaled value the field can have */
	double dmin;
	/** Maximum scaled value the field can have */
	double dmax;
	/** C-table alteration that has been applied to this entry */
	dba_alteration alteration;
	/** Othere altered versions of this varinfo */
	struct _dba_varinfo* alterations;
};
/** @copydoc _dba_varinfo */
typedef struct _dba_varinfo* dba_varinfo;

/**
 * Holds a variable information table
 *
 * It never needs to be deallocated, as all the dba_vartable returned by
 * DB-ALLe are pointers to memory-cached versions that are guaranteed to exist
 * for all the lifetime of the program.
 */
struct _dba_vartable;
/** @copydoc _dba_vartable */
typedef struct _dba_vartable* dba_vartable;

/**
 * Create a WMO variable code from its F, X and Y components.
 */
#define DBA_VAR(f, x, y) ((dba_varcode)( ((unsigned)(f)<<14) | ((unsigned)(x)<<8) | (unsigned)(y) ))

/**
 * Convert a XXYYY string to a WMO variable code.
 *
 * This is useful only in rare cases, such as when parsing tables; use
 * dba_descriptor_code to parse proper entry names such as "B01003" or "D21301".
 */
#define DBA_STRING_TO_VAR(str) ((dba_varcode)( \
		(( ((str)[0] - '0')*10 + ((str)[1] - '0') ) << 8) | \
		( ((str)[2] - '0')*100 + ((str)[3] - '0')*10 + ((str)[4] - '0') ) \
))

/**
 * Get the F part of a WMO variable code.
 */
#define DBA_VAR_F(code) ((code) >> 14)
/**
 * Get the X part of a WMO variable code.
 */
#define DBA_VAR_X(code) ((code) >> 8 & 0x3f)
/**
 * Get the Y part of a WMO variable code.
 */
#define DBA_VAR_Y(code) ((code) & 0xff)

/**
 * Create a variable alteration value
 */
#define DBA_ALT(width, scale) (((width)+128) << 8 | ((scale)+128))

/**
 * Read the width part of a variable alteration value
 */
#define DBA_ALT_WIDTH(code) (((code) >> 8) - 128)

/**
 * Read the scale part of a variable alteration value
 */
#define DBA_ALT_SCALE(code) (((code) & 0xff) - 128)

/**
 * Query informations about the DBALLE variable definitions
 * 
 * @param code
 *   The ::dba_varcode of the variable to query
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo is stored inside the dba_vartable, can be freely copied around
 *   and does not need to be deallocated.
 * @return
 *   The error indicator for this function (See @ref error.h)
 */
dba_err dba_varinfo_query_local(dba_varcode code, dba_varinfo* info);

/**
 * Query informations about the DBALLE variable definitions
 * 
 * @param code
 *   The ::dba_varcode of the variable to query
 * @param change
 *   WMO C table entry specify a change on the variable characteristics
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo is stored inside the dba_vartable, can be freely copied around
 *   and does not need to be deallocated.
 * @return
 *   The error indicator for this function (See @ref error.h)
 */
dba_err dba_varinfo_query_local_altered(dba_varcode code, dba_alteration change, dba_varinfo* info);

/**
 * Get a reference to the local B table
 *
 * @retval table
 *   The local B table.  dba_vartable structures are never deallocated: the
 *   pointer will be valid through the entire lifetime of the program, and can
 *   be freely copied.
 * @return
 *   The error indicator for this function (See @ref error.h)
 */
dba_err dba_varinfo_get_local_table(dba_vartable* table);

/**
 * Convert a FXXYYY string descriptor code into its short integer
 * representation.
 *
 * @param desc
 *   The 6-byte string descriptor as FXXYYY
 *
 * @return
 *   The short integer code that can be queried with the DBA_GET_* macros
 */
dba_varcode dba_descriptor_code(const char* desc);

/**
 * Create a new vartable structure
 *
 * @param id
 *   ID of the vartable data to access
 * @retval table
 *   The vartable to access the data.  dba_vartable structures are never
 *   deallocated: the pointer will be valid through the entire lifetime of the
 *   program, and can be freely copied.
 * @return
 *   The error status (See @ref error.h)
 */
dba_err dba_vartable_create(const char* id, dba_vartable* table);

/**
 * Return the ID for the given table
 *
 * @param table
 *   The table to query
 * @return
 *   The table id.  It is a pointer to the internal value, and does not need to
 *   be deallocated.
 */
const char* dba_vartable_id(dba_vartable table);

/**
 * Query the vartable
 *
 * @param table
 *   vartable to query
 * @param var
 *   vartable entry number (i.e. the XXYYY number in BXXYYY)
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo is stored inside the dba_vartable, can be freely copied around
 *   and does not need to be deallocated.
 * @return
 *   The error status (See @ref error.h)
 */
dba_err dba_vartable_query(dba_vartable table, dba_varcode var, dba_varinfo* info);

/**
 * Query an altered version of the vartable
 *
 * @param table
 *   vartable to query
 * @param var
 *   vartable entry number (i.e. the XXYYY number in BXXYYY)
 * @param change
 *   WMO C table entry specify a change on the variable characteristics
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo is stored inside the dba_vartable, can be freely copied around
 *   and does not need to be deallocated.
 * @return
 *   The error status (See @ref error.h)
 */
dba_err dba_vartable_query_altered(dba_vartable table, dba_varcode var, dba_alteration change, dba_varinfo* info);

/**
 * Type of callback called by dba_vartable_iterate() on every member of a dba_vartable
 *
 * @param info
 *   Element of the table that is currently visited.
 * @param data
 *   Arbitrary user-supplied data given as the data parameter in dba_vartable_iterate()
 */
typedef void (*dba_vartable_iterator)(dba_varinfo info, void* data);

/**
 * Iterate through all elements in a dba_vartable
 *
 * @param table
 *   Table to iterate.
 * @param func
 *   Callback to be called for every item of the table.
 * @param data
 *   Arbitrary value passed as-is to the callback.
 * @return
 *   The error status (See @ref error.h)
 */
dba_err dba_vartable_iterate(dba_vartable table, dba_vartable_iterator func, void* data);

#ifdef  __cplusplus
}
#endif

#endif
/* vim:set ts=4 sw=4: */
