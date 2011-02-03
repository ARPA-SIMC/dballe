/*
 * dballe/csv - CSV utility functions
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_CSV_H
#define DBA_CSV_H

/** @file
 * @ingroup core
 * Routines to parse data in CSV format
 */

#include <vector>
#include <string>
#include <iosfwd>
#include <stdio.h>

namespace dballe
{

/**
 * Parse a CSV line.
 *
 * @param in
 *   The file where to read from
 * @param cols
 *   The parsed columns.
 * @return
 *   true if a new line was found, else false
 */
bool csv_read_next(FILE* in, std::vector<std::string>& cols);

class CSVReader
{
protected:
    /// Read one line from the input source, returning false if EOF is reached
    virtual bool nextline() = 0;

public:
    /// Last line read
    std::string line;

    /// Parsed CSV columns for the last line read
    std::vector<std::string> cols;

    virtual ~CSVReader();

    /// Read the next CSV line, returning false if EOF is reached
    bool next();

    static std::string unescape(const std::string& csvstr);
};

class IstreamCSVReader : public CSVReader
{
protected:
    virtual bool nextline();

public:
    std::istream& in;

    IstreamCSVReader(std::istream& in) : in(in) {}
};

// TODO: CSV readers allowing to peek on the next line without consuming it, to
// allow the Msg parser to stop at msg boundary after peeking at a line
// also, stripping newlines at end of lines
// also, reading from istream
// also, de-escaping strings (if they start with quote)

/**
 * Output a string value, quoted if needed according to CSV rules
 */
void csv_output_quoted_string(std::ostream& out, const std::string& str);

}

/* vim:set ts=4 sw=4: */
#endif
