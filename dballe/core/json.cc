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

#include "json.h"
#include <cctype>
#include <cmath>

using namespace std;

namespace dballe {

JSONWriter::JSONWriter(std::string& out) : out(out) {}
JSONWriter::~JSONWriter() {}

void JSONWriter::val_head()
{
    if (!stack.empty())
    {
        switch (stack.back())
        {
            case LIST_FIRST: stack.back() = LIST; break;
            case LIST: out += ','; break;
            case MAPPING_KEY_FIRST: stack.back() = MAPPING_VAL; break;
            case MAPPING_KEY: out += ','; stack.back() = MAPPING_VAL; break;
            case MAPPING_VAL: out += ':'; stack.back() = MAPPING_KEY; break;
        }
    }
}

void JSONWriter::add_null()
{
    val_head();
    out += "null";
}

void JSONWriter::add_bool(bool val)
{
    val_head();
    out += val ? "true" : "false";
}

void JSONWriter::add_int(int val)
{
    val_head();
    out += to_string(val);
}

void JSONWriter::add_double(double val)
{
    val_head();

    double vint, vfrac;
    vfrac = modf(val, &vint);
    if (vfrac == 0.0)
    {
        out += to_string((int)vint);
        out += ".0";
    }
    else
        out += to_string(val);
}

void JSONWriter::add_cstring(const char* val)
{
    val_head();
    out += '"';
    for ( ; *val; ++val)
        switch (*val)
        {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '/': out += "\\/"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += *val; break;
        }
    out += '"';
}

void JSONWriter::add_string(const std::string& val)
{
    add_cstring(val.c_str());
}

void JSONWriter::start_list()
{
    val_head();
    out += '[';
    stack.push_back(LIST_FIRST);
}

void JSONWriter::end_list()
{
    out += ']';
    stack.pop_back();
}

void JSONWriter::start_mapping()
{
    val_head();
    out += '{';
    stack.push_back(MAPPING_KEY_FIRST);
}

void JSONWriter::end_mapping()
{
    out += '}';
    stack.pop_back();
}

}
