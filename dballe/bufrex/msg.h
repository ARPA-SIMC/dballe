/*
 * DB-ALLe - Archive for punctual meteorological data
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

#ifndef DBALLE_BUFREX_MSG_H
#define DBALLE_BUFREX_MSG_H

/** @file
 * @ingroup bufrex
 * Intermediate representation of parsed values, ordered according to a BUFR or
 * CREX message template.
 */

#include <dballe/core/var.h>
#include <dballe/core/rawmsg.h>
#include <dballe/bufrex/subset.h>
#include <vector>
#include <memory>

namespace bufrex {

struct DTable;

/**
 * Storage for the decoded data of a BUFR or CREX message.
 */
struct Msg
{
	/// Reference count
	int _ref;

	/** Message category */
	int type;
	/** International message subcategory */
	int subtype;
	/** Local message subcategory */
	int localsubtype;

	/** Edition number */
	int edition;

	/** Representative datetime for this data
	 * @{ */
	int rep_year;	/**< Year */
	int rep_month;	/**< Month */
	int rep_day;	/**< Day */
	int rep_hour;	/**< Hour */
	int rep_minute;	/**< Minute */
	int rep_second;	/**< Second */
	/** @} */

	/** dba_vartable used to lookup B table codes */
	const dballe::Vartable* btable;
	/** bufrex_dtable used to lookup D table codes */
	const DTable* dtable;

	/** Parsed data descriptor section */
	std::vector<dballe::Varcode> datadesc;

	/** Decoded variables */
	std::vector<Subset> subsets;

	Msg();
	virtual ~Msg();

	virtual void clear();

	/** Type of source/target encoding data */
	virtual const dballe::Encoding encoding() const throw () = 0;

	/**
	 * Get a Subset from the message.
	 *
	 * The subset will be created if it does not exist, and it will be
	 * memory managed by the Msg.
	 *
	 * @param subsection
	 *   The subsection index (starting from 0)
	 */
	Subset& obtain_subset(unsigned subsection);

	/**
	 * Get a Subset from the message.
	 *
	 * An exception will be thrown if the subset does not exist
	 *
	 * @param subsection
	 *   The subsection index (starting from 0)
	 */
	const Subset& subset(unsigned subsection) const;

	/// Load a new set of tables to use for encoding this message
	virtual void load_tables() = 0;

	/**
	 * Parse only the header of an encoded message
	 */
	virtual void decode_header(const dballe::Rawmsg& raw) = 0;

	/**
	 * Parse an encoded message
	 */
	virtual void decode(const dballe::Rawmsg& raw) = 0;

	/**
	 * Encode the message
	 */
	virtual void encode(dballe::Rawmsg& raw) const = 0;

	/**
	 * Dump the contents of this bufrex_msg
	 */
	void print(FILE* out) const;

	/// Print format-specific details
	virtual void print_details(FILE* out) const;

	/**
	 * Compute the differences between two bufrex_msg
	 */
	virtual unsigned diff(const Msg& msg, FILE* out) const;

	/// Diff format-specific details
	virtual unsigned diff_details(const Msg& msg, FILE* out) const;

	/**
	 * Create a Msg for the given encoding
	 */
	static std::auto_ptr<Msg> create(dballe::Encoding encoding);
};

struct BufrMsg : public Msg
{
	/** BUFR-specific encoding options */

	/** Common Code table C-1 identifying the originating centre */
	int centre;
	/** Centre-specific subcentre code */
	int subcentre;
	/** Version number of master tables used */
	int master_table;
	/** Version number of local tables used to augment the master table */
	int local_table;

	/** 1 if the BUFR message uses compression, else 0 */
	int compression;
	/** Update sequence number from octet 7 in section 1*/
	int update_sequence_number;
	/** 0 if the BUFR message does not contain an optional section, else
	 *  its length in bytes */
	int optional_section_length;
	/** Raw contents of the optional section */
	char* optional_section;

	BufrMsg();
	virtual ~BufrMsg();

	void clear();
	const dballe::Encoding encoding() const throw () { return dballe::BUFR; }
	virtual void load_tables();
	virtual void decode_header(const dballe::Rawmsg& raw);
	virtual void decode(const dballe::Rawmsg& raw);
	virtual void encode(dballe::Rawmsg& raw) const;
	virtual void print_details(FILE* out) const;
	virtual unsigned diff_details(const Msg& msg, FILE* out) const;

	// Get encoding for class, to use in templates
	static dballe::Encoding class_encoding() { return dballe::BUFR; }
};

struct CrexMsg : public Msg
{
	/** CREX-specific encoding options */

	/** Master table (00 for standard WMO FM95 CREX tables) */
	int master_table;
	/** Table version number */
	int table;
	/** True if the CREX message uses the check digit feature */
	int has_check_digit;

	void clear();
	const dballe::Encoding encoding() const throw () { return dballe::CREX; }
	virtual void load_tables();
	virtual void decode_header(const dballe::Rawmsg& raw);
	virtual void decode(const dballe::Rawmsg& raw);
	virtual void encode(dballe::Rawmsg& raw) const;
	virtual void print_details(FILE* out) const;
	virtual unsigned diff_details(const Msg& msg, FILE* out) const;

	// Get encoding for class, to use in templates
	static dballe::Encoding class_encoding() { return dballe::CREX; }
};

#if 0
/**
 * Get the ID of the table used by this bufrex_msg
 *
 * @retval id
 *   The table id, as a pointer to an internal string.  It must not be
 *   deallocated by the caller.  It is set to NULL when no table has been set.
 * @returns
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_get_table_id(bufrex_msg msg, const char** id);

/**
 * Try to generate a data description section by scanning the variable codes of
 * the variables in the first data subset.
 *
 * @param msg
 *   The message to act on
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufrex_msg_generate_datadesc(bufrex_msg msg);

/**
 * Parse only the header of an encoded message into a bufrex_msg
 */
dba_err bufrex_msg_decode_header(bufrex_msg msg, dba_rawmsg raw);

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
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufr_encoder_encode(bufrex_msg in, dba_rawmsg out);

/**
 * Decode only the header of a BUFR message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded header data
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err bufr_decoder_decode_header(dba_rawmsg in, bufrex_msg out);

/**
 * Decode a BUFR message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded data
 * @return
 *   The error indicator for the function.  See @ref error.h
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
 *   The error indicator for the function.  See @ref error.h
 */
dba_err crex_encoder_encode(bufrex_msg in, dba_rawmsg out);

/**
 * Decode a CREX message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded data
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err crex_decoder_decode(dba_rawmsg in, bufrex_msg out);

/**
 * Decode only the header of a CREX message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_msg that will hold the decoded data
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
dba_err crex_decoder_decode_header(dba_rawmsg in, bufrex_msg out);
#endif

/**
 * Parse a string containing a bufr/crex template selector.
 *
 * The string can be 3 numbers separated by dots (type.subtype.localsubtype) or
 * an alias among those listed in template-aliases.txt
 *
 * @param str
 *   The string to parse
 * @retval cat
 *   The parsed message category
 * @retval subcat
 *   The parsed message subcategory
 * @retval localsubcat
 *   The parsed local category
 * @return
 *   The error indicator for the function.  See @ref error.h
 */
void parse_template(const char* str, int* cat, int* subcat, int* localsubcat);

}

/* vim:set ts=4 sw=4: */
#endif
