/*
 * dballe/rawmsg - annotated raw buffer
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

#ifndef DBALLE_RAWMSG_H
#define DBALLE_RAWMSG_H

#include <string>

namespace dballe {

/** @file
 * @ingroup io
 * In-memory storage of an encoded weather report.
 */

/**
 * Supported encodings
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2,
} Encoding;

/**
 * Return a string with the name of the given encoding
 *
 * @param enc
 *   The encoding to name.
 * @return
 *   A short name for the encoding, such as "BUFR", "CREX", "AOF" or
 *   "(unknown)".  
 */
const char* encoding_name(Encoding enc);

struct File;

/**
 * Dynamic storage for encoded messages.
 */
struct Rawmsg : public std::string
{
	/**
	 * File where the dba_rawmsg has been read.  It can be NULL when not
	 * applicable, such as when the message is created from scratch and not yet
	 * written
	 */
	const File* file;
	/** Start offset of this message inside the file where it is found */
	int offset;
	/** Index of the message within the source */
	int index;

	/** Encoding of the raw data */
	Encoding encoding;

	Rawmsg();
	~Rawmsg();

	// Return the file name from which this message was read
	std::string filename() const throw ();

	// Clear all the contents of this dballe::Rawmsg
	void clear() throw ();
};

}

/* vim:set ts=4 sw=4: */
#endif
