/*
 * core/varmatch - Variable matcher
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "varmatch.h"
#include "dballe/core/var.h"
#include <functional>
#include <cstdlib>
#include <iostream>

using namespace std;
using namespace wreport;

namespace dballe {

Varmatch::Varmatch(wreport::Varcode code)
    : code(code) {}

bool Varmatch::operator()(const wreport::Var& var) const
{
    return var.code() == code;
}

namespace varmatch {

struct BetweenInt : public Varmatch
{
    int min, max;
    BetweenInt(Varcode code, int min, int max)
        : Varmatch(code), min(min), max(max) {}
    bool operator()(const wreport::Var& var) const
    {
        if (!Varmatch::operator()(var)) return false;
        if (const char* val = var.value())
        {
            int ival = strtoul(val, 0, 10);
            return min <= ival && ival <= max;
        }
        return false;
    }
};

struct BetweenString : public Varmatch
{
    string min, max;
    BetweenString(Varcode code, const std::string& min, const std::string& max)
        : Varmatch(code), min(min), max(max) {}
    bool operator()(const wreport::Var& var) const
    {
        if (!Varmatch::operator()(var)) return false;
        if (const char* val = var.value())
        {
            string sval(val);
            return min <= sval && sval <= max;
        }
        return false;
    }
};

namespace {
template<typename T>
static T from_raw_var(const char* val) { throw error_unimplemented("cannot get value for this type"); }
template<> int from_raw_var<int>(const char* val) { return strtoul(val, 0, 10); }
template<> string from_raw_var<string>(const char* val) { return val; }
}

template<typename T, typename OP>
struct Op : public Varmatch
{
    OP op;
    T val;
    Op(Varcode code, const T& val) : Varmatch(code), val(val) {}
    bool operator()(const wreport::Var& var) const
    {
        if (!Varmatch::operator()(var)) return false;
        if (const char* v = var.value())
            return op(from_raw_var<T>(v), val);
        return false;
    }
};

template<typename T>
static auto_ptr<Varmatch> make_op(Varcode code, const std::string& op, const T& val)
{
    if (op == "<")
        return auto_ptr<Varmatch>(new Op< T, less<T> >(code, val));
    else if (op == "<=")
        return auto_ptr<Varmatch>(new Op< T, less_equal<T> >(code, val));
    else if (op == ">")
        return auto_ptr<Varmatch>(new Op< T, greater<T> >(code, val));
    else if (op == ">=")
        return auto_ptr<Varmatch>(new Op< T, greater_equal<T> >(code, val));
    else if (op == "=" || op == "==")
        return auto_ptr<Varmatch>(new Op< T, equal_to<T> >(code, val));
    else if (op == "<>")
        return auto_ptr<Varmatch>(new Op< T, not_equal_to<T> >(code, val));
    else
        error_consistency::throwf("cannot understand comparison operator '%s'", op.c_str());
}

}

std::auto_ptr<Varmatch> Varmatch::parse(const std::string& filter)
{
    size_t sep1_begin = filter.find_first_of("<=>");
    if (sep1_begin == string::npos)
        error_consistency::throwf("cannot find any operator in filter '%s'", filter.c_str());

    size_t sep1_end = filter.find_first_not_of("<=>", sep1_begin);
    if (sep1_end == string::npos)
        error_consistency::throwf("cannot find end of first operator in filter '%s'", filter.c_str());
    size_t sep2_begin = filter.find_first_of("<=>", sep1_end);
    if (sep2_begin != string::npos)
    {
        // min<=B12345<=max
        size_t sep2_end = filter.find_first_not_of("<=>", sep2_begin);
        if (sep2_end == string::npos)
            error_consistency::throwf("cannot find end of second operator in filter '%s'", filter.c_str());
        Varcode code = resolve_varcode_safe(filter.substr(sep1_end, sep2_begin - sep1_end).c_str());
        Varinfo info = varinfo(code);
        string min = filter.substr(0, sep1_begin);
        string max = filter.substr(sep2_end);

        if (info->is_string())
            return auto_ptr<Varmatch>(new varmatch::BetweenString(code, min, max));
        else
        {
            int imin = info->encode_int(strtod(min.c_str(), NULL));
            int imax = info->encode_int(strtod(max.c_str(), NULL));
            return auto_ptr<Varmatch>(new varmatch::BetweenInt(code, imin, imax));
        }
    } else {
        // B12345<=>val
        Varcode code = resolve_varcode_safe(filter.substr(0, sep1_begin).c_str());
        Varinfo info = varinfo(code);
        string op = filter.substr(sep1_begin, sep1_end - sep1_begin);

        if (info->is_string())
            return varmatch::make_op(code, op, filter.substr(sep1_end));
        else {
            int val = info->encode_int(strtod(filter.substr(sep1_end).c_str(), NULL));
            return varmatch::make_op(code, op, val);
        }
    }
}

}
