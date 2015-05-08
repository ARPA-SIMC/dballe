/*
 * json - JSON writer
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_CORE_JSON_H
#define DBALLE_CORE_JSON_H

#include <vector>
#include <string>

namespace dballe {

/**
 * JSON serializer
 *
 * It is called with a sequence of sax-like events, and appends the resulting
 * JSON to a string.
 *
 * The JSON output is all in one line, so that end of line can be used as
 * separator between distinct JSON records.
 */
class JSONWriter
{
protected:
    enum State {
        LIST_FIRST,
        LIST,
        MAPPING_KEY_FIRST,
        MAPPING_KEY,
        MAPPING_VAL,
    };
    std::string& out;
    std::vector<State> stack;

    /// Append whatever separator is needed (if any) before a new value
    void val_head();

    void jputc(char c);
    void jputs(const char* s);

public:
    JSONWriter(std::string& out);
    ~JSONWriter();

    void start_list();
    void end_list();

    void start_mapping();
    void end_mapping();

    void add_null();
    void add_bool(bool val);
    void add_int(int val);
    void add_double(double val);
    void add_cstring(const char* val);
    void add_string(const std::string& val);

    void add(const std::string& val) { add_string(val); }
    void add(const char* val) { add_cstring(val); }
    void add(double val) { add_double(val); }
    void add(int val) { add_int(val); }
    void add(bool val) { add_bool(val); }

    // Shortcut to add a mapping, which also ensures that the key is a string
    template<typename T>
    void add(const char* a, T b)
    {
        add_cstring(a);
        add(b);
    }
};

}
#endif
