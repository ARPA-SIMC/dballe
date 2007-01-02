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

#ifndef DBA_FILE_WRITERS_H
#define DBA_FILE_WRITERS_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup io
 * Encapsulates the algorithms used to write raw encoded
 * messages to files containing weather reports.
 */

#include <dballe/core/rawfile.h>
#include <dballe/msg/msg.h>


/**
 * Create a writer for data in BUFR format
 *
 * @retval writer
 *   The new writer, to be deallocated with dba_file_writer_delete()
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_file_writer_create_bufr(dba_file_writer* writer, dba_rawfile file);

/**
 * Specify the template to use for encoding BUFR messages.
 *
 * Note that the template can be changed while encoding, between one message
 * and another.
 *
 * @param writer
 *   The writer to change the parameter for
 * @param type
 *   Message type of the template to use
 * @param subtype
 *   Message subtype of the template to use
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_file_writer_set_bufr_template(dba_file_writer writer, int type, int subtype);

/**
 * Create a writer for data in CREX format
 *
 * @retval writer
 *   The new writer, to be deallocated with dba_file_writer_delete()
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_file_writer_create_crex(dba_file_writer* writer, dba_rawfile file);

/**
 * Specify the template to use for encoding CREX messages.
 *
 * Note that the template can be changed while encoding, between one message
 * and another.
 *
 * @param writer
 *   The writer to change the parameter for
 * @param type
 *   Message type of the template to use
 * @param subtype
 *   Message subtype of the template to use
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_file_writer_set_crex_template(dba_file_writer writer, int type, int subtype);

/**
 * Create a writer for data in AOF format
 *
 * @retval writer
 *   The new writer, to be deallocated with dba_file_writer_delete()
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
dba_err dba_file_writer_create_aof(dba_file_writer* writer, dba_rawfile file);

/**
 * Write a message to the file.
 *
 * @param writer
 *   ::dba_file_writer to use for identifying the message in the file.
 * @param msg
 *   The dba_rawmsg that will hold the data.
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. See @ref error.h
 */
dba_err dba_file_writer_write(dba_file_writer writer, dba_msg msg);

/**
 * Write an encoded message to the file.
 *
 * @param writer
 *   ::dba_file_writer to use for identifying the message in the file.
 * @param msg
 *   The dba_rawmsg that will hold the data.
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. See @ref error.h
 */
dba_err dba_file_writer_write_raw(dba_file_writer writer, dba_rawmsg msg);

/**
 * Delete a dba_file_writer
 *
 * @param writer
 *   The writer to delete.
 */
void dba_file_writer_delete(dba_file_writer writer);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
