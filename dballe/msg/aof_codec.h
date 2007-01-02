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

#ifndef DBALLE_AOF_CODEC_H
#define DBALLE_AOF_CODEC_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup aof
 *
 * AOF message codec.
 *
 * It provides various AOF encoding and decoding functions, and implements
 * dba_file reading and writing of AOF files.
 *
 * AOF records can be read, written and interpreted into a dba_msg.  Encoding
 * from a dba_msg is not yet implemented.  A "makeaof" tool exists, not part of
 * DB-All.e, that can convert BUFR messages into AOF.
 *
 * Endianness of the written records can be controlled by the environment
 * variable DBA_AOF_ENDIANNESS:
 *
 * \li \b ARCH writes using the host endianness
 * \li \n LE writes using little endian
 * \li \n BE writes using big endian
 *
 * If the environment variable is not set, the default is to write using the
 * host endianness.
 */

#include <dballe/core/rawmsg.h>
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
#include <stdint.h>

/**
 * Decode an AOF message
 *
 * @param msg
 *   The aof_message with the data to decode
 * @retval out
 *   The decoded message
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err aof_codec_decode(dba_rawmsg msg, dba_msgs* msgs);

/**
 * Get category and subcategory of an AOF message
 */
dba_err aof_codec_get_category(dba_rawmsg msg, int* category, int* subcategory);

/**
 * Encode an AOF message
 *
 * @param msg
 *   The aof_message with the data to encode
 * @param in
 *   The message to encode
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
/*
dba_err aof_message_encode(aof_message msg, dba_msg in);
*/

/**
 * Print the contents of the AOF message
 *
 * @param msg
 *   The encoded message to dump
 * @param out
 *   The stream to use to print the message
 */
void aof_codec_dump(dba_rawmsg msg, FILE* out);

/**
 * Read a fortran "unformatted sequential" record with an array of 32-bit words.
 *
 * @param file
 *   The file to read from.
 * @retval rec
 *   The contents of the "unformatted sequential" record read.  It will need to
 *   be deallocated by the caller.
 * @retval len
 *   The length of the returned rec record.
 * @return
 *   The error indicator for the function. @see dba_err.
 */
dba_err aof_codec_read_record(dba_file file, uint32_t** rec, int* len);

/* Write a fortran "unformatted sequential" record contained in an array of
 * 32-bit words.
 *
 * @param file
 *   The file to write to.
 * @param rec
 *   The data to write.
 * @param len
 *   The number of elements in the rec array.
 * @return
 *   The error indicator for the function. @see dba_err.
 */
dba_err aof_codec_write_record(dba_file file, const uint32_t* rec, int len);

/**
 * Read the header of an aof file.
 *
 * @param file
 *   The file to read from.
 * @retval fdr
 *   The contents of the First Data Record.  It will need to be deallocated by
 *   the caller.
 * @retval fdr_len
 *   The length of the fdr array.
 * @retval ddr
 *   The contents of the Data Description Record.  It will need to be
 *   deallocated by the caller.
 * @retval ddr_len
 *   The length of the ddr array.
 * @return
 *   The error indicator for the function. @see dba_err.
 */
dba_err aof_codec_read_header(dba_file file, uint32_t** fdr, int* fdr_len, uint32_t** ddr, int* ddr_len);

/**
 * Writes a dummy header to the AOF file.
 *
 * This function is used to write a file header before having any information
 * on the data that will be written afterwards.  The dummy header can be
 * completed later using aof_codec_fix_header().
 *
 * @param file
 *   The file to write the header to.
 * @return
 *   The error indicator for the function. @see dba_err.
 */
dba_err aof_codec_write_dummy_header(dba_file file);

/**
 * Fix the header of an AOF file.
 *
 * Currently it just recomputes the start and end of the observation period by
 * looking at the observation headers for all the observations found in the
 * file.
 *
 * @param file
 *   The file to operate on.  It must be open in read-write mode and be a
 *   seekable file.
 * @return
 *   The error indicator for the function. @see dba_err.
 */
dba_err aof_codec_fix_header(dba_file file);
	

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
