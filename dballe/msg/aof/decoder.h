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

#ifndef DBALLE_AOF_DECODER_H
#define DBALLE_AOF_DECODER_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup aof
 *
 * AOF message decoder
 */

#include <dballe/io/dba_rawmsg.h>
#include <dballe/msg/dba_msgs.h>

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
dba_err aof_decoder_decode(dba_rawmsg msg, dba_msgs* msgs);

/**
 * Get category and subcategory of an AOF message
 */
dba_err aof_decoder_get_category(dba_rawmsg msg, int* category, int* subcategory);

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
void aof_decoder_dump(dba_rawmsg msg, FILE* out);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
