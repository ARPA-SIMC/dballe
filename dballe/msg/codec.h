/*
 * msg/codec - General codec options
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

#ifndef DBA_MSG_CODEC_H
#define DBA_MSG_CODEC_H

#if 0
#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#endif

#include <dballe/core/rawmsg.h>
#include <memory>
#include <stdio.h>

/** @file
 * @ingroup msg
 * General codec options
 */

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct Rawmsg;
struct Msgs;

namespace msg {

/**
 * Message importer
 *
 * This class is designed like a configurable virtual functor.
 *
 * Importers of various kinds can provide their implementations.
 */
class Importer
{
public:
    struct Options
    {
        bool simplified;

        /// Create new Options initialised with default values
        Options()
            : simplified(false) {}

        /// Print a summary of the options to \a out
        void print(FILE* out);
    };

protected:
    Options opts;

public:
    Importer(const Options& opts);
    virtual ~Importer();

    /**
     * Decode a message from its raw encoded representation
     *
     * @param rmsg
     *   Encoded message
     * @retval msgs
     *   The resulting ::dba_msg
     */
    virtual void from_rawmsg(const Rawmsg& msg, Msgs& msgs) const = 0;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void from_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const = 0;


    /// Instantiate the right importer for the given type
    static std::auto_ptr<Importer> create(Encoding type, const Options& opts=Options());
};

/**
 * Message exporter
 *
 * This class is designed like a configurable virtual functor.
 *
 * Exporters of various kinds can provide their implementations.
 */
class Exporter
{
public:
    struct Options
    {
        /// Name of template to use for output (leave empty to autodetect)
        std::string template_name;
        /// Originating centre
        int centre;
        /// Originating subcentre
        int subcentre;
        /// Originating application ID
        int application;

        /// Create new Options initialised with default values
        Options()
            : centre(255), subcentre(255), application(255) {}

        /// Print a summary of the options to \a out
        void print(FILE* out);
    };

protected:
    Options opts;

public:
    Exporter(const Options& opts);
    virtual ~Exporter();

    /**
     * Encode a message
     *
     * @param msgs
     *   Message to encode
     * @retval rmsg
     *   The resulting Rawmsg
     */
    virtual void to_rawmsg(const Msgs& msgs, Rawmsg& msg) const = 0;

    /**
     * Export to a Bulletin
     */
    virtual void to_bulletin(const Msgs& msgs, wreport::Bulletin& msg) const = 0;


    /// Instantiate the right importer for the given type
    static std::auto_ptr<Exporter> create(Encoding type, const Options& opts=Options());
};

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
