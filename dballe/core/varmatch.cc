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
#include "dballe/var.h"
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
        if (!var.isset()) return false;
        int ival = var.enqi();
        return min <= ival && ival <= max;
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
        if (!var.isset()) return false;
        string sval = var.enqs();
        return min <= sval && sval <= max;
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
        if (!var.isset()) return false;
        return op(var.enq<T>(), val);
    }
};

template<typename T>
static unique_ptr<Varmatch> make_op(Varcode code, const std::string& op, const T& val)
{
    if (op == "<")
        return unique_ptr<Varmatch>(new Op< T, less<T> >(code, val));
    else if (op == "<=")
        return unique_ptr<Varmatch>(new Op< T, less_equal<T> >(code, val));
    else if (op == ">")
        return unique_ptr<Varmatch>(new Op< T, greater<T> >(code, val));
    else if (op == ">=")
        return unique_ptr<Varmatch>(new Op< T, greater_equal<T> >(code, val));
    else if (op == "=" || op == "==")
        return unique_ptr<Varmatch>(new Op< T, equal_to<T> >(code, val));
    else if (op == "<>")
        return unique_ptr<Varmatch>(new Op< T, not_equal_to<T> >(code, val));
    else
        error_consistency::throwf("cannot understand comparison operator '%s'", op.c_str());
}

}

std::unique_ptr<Varmatch> Varmatch::parse(const std::string& filter)
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
        Varcode code = resolve_varcode(filter.substr(sep1_end, sep2_begin - sep1_end).c_str());
        Varinfo info = varinfo(code);
        string min = filter.substr(0, sep1_begin);
        string max = filter.substr(sep2_end);

        switch (info->type)
        {
            case Vartype::String:
                return unique_ptr<Varmatch>(new varmatch::BetweenString(code, min, max));
            case Vartype::Binary:
                error_consistency::throwf("cannot apply filter '%s' to a binary variable", filter.c_str());
            case Vartype::Integer:
            case Vartype::Decimal:
            {
                int imin = info->encode_decimal(strtod(min.c_str(), NULL));
                int imax = info->encode_decimal(strtod(max.c_str(), NULL));
                return unique_ptr<Varmatch>(new varmatch::BetweenInt(code, imin, imax));
            }
        }
        error_consistency::throwf("unsupported variable type %d", (int)info->type);
    } else {
        // B12345<=>val
        Varcode code = resolve_varcode(filter.substr(0, sep1_begin).c_str());
        Varinfo info = varinfo(code);
        string op = filter.substr(sep1_begin, sep1_end - sep1_begin);

        switch (info->type)
        {
            case Vartype::String:
                return varmatch::make_op(code, op, filter.substr(sep1_end));
            case Vartype::Binary:
                error_consistency::throwf("cannot apply filter '%s' to a binary variable", filter.c_str());
            case Vartype::Integer:
            case Vartype::Decimal:
            {
                int val = info->encode_decimal(strtod(filter.substr(sep1_end).c_str(), NULL));
                return varmatch::make_op(code, op, val);
            }
        }
        error_consistency::throwf("unsupported variable type %d", (int)info->type);
    }
}

}
