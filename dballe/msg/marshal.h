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

#ifndef DBA_MARSHAL_H
#define DBA_MARSHAL_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Message marshalling and unmarshalling
 */

#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>

/**
 * Decode a message from its raw encoded representation
 *
 * @param rmsg
 *   Encoded message
 * @retval msgs
 *   The resulting ::dba_msg
 * @return
 *   The error indicator for the function. See @ref error.h
 */
dba_err dba_marshal_decode(dba_rawmsg rmsg, dba_msgs *msgs);

/**
 * Encode a message into its raw encoded representation
 *
 * @param msgs
 *   Message to encode
 * @param type
 *   Format to use for encoding
 * @retval rmsg
 *   The resulting ::dba_rawmsg
 * @return
 *   The error indicator for the function. See @ref error.h
 */
dba_err dba_marshal_encode(dba_msgs msgs, dba_encoding type, dba_rawmsg *rmsg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
