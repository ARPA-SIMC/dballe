/*
 * dballe/aoffile - AOF File I/O
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

#ifndef DBA_CORE_AOFFILE_H
#define DBA_CORE_AOFFILE_H

/** @file
 * @ingroup core
 * AOF File I/O
 *
 * This module provides a dballe::File implementation for AOF files.
 */

#include <dballe/core/file.h>
#include <stdint.h>

namespace dballe {
namespace core {

class AofFile : public dballe::core::File
{
protected:
    /**
     * Read a Fortran Unformatted Sequential Binary record from a file
     *
     * @param res
     *   The data read. The resulting data is an array of 32bit words that will
     *   be byteswapped to native endianness if needed.
     * @return true if a record was found, false on EOF
     */
    bool read_record(std::string& res);

    /**
     * Write a Fortran Unformatted Sequential Binary record to a file
     *
     * @param res
     *   The data to be written, considered an array of 32bit words that will be
     *   byteswapped for endianness if needed
     */
    void write_record(const std::string& res);

    /**
     * Write a Fortran Unformatted Sequential Binary record to a file
     *
     * @param words
     *   The data to be written, as an array of 32bit words that will be
     *   byteswapped for endianness if needed
     * @param wordcount
     *   Number of words to write
     */
    void write_record(const uint32_t* words, size_t wordcount);

    /**
     * Write a word to the file, byteswapping it for endianness if needed
     */
    void write_word(uint32_t word);

public:
    AofFile(const std::string& name, FILE* fd, bool close_on_exit=true);
    virtual ~AofFile();

    virtual Encoding encoding() const override { return AOF; }
    BinaryMessage read() override;
    void write(const std::string& msg) override;

    /**
     * Read the file header, perform some consistency checks then discard the
     * data
     */
    void read_header();

    /// Write a dummy file header
    void write_dummy_header();

    /**
     * Rewrite the file header, scanning the file to compute a correct one
     */
    void fix_header();
};

}
}
#endif
