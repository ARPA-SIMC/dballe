/*
 * dballe/csv - CSV utility functions
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
/*
 * The FormatInt class is Copyright (c) 2012, Victor Zverovich
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dballe/core/csv.h>
#include <dballe/types.h>
#include "dballe/var.h"
#include <wreport/error.h>
#include <cctype>
#include <iostream>
#include <fstream>
#include <limits>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdint.h>

using namespace std;
using namespace wreport;

namespace dballe {

CSVReader::CSVReader() : in(0), close_on_exit(false) {}
CSVReader::CSVReader(std::istream& in) : in(&in), close_on_exit(false) {}
CSVReader::CSVReader(const std::string& pathname)
    : in(0), close_on_exit(false)
{
    open(pathname);
}
CSVReader::~CSVReader() {}

void CSVReader::open(const std::string& pathname)
{
    close();
    close_on_exit = true;
    in = new ifstream(pathname.c_str());
    if (in->fail())
        error_system::throwf("cannot open file %s", pathname.c_str());
}

void CSVReader::close()
{
    if (in && close_on_exit)
        delete in;
    in = 0;
    close_on_exit = true;
}

std::string CSVReader::unescape(const std::string& csvstr)
{
    if (csvstr.empty()) return csvstr;
    if (csvstr[0] != '"') return csvstr;
    if (csvstr.size() == 1) return csvstr;
    string res;
    bool escape = false;
    for (string::const_iterator i = csvstr.begin() + 1; i != csvstr.end(); ++i)
    {
        if (*i == '"')
        {
            if (!escape)
                escape = true;
            else {
                res += *i;
                escape = false;
            }
        } else
            res += *i;
    }
    return res;
}

int CSVReader::as_int(unsigned col) const
{
    if (cols[col].empty())
        error_consistency::throwf("cannot parse a number from column %u, which is empty", col);
    return strtoul(cols[col].c_str(), 0, 10);
}

int CSVReader::as_int_withmissing(unsigned col) const
{
    if (cols[col].empty()) return MISSING_INT;
    return strtoul(cols[col].c_str(), 0, 10);
}

wreport::Varcode CSVReader::as_varcode(unsigned col) const
{
    return resolve_varcode(cols[col]);
}

bool CSVReader::move_to_data(unsigned number_col)
{
    while (true)
    {
        if (number_col < cols.size()
         && (cols[number_col].empty() || isdigit(cols[number_col][0]) || cols[number_col][0] == '-'))
            break;
        if (!next())
            return false;
    }
    return true;
}

int CSVReader::next_char()
{
    int res = in->get();
    if (res == EOF && !in->eof())
        throw error_system("reading a character from CSV input");
    return res;
}

bool CSVReader::next()
{
    if (!in) return false;

    cols.clear();

    // Tokenize the input line
    enum State { BEG, COL, QCOL, EQCOL, HALFEOL } state = BEG;
    string col;
    int c;
    while ((c = next_char()) != EOF)
    {
        switch (state)
        {
            // Look for the beginning of a column value
            case BEG:
                switch (c)
                {
                    case '"':
                        state = QCOL;
                        break;
                    case ',':
                        state = BEG;
                        cols.push_back(col);
                        col.clear();
                        break;
                    case '\r':
                        state = HALFEOL;
                        break;
                    case '\n':
                        cols.push_back(col);
                        return true;
                    default:
                        state = COL;
                        col += c;
                        break;
                }
                break;
            // Inside a column value
            case COL:
                switch (c)
                {
                    case ',':
                        state = BEG;
                        cols.push_back(col);
                        col.clear();
                        break;
                    case '\r':
                        state = HALFEOL;
                        break;
                    case '\n':
                        cols.push_back(col);
                        return true;
                    default:
                        col += c;
                        break;
                }
                break;
            // Inside a quoted column value
            case QCOL:
                switch (c)
                {
                    case '\"':
                        state = EQCOL;
                        break;
                    default:
                        col += c;
                        break;
                }
                break;
            // After a quote character found inside a quoted column value
            case EQCOL:
                switch (c)
                {
                    // The quote marked the end of the value
                    case ',':
                        state = BEG;
                        cols.push_back(col);
                        col.clear();
                        break;
                    case '\r':
                        state = HALFEOL;
                        break;
                    case '\n':
                        cols.push_back(col);
                        return true;
                    // The quote was an escape
                    default:
                        state = QCOL;
                        col += c;
                        break;
                }
                break;
            // After \r was found
            case HALFEOL:
                switch (c)
                {
                    case '\n':
                        cols.push_back(col);
                        return true;
                    default:
                        state = COL;
                        col += '\r';
                        col += c;
                        break;
                }
                break;
        }
    }

    if (state == BEG)
        return false;

    if (!col.empty())
        cols.push_back(col);

    return true;
}

bool csv_read_next(FILE* in, std::vector<std::string>& cols)
{
	char line[2000];
	char* tok;
	char* stringp;

	if (fgets(line, 2000, in) == NULL)
		return false;

	cols.clear();
	for (stringp = line; (tok = strsep(&stringp, ",")) != NULL; )
		cols.push_back(tok);

	return true;
}

void csv_output_quoted_string(ostream& out, const std::string& str)
{
    if (str.find_first_of("\",") != string::npos)
    {
        out << "\"";
        for (string::const_iterator i = str.begin(); i != str.end(); ++i)
        {
            if (*i == '"')
                out << '"';
            out << *i;
        }
        out << "\"";
    } else
        out << str;
}

void csv_output_quoted_string(FILE* out, const std::string& str)
{
    if (str.find_first_of("\",") != string::npos)
    {
        putc('"', out);
        for (string::const_iterator i = str.begin(); i != str.end(); ++i)
        {
            if (*i == '"')
                putc('"', out);
            putc(*i, out);
        }
        putc('"', out);
    } else
        fputs(str.c_str(), out);
}

namespace {

// FormatInt by Victor Zverovich
// See https://github.com/vitaut/format

const char DIGITS[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

class FormatInt {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum {BUFFER_SIZE = std::numeric_limits<uint64_t>::digits10 + 3};
  char buffer_[BUFFER_SIZE];
  char *str_;

  // Formats value in reverse and returns the number of digits.
  char *FormatDecimal(uint64_t value) {
    char *buffer_end = buffer_ + BUFFER_SIZE;
    *--buffer_end = '\0';
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      unsigned index = (value % 100) * 2;
      value /= 100;
      *--buffer_end = DIGITS[index + 1];
      *--buffer_end = DIGITS[index];
    }
    if (value < 10) {
      *--buffer_end = static_cast<char>('0' + value);
      return buffer_end;
    }
    unsigned index = static_cast<unsigned>(value * 2);
    *--buffer_end = DIGITS[index + 1];
    *--buffer_end = DIGITS[index];
    return buffer_end;
  }

 public:
  explicit FormatInt(int value) {
    unsigned abs_value = value;
    bool negative = value < 0;
    if (negative)
      abs_value = 0 - value;
    str_ = FormatDecimal(abs_value);
    if (negative)
      *--str_ = '-';
  }
  explicit FormatInt(unsigned value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(uint64_t value) : str_(FormatDecimal(value)) {}

  unsigned size() const { return buffer_ + BUFFER_SIZE - str_ - 1; }
  const char *c_str() const { return str_; }
  std::string str() const { return str_; }
};

}

CSVWriter::~CSVWriter()
{
}

void CSVWriter::add_value_empty()
{
    if (!row.empty())
        row += ',';
}

void CSVWriter::add_value_raw(const char* str)
{
    if (!row.empty())
        row += ',';
    row.append(str);
}

void CSVWriter::add_value_raw(const std::string& str)
{
    if (!row.empty())
        row += ',';
    row.append(str);
}

void CSVWriter::add_value(uint64_t val)
{
    FormatInt fmt(val);
    add_value_raw(fmt.c_str());
}

void CSVWriter::add_value(unsigned val)
{
    FormatInt fmt(val);
    add_value_raw(fmt.c_str());
}

void CSVWriter::add_value(int val)
{
    FormatInt fmt(val);
    add_value_raw(fmt.c_str());
}

void CSVWriter::add_value_withmissing(int val)
{
    if (val == MISSING_INT)
    {
        if (!row.empty())
            row += ',';
    } else {
        add_value(val);
    }
}

void CSVWriter::add_value(wreport::Varcode val)
{
    if (!row.empty())
        row += ',';

    switch (WR_VAR_F(val))
    {
        case 0: row += 'B'; break;
        case 1: row += 'R'; break;
        case 2: row += 'C'; break;
        case 3: row += 'D'; break;
    }

    unsigned index = WR_VAR_X(val) * 2;
    row += DIGITS[index];
    row += DIGITS[index + 1];

    FormatInt fmt(WR_VAR_Y(val));
    for (unsigned i = 0; i < 3-fmt.size(); ++i)
        row += '0';
    row.append(fmt.c_str());
}

void CSVWriter::add_var_value_raw(const wreport::Var& var)
{
    if (!var.isset())
    {
        add_value_empty();
        return;
    }

    switch (var.info()->type)
    {
        case Vartype::String:
            add_value(var.enqc());
            break;
        case Vartype::Binary:
            // Skip binary variables, that cannot really be encoded in CSV
            add_value_empty();
            break;
        case Vartype::Integer:
        case Vartype::Decimal:
            add_value(var.enqi());
            break;
    }
}

void CSVWriter::add_var_value_formatted(const wreport::Var& var)
{
    if (!var.isset())
    {
        add_value_empty();
        return;
    }

    switch (var.info()->type)
    {
        case Vartype::String:
            add_value(var.enqc());
            break;
        case Vartype::Binary:
            // Skip binary variables, that cannot really be encoded in CSV
            add_value_empty();
            break;
        case Vartype::Integer:
            add_value(var.enqi());
            break;
        case Vartype::Decimal:
            add_value_raw(var.format(""));
            break;
    }
}

void CSVWriter::add_value(const char* val)
{
    if (!row.empty())
        row += ',';
    if (!*val) return;
    row += '"';
    for ( ; *val; ++val)
    {
        if (*val == '"')
            row += '"';
        row += *val;
    }
    row += '"';
}

void CSVWriter::add_value(const std::string& val)
{
    if (!row.empty())
        row += ',';
    if (val.empty()) return;
    row += '"';
    for (string::const_iterator i = val.begin(); i != val.end(); ++i)
    {
        if (*i == '"')
            row += '"';
        row += *i;
    }
    row += '"';
}

}
