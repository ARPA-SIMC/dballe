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

#ifndef DBA_RAWFILE_H
#define DBA_RAWFILE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup io
 * Encapsulates low-level file access.
 *
 * File access is still mainly performed using normal stdio functions, however
 * dba_rawfile adds useful metadata to the normal FILE* stream, such as
 * tracking the file name and counting the number of messages that have been
 * read or written.
 */

#include <dballe/core/msg.h>
#include <stdio.h>

struct _dba_rawfile
{
	char* name;
	FILE* fd;
	int close_on_exit;
	int idx;
};
typedef struct _dba_rawfile* dba_rawfile;

dba_err dba_rawfile_create(dba_rawfile* file, const char* name, const char* mode);
void dba_rawfile_delete(dba_rawfile file);

/**
 * Try to guess the file encoding by peeking at the first byte of the file.
 *
 * The byte read will be put back in the file stream using ungetc.
 * 
 * @param file
 *   The file to scan
 * @retval enc
 *   The guessed encoding
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_rawfile_guess_encoding(dba_rawfile file, dba_encoding* enc);

/*
 * Write the encoded message data to the file
 *
 * @param file
 *   The file to write to
 * @param msg
 *   The ::dba_rawmsg with the encoded data to write
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_rawfile_write(dba_rawfile file, dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
