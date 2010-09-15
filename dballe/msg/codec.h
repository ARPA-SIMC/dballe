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

namespace import {

struct Options
{
    bool simplified;

    /// Create new Options initialised with default values
    Options()
        : simplified(false) {}

    /// Print a summary of the options to \a out
    void print(FILE* out);
};

} // namespace import

/**
 * Message importer
 *
 * This class is designed like a configurable virtual functor.
 *
 * Importers of various kinds can provide their implementations.
 */ 
class Importer
{
protected:
    import::Options opts;

public:
    Importer(const import::Options& opts);
    virtual ~Importer();

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
    virtual void import(const Rawmsg& msg, Msgs& msgs) const = 0;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void import_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const = 0;


    /// Instantiate the right importer for the given type
    static std::auto_ptr<Importer> create(Encoding type, const import::Options& opts=import::Options());

#if 0
	/**
	 * Read and parse a message from the file.
	 *
	 * @param file
	 *   ::dba_file to read from
	 * @retval msgs
	 *   The resulting ::dba_msgs
	 * @retval found
	 *   Will be set to true if a message has been found in the file, else to false.
	 * @return
	 *   The error indicator for the function. See @ref error.h
	 */
	dba_err read(dba_file file, dba_msgs* msgs, int* found);
#endif

#if 0
	/**
	 * Dump all the decoder options to a stream
	 *
	 * @param out
	 *   The stream to dump to.
	 */
	void print(FILE* out);
#endif
};

#if 0

class Encoder
{
public:
	Encoder();

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
	dba_err encode(dba_msgs msgs, dba_encoding type, dba_rawmsg *rmsg);

	/**
	 * Write a message to the file.
	 *
	 * @param file
	 *   ::dba_file to write to
	 * @param msgs
	 *   The ::dba_msgs to encode and write.
	 * @param cat
	 *   The BUFR of CREX message category to use for encoding (0 for auto detect)
	 * @param subcat
	 *   The BUFR of CREX message subcategory to use for encoding (0 for auto detect)
	 * @param localsubcat
	 *   The BUFR of CREX message subcategory (defined by local centres) to use for encoding (0 for auto detect)
	 * @return
	 *   The error indicator for the function. See @ref error.h
	 */
	dba_err write(dba_file file, dba_msgs msgs, int cat, int subcat, int localsubcat);
};

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
