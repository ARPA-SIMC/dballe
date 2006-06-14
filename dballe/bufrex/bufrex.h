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

#ifndef DBALLE_CREX_H
#define DBALLE_CREX_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * High level encoding and decoding functions
 */

#include <dballe/io/dba_rawmsg.h>
#include <dballe/msg/dba_msg.h>

/**
 * Decode a BUFR message into a dba_msg
 *
 * @param raw
 *   The message to decode
 * @retval msg
 *   The decoded message
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_decode_bufr(dba_rawmsg raw, dba_msg* msg);

/**
 * Decode a CREX message into a dba_msg
 *
 * @param raw
 *   The message to decode
 * @retval msg
 *   The decoded message
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_decode_crex(dba_rawmsg raw, dba_msg* msg);

/**
 * Encode a dba_msg into a BUFR message
 *
 * @param msg
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_encode_bufr(dba_msg msg, int type, int subtype, dba_rawmsg* raw);

/**
 * Encode a dba_msg into a CREX message
 *
 * @param msg
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_encode_crex(dba_msg msg, int type, int subtype, dba_rawmsg* raw);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
