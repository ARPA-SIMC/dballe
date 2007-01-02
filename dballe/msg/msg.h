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

#ifndef DBA_MSG_H
#define DBA_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup msg
 * Encoding-independent in-memory representation for any kind of weather
 * report.
 */

#include <dballe/msg/level.h>
#include <stdio.h>

/**
 * Source of the data
 */
enum _dba_msg_type {
	MSG_GENERIC,	/**< Data from unspecified source */
	MSG_SYNOP,		/**< Synop measured data */
	MSG_PILOT,		/**< Pilot sounding data */
	MSG_TEMP,		/**< Temp sounding data */
	MSG_TEMP_SHIP,	/**< Temp ship sounding data */
	MSG_AIREP,		/**< Airep airplane data */
	MSG_AMDAR,		/**< Amdar airplane data */
	MSG_ACARS,		/**< Acars airplane data */
	MSG_SHIP,		/**< Ship measured data */
	MSG_BUOY,		/**< Buoy measured data */
	MSG_METAR,		/**< Metar data */
	MSG_SAT			/**< Satellite data */
};
/** @copydoc _dba_msg_type */
typedef enum _dba_msg_type dba_msg_type;

/**
 * Storage for related physical data
 */
struct _dba_msg
{
 	/** Source of the data */
	dba_msg_type type;

	/** Number of levels in this message */
	int data_count;

	/**
	 * Number of levels allocated (must always be greater than or equal to
	 * data_count
	 */
	int data_alloc;

	/**
	 * The array with the data, reallocated as needed
	 */
	dba_msg_level* data;
};
/** @copydoc _dba_msg */
typedef struct _dba_msg* dba_msg;

/**
 * Return a string with the name of a dba_msg_type
 *
 * @param type
 *   The dba_msg_type value to name
 * @return
 *   The name, as a const string.  This function is thread safe.
 */
const char* dba_msg_type_name(dba_msg_type type);


/**
 * Create a new dba_msg
 *
 * @retval msg
 *   The newly created dba_msg.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_create(dba_msg* msg);

/**
 * Dump all the contents of the message to the given stream
 *
 * @param msg
 *   The dba_msg to dump
 * @param out
 *   The stream to dump the contents of the dba_msg to.
 */
void dba_msg_print(dba_msg msg, FILE* out);

/**
 * Print the differences between two dba_msg to a stream
 *
 * @param msg1
 *   First dba_msg to compare
 * @param msg2
 *   Second dba_msg to compare
 * @retval diffs
 *   Integer variable that will be incremented by the number of differences
 *   found.
 * @param out
 *   The stream to dump a description of the differences to.
 */
void dba_msg_diff(dba_msg msg1, dba_msg msg2, int* diffs, FILE* out);

/**
 * Delete a dba_msg
 *
 * @param msg
 *   The dba_msg to delete.
 */
void dba_msg_delete(dba_msg msg);


