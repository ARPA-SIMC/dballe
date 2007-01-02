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

#ifndef DBALLE_BUFREX_H
#define DBALLE_BUFREX_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * High level encoding and decoding functions
 */

#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/bufrex/msg.h>

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
dba_err bufrex_decode_bufr(dba_rawmsg raw, dba_msgs* msgs);

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
dba_err bufrex_decode_crex(dba_rawmsg raw, dba_msgs* msgs);

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
dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw);

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
dba_err bufrex_encode_crex(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw);


/**
 * Fill in the bufrex_msg with the contents of a dba_msg
 */
dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg);

/**
 * Fill in the bufrex_msg with the contents of a dba_msgs
 */
dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs);

/**
 * Fill in a dba_msgs with the contents of the bufrex_msg
 */
dba_err bufrex_msg_to_dba_msgs(bufrex_msg raw, dba_msgs* msgs);


/**
 * Infer good type and subtype from a dba_msg
 */
dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype);
	

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
