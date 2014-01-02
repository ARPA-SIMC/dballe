/*
 * Copyright (C) 2011--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "test-utils-core.h"
#include "csv.h"

#include <sstream>
#include <iostream>

using namespace std;
using namespace dballe;
using namespace wibble::tests;

namespace tut {

struct core_csv_shar {
};
TESTGRP(core_csv);

// Test CSV string escaping
template<> template<>
void to::test<1>()
{
    stringstream s;
    csv_output_quoted_string(s, "");
    ensure_equals(s.str(), "");
    ensure_equals(CSVReader::unescape(s.str()), "");

    s.str(std::string());
    csv_output_quoted_string(s, "1");
    ensure_equals(s.str(), "1");
    ensure_equals(CSVReader::unescape(s.str()), "1");

    s.str(std::string());
    csv_output_quoted_string(s, "12");
    ensure_equals(s.str(), "12");
    ensure_equals(CSVReader::unescape(s.str()), "12");

    s.str(std::string());
    csv_output_quoted_string(s, "123");
    ensure_equals(s.str(), "123");
    ensure_equals(CSVReader::unescape(s.str()), "123");

    s.str(std::string());
    csv_output_quoted_string(s, ",");
    ensure_equals(s.str(), "\",\"");
    ensure_equals(CSVReader::unescape(s.str()), ",");

    s.str(std::string());
    csv_output_quoted_string(s, "antani, blinda");
    ensure_equals(s.str(), "\"antani, blinda\"");
    ensure_equals(CSVReader::unescape(s.str()), "antani, blinda");

    s.str(std::string());
    csv_output_quoted_string(s, "antani, \"blinda\"");
    ensure_equals(s.str(), "\"antani, \"\"blinda\"\"\"");
    ensure_equals(CSVReader::unescape(s.str()), "antani, \"blinda\"");

    s.str(std::string());
    csv_output_quoted_string(s, "\"");
    ensure_equals(s.str(), "\"\"\"\"");
    ensure_equals(CSVReader::unescape(s.str()), "\"");

    s.str(std::string());
    csv_output_quoted_string(s, "\",\"");
    ensure_equals(s.str(), "\"\"\",\"\"\"");
    ensure_equals(CSVReader::unescape(s.str()), "\",\"");

    ensure_equals(CSVReader::unescape("\""), "\"");
    ensure_equals(CSVReader::unescape("\"\""), "");
    ensure_equals(CSVReader::unescape("\"\"\""), "\"");
    ensure_equals(CSVReader::unescape("\"\"\"\""), "\"");
    ensure_equals(CSVReader::unescape("\"\"\"\"\""), "\"\"");
    ensure_equals(CSVReader::unescape("a\"b"), "a\"b");
}

// Test CSV reader
template<> template<>
void to::test<2>()
{
    {
        stringstream in("");
        CSVReader reader(in);
        ensure(!reader.next());
    }

    {
        stringstream in("\n");
        CSVReader reader(in);
        ensure(reader.next());
        wassert(actual(reader.cols.size()) == 1);
        wassert(actual(reader.cols[0]) == "");
        ensure(!reader.next());
    }

    {
        stringstream in("\r\n");
        CSVReader reader(in);
        ensure(reader.next());
        wassert(actual(reader.cols.size()) == 1);
        wassert(actual(reader.cols[0]) == "");
        ensure(!reader.next());
    }

    {
        stringstream in("1,2\r\n");
        CSVReader reader(in);
        ensure(reader.next());
        ensure_equals(reader.cols.size(), 2u);
        ensure_equals(reader.cols[0], "1");
        ensure_equals(reader.cols[1], "2");
        ensure(!reader.next());
    }

    {
        stringstream in("1\r\n2\r\n");
        CSVReader reader(in);
        ensure(reader.next());
        ensure_equals(reader.cols.size(), 1u);
        ensure_equals(reader.cols[0], "1");
        ensure(reader.next());
        ensure_equals(reader.cols.size(), 1u);
        ensure_equals(reader.cols[0], "2");
        ensure(!reader.next());
    }

    {
        stringstream in("1,2\n");
        CSVReader reader(in);
        ensure(reader.next());
        ensure_equals(reader.cols.size(), 2u);
        ensure_equals(reader.cols[0], "1");
        ensure_equals(reader.cols[1], "2");
        ensure(!reader.next());
    }

    {
        stringstream in(
                "1,\",\",2\n"
                "antani,,blinda\n"
                ",\n"
        );
        CSVReader reader(in);

        ensure(reader.next());
        ensure_equals(reader.cols.size(), 3u);
        ensure_equals(reader.cols[0], "1");
        ensure_equals(reader.cols[1], ",");
        ensure_equals(reader.cols[2], "2");

        ensure(reader.next());
        ensure_equals(reader.cols.size(), 3u);
        ensure_equals(reader.cols[0], "antani");
        ensure_equals(reader.cols[1], "");
        ensure_equals(reader.cols[2], "blinda");

        ensure(reader.next());
        ensure_equals(reader.cols.size(), 2u);
        ensure_equals(reader.cols[0], "");
        ensure_equals(reader.cols[1], "");

        ensure(!reader.next());
    }

    {
        stringstream in("1,2");
        CSVReader reader(in);

        ensure(reader.next());
        ensure_equals(reader.cols.size(), 2u);
        ensure_equals(reader.cols[0], "1");
        ensure_equals(reader.cols[1], "2");

        ensure(!reader.next());
    }

    {
        stringstream in("\"1\"\r\n");
        CSVReader reader(in);

        ensure(reader.next());
        ensure_equals(reader.cols.size(), 1u);
        ensure_equals(reader.cols[0], "1");

        ensure(!reader.next());
    }
}

namespace {

class TestCSVWriter : public CSVWriter
{
public:
    stringstream buf;
    virtual void flush_row()
    {
        buf << row;
    }
};

}

// Test write/read cycles
template<> template<>
void to::test<3>()
{
    TestCSVWriter out;
    out.add_value(1);
    out.add_value("\"");
    out.add_value("'");
    out.add_value(",");
    out.add_value("\n");
    out.flush_row();

    out.buf.seekg(0);
    CSVReader in(out.buf);
    wassert(actual(in.next()).istrue());
    wassert(actual(in.cols.size()) == 5);
    wassert(actual(in.as_int(0)) == 1);
    wassert(actual(in.cols[1]) == "\"");
    wassert(actual(in.cols[2]) == "'");
    wassert(actual(in.cols[3]) == ",");
    wassert(actual(in.cols[4]) == "\n");
    wassert(actual(in.next()).isfalse());
}

}

// vim:set ts=4 sw=4:
