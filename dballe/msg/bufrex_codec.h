/*
 * dballe/bufrex_codec - BUFR/CREX import and export
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <dballe/msg/codec.h>
#include <stdint.h>

namespace dballe {
struct Msg;

namespace msg {

class BufrCrexImporter : public Importer
{
public:
    BufrCrexImporter(const import::Options& opts);

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void import_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const;
};

class BufrImporter : public BufrCrexImporter
{
public:
    BufrImporter(const import::Options& opts=import::Options());
    virtual ~BufrImporter();

    virtual void import(const Rawmsg& msg, Msgs& msgs) const;
};

class CrexImporter : public BufrCrexImporter
{
public:
    CrexImporter(const import::Options& opts=import::Options());
    virtual ~CrexImporter();

    virtual void import(const Rawmsg& msg, Msgs& msgs) const;
};

#if 0

/** @file
 * @ingroup bufrex
 * High level encoding and decoding functions
 */

#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#include <dballe/bufrex/msg.h>

/**
 * Encode a dba_msg into a BUFR message
 *
 * @param msgs
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, int localsubtype, dba_rawmsg* raw);

/**
 * Encode a dba_msg into a CREX message
 *
 * @param msgs
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_encode_crex(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw);


/**
 * Fill in the bufrex_msg with the contents of a dba_msg
 *
 * @param raw
 *   The bufrex message that will be filled with the data from msg
 * @param msg
 *   The dba_msg to take the data from
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg);

/**
 * Fill in the bufrex_msg with the contents of a dba_msgs
 *
 * @param raw
 *   The bufrex message that will be filled with the data from msg
 * @param msgs
 *   The dba_msgs to take the data from
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs);

/**
 * Fill in a dba_msgs with the contents of the bufrex_msg
 *
 * @param raw
 *   The bufrex message with the data to interpret
 * @param opts
 *   Codec options
 * @retval msgs
 *   The dba_msgs with the interpreted data
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_to_dba_msgs(bufrex_msg raw, dba_msg_codec_options opts, dba_msgs* msgs);


/**
 * Infer good type and subtype from a dba_msg
 *
 * @param msg
 *   The message for which to infer the encoding template
 * @retval type
 *   The type of the encoding template to use
 * @retval subtype
 *   The subtype of the encoding template to use
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype, int* localsubtype);
	

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
