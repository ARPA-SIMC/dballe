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

#ifndef DBA_CORE_FILE_H
#define DBA_CORE_FILE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * File I/O for files containing meterorological messages.
 *
 * This module provides a unified interface to read and write messages to files
 * in various formats.
 *
 * Format-specific implementation is not provided by this module, but other
 * libraries can implement format specific read and write functions and
 * register them with ::dba_file.
 */

#include <dballe/core/rawmsg.h>

struct _dba_file;
/**
 * Opaque structure representing a file with meteorological data
 */
typedef struct _dba_file* dba_file;

/**
 * Create a dba_file structure.
 *
 * @param type
 *   The type of data contained in the file.  If -1 is passed, then
 *   dba_file_create will attempt to autodetect the file type from its first
 *   byte.
 * @param name
 *   The name of the file to access
 * @param mode
 *   The opening mode of the file (@see fopen)
 * @retval file
 *   The new file, to be deallocated with dba_file_delete()
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err dba_file_create(dba_encoding type, const char* name, const char* mode, dba_file* file);

/**
 * Delete a dba_file.
 *
 * @param file
 *   The file to delete.
 */
void dba_file_delete(dba_file file);

/**
 * Get the type of the dba_file
 *
 * @param file
 *   The dba_file to query.
 * @return 
 *   The file encoding.
 */
dba_encoding dba_file_type(dba_file file);

/**
 * Get the name of the dba_file
 *
 * @param file
 *   The dba_file to query.
 * @return 
 *   The file name.
 */
const char* dba_file_name(dba_file file);

/**
 * Read a message from the file.
 *
 * @param file
 *   ::dba_file to read from
 * @param msg
 *   The dba_rawmsg that will hold the data.
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_read(dba_file file, dba_rawmsg msg, int* found);

/**
 * Write the encoded message data to the file
 *
 * @param file
 *   The ::dba_file to write to
 * @param msg
 *   The ::dba_rawmsg with the encoded data to write
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_write(dba_file file, dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