/**
 * Add or replace a value in the dba_msg
 *
 * @param msg
 *   The message to operate on
 * @param var
 *   The dba_var with the value to set, that will be copied into the dba_msg.
 * @param code
 *   The dba_varcode of the destination value.  If it is different than the
 *   varcode of var, a conversion will be attempted.
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_set(dba_msg msg, dba_var var, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);

/**
 * Add or replace a value in the dba_msg, taking ownership of the source
 * variable witout copying it.
 *
 * @param msg
 *   The message to operate on
 * @param var
 *   The dba_var with the value to set.  This dba_msg will take ownership of
 *   memory management.
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_set_nocopy(dba_msg msg, dba_var var, int ltype, int l1, int l2, int pind, int p1, int p2);

/**
 * Add or replace a value in the dba_msg
 *
 * @param msg
 *   The message to operate on
 * @param var
 *   The dba_var with the value to set, that will be copied into the dba_msg.
 * @param id
 *   Shortcut ID of the value to set (see @ref vars.h)
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_set_by_id(dba_msg msg, dba_var var, int id);

/**
 * Add or replace a value in the dba_msg, taking ownership of the source
 * variable witout copying it.
 *
 * @param msg
 *   The message to operate on
 * @param var
 *   The dba_var with the value to set.  This dba_msg will take ownership of
 *   memory management.
 * @param id
 *   Shortcut ID of the value to set (see @ref vars.h)
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_set_nocopy_by_id(dba_msg msg, dba_var var, int id);

/**
 * Add or replace an integer value in the dba_msg
 *
 * @param msg
 *   The message to operate on
 * @param code
 *   The dba_varcode of the destination value..  See @ref vartable.h
 * @param val
 *   The integer value of the data
 * @param conf
 *   The confidence interval of the data, as the value of a B33007 WMO B (per
 *   cent confidence) table entry, that is, a number between 0 and 100
 *   inclusive.
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_seti(dba_msg msg, dba_varcode code, int val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);

/**
 * Add or replace a double value in the dba_msg
 *
 * @param msg
 *   The message to operate on
 * @param code
 *   The dba_varcode of the destination value.  See @ref vartable.h
 * @param val
 *   The double value of the data
 * @param conf
 *   The confidence interval of the data, as the value of a B33007 WMO B (per
 *   cent confidence) table entry, that is, a number between 0 and 100
 *   inclusive.
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_setd(dba_msg msg, dba_varcode code, double val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);

/**
 * Add or replace a string value in the dba_msg
 *
 * @param msg
 *   The message to operate on
 * @param code
 *   The dba_varcode of the destination value.  See @ref vartable.h
 * @param val
 *   The string value of the data
 * @param conf
 *   The confidence interval of the data, as the value of a B33007 WMO B (per
 *   cent confidence) table entry, that is, a number between 0 and 100
 *   inclusive.
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_setc(dba_msg msg, dba_varcode code, const char* val, int conf, int ltype, int l1, int l2, int pind, int p1, int p2);


/**
 * Find a level given its description
 *
 * @param msg
 *   The dba_msg to query
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @return
 *   The level found, or NULL if it was not found.
 */
dba_msg_level dba_msg_find_level(dba_msg msg, int ltype, int l1, int l2);

/**
 * Find a datum given its description
 *
 * @param msg
 *   The dba_msg to query
 * @param code
 *   The ::dba_varcode of the variable to query. See @ref vartable.h
 * @param ltype
 *   Type of the level.  See @ref level_table.
 * @param l1
 *   L1 value of the level.  See @ref level_table.
 * @param l2
 *   L2 value of the level.  See @ref level_table.
 * @param pind
 *   Time range type indicator.  See @ref trange_table.
 * @param p1
 *   Time range P1 indicator.  See @ref trange_table.
 * @param p2
 *   Time range P2 indicator.  See @ref trange_table.
 * @return
 *   The level found, or NULL if it was not found.
 */
dba_msg_datum dba_msg_find(dba_msg msg, dba_varcode code, int ltype, int l1, int l2, int pind, int p1, int p2);

/** 
 * Find a datum given its shortcut ID
 *
 * @param msg
 *   The message to query
 * @param id
 *   Shortcut ID of the value to set (see @ref vars.h)
 * @return
 *   The value found, or NULL if it was not found.
 */
dba_msg_datum dba_msg_find_by_id(dba_msg msg, int id);


/**
 * Get the source of the data
 *
 * @param msg
 *   The dba_msg to query
 * @return
 *   The query source type
 */
dba_msg_type dba_msg_get_type(dba_msg msg);


/**
 * Get the message source type corresponding to the given report code
 */
dba_msg_type dba_msg_type_from_repcod(int repcod);

/**
 * Get the report code corresponding to the given message source type
 */
int dba_msg_repcod_from_type(dba_msg_type type);


/**
 * Copy a dba_msg, removing the sounding significance from the level
 * descriptions and packing together the data at the same pressure level.
 *
 * This is used to postprocess data after decoding, where the l2 field of the
 * level description is temporarily used to store the vertical sounding
 * significance, to simplify decoding.
 */
dba_err dba_msg_sounding_pack_levels(dba_msg msg, dba_msg* dst);

/**
 * Copy a dba_msg, adding the sounding significance from the level
 * descriptions and moving the data at the same pressure level to the resulting
 * pseudolevels.
 *
 * This is used to preprocess data before encoding, where the l2 field of the
 * level description is temporarily used to store the vertical sounding
 * significance, to simplify encoding.
 */
dba_err dba_msg_sounding_unpack_levels(dba_msg msg, dba_msg* dst);

#include <dballe/msg/vars.h>

#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
