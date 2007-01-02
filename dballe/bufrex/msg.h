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

#ifndef DBALLE_BUFREX_MSG_H
#define DBALLE_BUFREX_MSG_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Intermediate representation of parsed values, ordered according to a BUFR or
 * CREX message template.
 */

#include <dballe/core/var.h>
#include <dballe/core/rawmsg.h>
#include <dballe/bufrex/dtable.h>
#include <dballe/bufrex/subset.h>

/**
 * Encoding type (BUFR or CREX)
 */
enum _bufrex_type {
	/** WMO BUFR */
	BUFREX_BUFR,
	/** WMO CREX */
	BUFREX_CREX
};
/** @copydoc _bufrex_type */
typedef enum _bufrex_type bufrex_type;

struct _bufrex_opcode;

/** BUFR-specific encoding options */
struct _bufrex_bufr_options {
	/** Common Code table C-1 identifying the originating centre */
	int origin;
	/** Version number of master tables used */
	int master_table;
	/** Version number of local tables used to augment the master table */
	int local_table;
};
/** CREX-specific encoding options */
struct _bufrex_crex_options {
	/** Master table (00 for standard WMO FM95 CREX tables) */
	int master_table;
	/** Table version number */
	int table;
};

/**
 * Storage for the decoded data of a BUFR or CREX message.
 */
struct _bufrex_msg
{
	/** Type of source/target encoding data */
	bufrex_type encoding_type;

	/** Encoding-specific information */
	union {
		struct _bufrex_crex_options crex;
		struct _bufrex_bufr_options bufr;
	} opt;

	/** Message category */
	int type;
	/** Message subcategory */
	int subtype;

	/** Edition number */
	int edition;

	/** Representative datetime for this data
	 * @{ */
	int rep_year;	/**< Year */
	int rep_month;	/**< Month */
	int rep_day;	/**< Day */
	int rep_hour;	/**< Hour */
	int rep_minute;	/**< Minute */
	/** @} */

	/** dba_vartable used to lookup B table codes */
	dba_vartable btable;
	/** bufrex_dtable used to lookup D table codes */
	bufrex_dtable dtable;

	/** Decoded variables */
	bufrex_subset* subsets;
	/** Number of decoded variables */
	size_t subsets_count;
	/** Size (in dba_var*) of the buffer allocated for vars */
	size_t subsets_alloclen;

	/** Parsed CREX data descriptor section */
	bufrex_opcode datadesc;
	/**
	 * Pointer to end of the datadesc chain, used to point to the insertion
	 * point for appends; it always points to a NULL pointer
	 */
	bufrex_opcode* datadesc_last;
};
/** @copydoc _bufrex_msg */
typedef struct _bufrex_msg* bufrex_msg;

/**
 * Create a bufrex_msg
 *
 * @param type
 *   Message type (BUFR or CREX)
 * @retval msg
 *   Newly created bufrex_msg
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err bufrex_msg_create(bufrex_type type, bufrex_msg* msg);

/**
 * Delete a bufrex_msg
 *
 * @param msg
 *   The bufrex_msg to delete
 */
void bufrex_msg_delete(bufrex_msg msg);

/**
 * Delete all the contents of a bufrex_msg.
 *
 * This can be used to reuse the structure to encode/decode more than one
 * message.
 *
 * @param msg
 *   The bufrex_msg to reset
 */
void bufrex_msg_reset(bufrex_msg msg);

/**
 * Get a dba_subset item from the message.
 *
 * The subset will be created if it does not exist.
 *
 * @param msg
 *   The bufrex_msg to query
 * @param subsection
 *   The subsection index (starting from 0)
 * @retval vars
 *   The subsection requested.  It can be newly created if the message did not
 *   contain such a subsection before, but memory management is handled by the
 *   bufrex_msg structure.
 * @return
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err bufrex_msg_get_subset(bufrex_msg msg, int subsection, bufrex_subset* vars);

/**
 * Get the ID of the table used by this bufrex_msg
 *
 * @retval id
 *   The table id, as a pointer to an internal string.  It must not be
 *   deallocated by the caller.  It is set to NULL when no table has been set.
 * @returns
 *   The error indicator for the function (See @ref dba_err)
 */
