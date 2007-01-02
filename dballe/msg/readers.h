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

#ifndef DBA_FILE_READERS_H
#define DBA_FILE_READERS_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup io
 * Encapsulates the algorithms used to extract raw encoded
 * messages from files containing weather reports.
 */

#include <dballe/core/rawfile.h>

struct _dba_file_reader;
typedef struct _dba_file_reader* dba_file_reader;

typedef void (*dba_file_reader_delete_fun)(dba_file_reader);
typedef dba_err (*dba_file_reader_read_fun)(dba_file_reader reader, dba_rawmsg msg, int* found);

/**
 * Encapsulates decoder logic for specific message types
 */
struct _dba_file_reader
{
	/** Function to use to delete this decoder */
	dba_file_reader_delete_fun delete_fun;

	/** Function to use to read a message */
	dba_file_reader_read_fun read_fun;

	/** ::dba_rawfile object to use for reading */
	dba_rawfile file;
};


/**
 * Create a reader for data in BUFR format
 *
 * @retval reader
 *   The new reader, to be deallocated with dba_file_reader_delete()
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err dba_file_reader_create_bufr(dba_file_reader* reader, dba_rawfile file);

/**
 * Create a reader for data in CREX format
 *
 * @retval reader
 *   The new reader, to be deallocated with dba_file_reader_delete()
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err dba_file_reader_create_crex(dba_file_reader* reader, dba_rawfile file);

/**
 * Create a reader for data in AOF format
 *
 * @retval reader
 *   The new reader, to be deallocated with dba_file_reader_delete()
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err dba_file_reader_create_aof(dba_file_reader* reader, dba_rawfile file);

/**
 * Read a message from the file.
 *
 * @param reader
 *   ::dba_file_reader to use for identifying the message in the file.
 * @param msg
 *   The dba_rawmsg that will hold the data.
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_reader_read(dba_file_reader reader, dba_rawmsg msg, int* found);

/**
 * Delete a dba_file_reader
 *
 * @param reader
 *   The reader to delete.
 */
void dba_file_reader_delete(dba_file_reader reader);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
