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

#ifndef DBALLE_RAWMSG_H
#define DBALLE_RAWMSG_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup io
 * In-memory storage of an encoded weather report.
 */

#include <dballe/core/error.h>

/**
 * Supported encodings for report data
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2
} dba_encoding;

/**
 * Return a string with the name of the given encoding
 *
 * @param enc
 *   The encoding to name.
 * @return
 *   A short name for the encoding, such as "BUFR", "CREX", "AOF" or
 *   "(unknown)".  
 */
const char* dba_encoding_name(dba_encoding enc);

struct _dba_file;

/**
 * Dynamic storage for encoded messages.
 */
struct _dba_rawmsg {
	/**
	 * File where the dba_rawmsg has been read.  It can be NULL when not
	 * applicable, such as when the message is created from scratch and not yet
	 * written
	 */
	struct _dba_file* file;
	/** Start offset of this message inside the file where it is found */
	int offset;
	/** Index of the message within the source */
	int index;

	/** Buffer with the raw encoded data */
	unsigned char* buf;
	/** Length of the raw encoded data */
	int len;
	/** Length of the allocated memory for buf */
	int alloclen;

	/** Encoding of the raw data */
	dba_encoding encoding;
};
/** @copydoc _dba_rawmsg */
typedef struct _dba_rawmsg* dba_rawmsg;

/**
 * Create a new ::dba_rawmsg.
 *
 * \retval msg
 *   The new ::dba_rawmsg.  It will need to be deallocated using
 *   dba_rawmsg_delete()
 * \returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_rawmsg_create(dba_rawmsg* msg);

/**
 * Delete a ::dba_rawmsg
 * 
 * \param msg
 *   The ::dba_rawmsg to delete
 * \returns
 *   The error indicator for the function.  See @ref error.h
 */
void dba_rawmsg_delete(dba_rawmsg msg);

/**
 * Clear all the contents of this ::dba_rawmsg
 * 
 * \param msg
 *   The ::dba_rawmsg to clear
 */
void dba_rawmsg_reset(dba_rawmsg msg);

/**
 * Set the internal buffer to `buf', taking ownership of the buffer.
 */
dba_err dba_rawmsg_acquire_buf(dba_rawmsg msg, unsigned char* buf, int size);

/**
 * Get the raw (encoded) representation of the message.
 *
 * \param msg
 *   The message to query
 * \retval buf
 *   A const pointer to the internal buffer with the encoded representation of
 *   the message.
 * \retval size
 *   The size of the data in buf
 * \returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_rawmsg_get_raw(dba_rawmsg msg, const unsigned char** buf, int* size);

/**
 * Expand the size of the buffer holding message data.
 *
 * Allocation is optimised for the normal message size of meteorological data:
 * if the buffer size is below 16Kbytes, then the size is doubled; when above
 * 16Kb, then it is grown by 2Kb.
 *
 * \param msg
 *   The message to enlarge
 * \returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_rawmsg_expand_buffer(dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