dba_err bufrex_msg_get_table_id(bufrex_msg msg, const char** id);

/**
 * Load a new set of tables to use for encoding this message
 */
dba_err bufrex_msg_load_tables(bufrex_msg msg);

/**
 * Query the WMO B table used for this BUFR/CREX data
 *
 * @param msg
 *   ::bufrex_msg to query
 * @param code
 *   code of the variable to query
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   ::dba_varinfo needs to be deallocated using dba_varinfo_delete()
 * @return
 *   The error status (See @ref dba_err)
 */
dba_err bufrex_msg_query_btable(bufrex_msg msg, dba_varcode code, dba_varinfo* info);

/**
 * Query the WMO D table used for this BUFR/CREX data
 * 
 * @param msg
 *   ::bufrex_msg to query
 * @param code
 *   code of the entry to query
 * @param res
 *   the bufrex_opcode chain that contains the expansion elements
 *   (must be deallocated by the caller using bufrex_opcode_delete)
 * @return
 *   The error status (See @ref dba_err)
 */
dba_err bufrex_msg_query_dtable(bufrex_msg msg, dba_varcode code, struct _bufrex_opcode** res);

/**
 * Reset the data descriptor section for the message
 *
 * @param msg
 *   The message to act on
 */
void bufrex_msg_reset_datadesc(bufrex_msg msg);

/**
 * Get the data descriptor section of this ::bufrex_msg
 *
 * @param msg
 *   The message to act on
 * @retval res
 *   A copy of the internal list of data descriptors for the data descriptor
 *   section.  It must be deallocated by the caller using
 *   bufrex_opcode_delete()
 * @returns
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err bufrex_msg_get_datadesc(bufrex_msg msg, struct _bufrex_opcode** res);

/**
 * Append one ::dba_varcode to the data descriptor section of the message
 *
 * @param msg
 *   The message to act on
 * @param varcode
 *   The ::dba_varcode to append
 * @returns
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err bufrex_msg_append_datadesc(bufrex_msg msg, dba_varcode varcode);

/**
 * Try to generate a data description section by scanning the variable codes of
 * the variables in the first data subset.
 *
 * @param msg
 *   The message to act on
 * @returns
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err bufrex_msg_generate_datadesc(bufrex_msg msg);

/**
 * Parse an encoded message into a bufrex_msg
 */
dba_err bufrex_msg_decode(bufrex_msg msg, dba_rawmsg raw);

/**
 * Encode the contents of the bufrex_msg
 */
dba_err bufrex_msg_encode(bufrex_msg msg, dba_rawmsg* raw);


/**
 * Encode a BUFR message
 * 
 * @param in
 *   The ::bufrex_msg with the data to encode
 * @param out
 *   The ::dba_rawmsg that will hold the encoded data
 * @return
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err bufr_encoder_encode(bufrex_msg in, dba_rawmsg out);

/**
 * Decode a BUFR message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded data
 * @retval opt
 *   A newly created bufr_options with informations about the decoding process.
 *   If NULL is passed, nothing will be returned.  If a bufr_options is
 *   returned, it will need to be deleted with bufr_options_delete().
 * @return
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err bufr_decoder_decode(dba_rawmsg in, bufrex_msg out);

/**
 * Encode a CREX message
 * 
 * @param in
 *   The ::bufrex_msg with the data to encode
 * @param out
 *   The ::dba_rawmsg that will hold the encoded data
 * @return
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err crex_encoder_encode(bufrex_msg in, dba_rawmsg out);

/**
 * Decode a CREX message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded data
 * @retval opt
 *   A newly created bufr_options with informations about the decoding process.
 *   If NULL is passed, nothing will be returned.  If a bufr_options is
 *   returned, it will need to be deleted with bufr_options_delete().
 * @return
 *   The error indicator for the function.  See @ref ::dba_err
 */
dba_err crex_decoder_decode(dba_rawmsg in, bufrex_msg out);

/**
 * Dump the contents of this bufrex_msg
 */
void bufrex_msg_print(bufrex_msg msg, FILE* out);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
