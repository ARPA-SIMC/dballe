/*
 * dballe/aof_codec - AOF import
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

#ifndef DBALLE_AOF_CODEC_H
#define DBALLE_AOF_CODEC_H

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

#include <dballe/msg/codec.h>
#include <stdint.h>

namespace dballe {
struct Msg;

namespace msg {

class AOFImporter : public Importer
{
protected:
    // Message-specific code
    static void read_synop(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_flight(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_satob(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_dribu(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_temp(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_pilot(const uint32_t* obs, int obs_len, Msg& msg);
    static void read_satem(const uint32_t* obs, int obs_len, Msg& msg);

    /// Parse WMO block and station numbers in the Observation Header
    static void parse_st_block_station(const uint32_t* obs, Msg& msg);
    /// Parse station altitude the Observation Header
    static void parse_altitude(const uint32_t* obs, Msg& msg);
    /// Parse string ident in the Observation Header
    static void parse_st_ident(const uint32_t* obs, Msg& msg);
    /// Parse latitude, longitude, date and time in the Observation Header
    static void parse_lat_lon_datetime(const uint32_t* obs, Msg& msg);
    /// Parse 27 Weather group in Synop observations
    static void parse_weather_group(const uint32_t* obs, Msg& msg);
    /// Parse 28 General cloud group in Synop observations
    static void parse_general_cloud_group(const uint32_t* obs, Msg& msg);
    /// Parse a bit-packed cloud group in Synop observations
    static void parse_cloud_group(uint32_t val, int* ns, int* c, int* h);


public:
    AOFImporter(const Options& opts=Options());
    virtual ~AOFImporter();

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
    virtual void from_rawmsg(const Rawmsg& msg, Msgs& msgs) const;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void from_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const;

    /**
     * Get category and subcategory of an AOF message
     *
     * @param msg
     *   The message to scan
     * @retval category
     *   The AOF category of the message
     * @retval subcategory
     *   The AOF subcategory of the message
     */
    static void get_category(const Rawmsg& msg, int* category, int* subcategory);

    /**
     * Print the contents of the AOF message
     *
     * @param msg
     *   The encoded message to dump
     * @param out
     *   The stream to use to print the message
     */
    static void dump(const Rawmsg& msg, FILE* out);
};

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
