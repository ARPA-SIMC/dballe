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

#include <wreport/var.h>
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

    /**
     * Return the given column, as an integer.
     *
     * A missing value is returned as MISSING_INT.
     */
    int as_int_withmissing(unsigned col) const;

    /**
     * Find the first line where the given column exists and starts with a
     * number.
     *
     * This can be used to skip titles and empty lines, moving to the start of
     * the real data. Real data is identified by using a column that starts
     * with text in the headers and number in the data.
     *
     * @returns true if a data line has been found, false if we reached EOF
     */
    bool move_to_data(unsigned number_col=0);

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

class CSVWriter
{
protected:
    std::string row;

public:
    virtual ~CSVWriter();

    /// Add a value to the current row, without any escaping
    void add_value_raw(const char* str);

    /// Add a value to the current row, without any escaping
    void add_value_raw(const std::string& str);

    /// Add an int value to the current row
    void add_value(int val);

    /// Add an int value that can potentially be missing
    void add_value_withmissing(int val);

    /// Add an int value to the current row
    void add_value(unsigned val);

    /// Add an int value to the current row
    void add_value(size_t val);

    /// Add an int value to the current row
    void add_value(wreport::Varcode val);

    /// Add a variable value
    void add_var_value(const wreport::Var& val);

    /// Add a string to the current row
    void add_value(const char* val);

    /// Add a string to the current row
    void add_value(const std::string& val);

    /// Write the current line to the output file, and start a new one
    virtual void flush_row() = 0;
};


}

/* vim:set ts=4 sw=4: */
#endif
