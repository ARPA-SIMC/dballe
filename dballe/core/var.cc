/*
 * dballe/var - DB-All.e specialisation of wreport variable
 *
 * Copyright (C) 2005,2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "var.h"
#include "aliases.h"
#include <wreport/vartable.h>

using namespace wreport;
using namespace std;

namespace dballe {

const Vartable* local = NULL;

wreport::Varinfo varinfo(wreport::Varcode code)
{
	if (local == NULL)
		local = Vartable::get("dballe");
	return local->query(code);
}

wreport::Varinfo varinfo(const char* code)
{
    if (local == NULL)
        local = Vartable::get("dballe");
    return local->query(resolve_varcode_safe(code));
}

void format_code(wreport::Varcode code, char* buf)
{
    // Format variable code
    char type;
    switch (WR_VAR_F(code))
    {
        case 0: type = 'B'; break;
        case 1: type = 'R'; break;
        case 2: type = 'C'; break;
        case 3: type = 'D'; break;
        default: type = '?'; break;
    }
    snprintf(buf, 7, "%c%02d%03d", type, WR_VAR_X(code), WR_VAR_Y(code));
}

std::string format_code(wreport::Varcode code)
{
    char buf[8];
    format_code(code, buf);
    return buf;
}

wreport::Varcode resolve_varcode(const char* name)
{
    wreport::Varcode res = 0;
    if ((res = varcode_alias_resolve(name)) == 0)
        res = descriptor_code(name);
    return res;
}

wreport::Varcode resolve_varcode_safe(const char* name)
{
    if (!name)
        throw error_consistency("cannot parse a Varcode out of a NULL");
    if (!name[0])
        throw error_consistency("cannot parse a Varcode out of an empty string");

    // Try looking up among aliases
    Varcode res = varcode_alias_resolve(name);
    if (res) return res;

    if (name[0] != 'B')
        error_consistency::throwf("cannot parse a Varcode out of '%s'", name);

    // Ensure that B is followed by 5 integers
    for (unsigned i = 1; i < 6; ++i)
        if (name[i] and !isdigit(name[i]))
            error_consistency::throwf("cannot parse a Varcode out of '%s'", name);

    return WR_STRING_TO_VAR(name + 1);
}

wreport::Varcode resolve_varcode_safe(const std::string& name)
{
    if (name.empty())
        throw error_consistency("cannot parse a Varcode out of an empty string");

    // Try looking up among aliases
    Varcode res = varcode_alias_resolve(name);
    if (res) return res;

    if (name[0] != 'B')
        error_consistency::throwf("cannot parse a Varcode out of '%s'", name.c_str());

    // Ensure that B is followed by 5 integers
    for (unsigned i = 1; i < 6; ++i)
        if (name[i] and !isdigit(name[i]))
            error_consistency::throwf("cannot parse a Varcode out of '%s'", name.c_str());

    return WR_STRING_TO_VAR(name.data() + 1);
}

void resolve_varlist_safe(const std::string& varlist, std::function<void(wreport::Varcode)> dest)
{
    if (varlist.empty())
        throw error_consistency("cannot parse a Varcode list out of an empty string");

    size_t beg = 0;
    while (true)
    {
        size_t end = varlist.find(',', beg);
        if (end == string::npos)
        {
            dest(resolve_varcode_safe(varlist.substr(beg)));
            break;
        } else {
            dest(resolve_varcode_safe(varlist.substr(beg, end-beg)));
        }
        beg = end + 1;
    }
}

void resolve_varlist_safe(const std::string& varlist, std::set<wreport::Varcode>& out)
{
    resolve_varlist_safe(varlist, [&](wreport::Varcode code) { out.insert(code); });
}

wreport::Varcode map_code_to_dballe(wreport::Varcode code)
{
    switch (code)
    {
        case WR_VAR(0,  7,  1): return WR_VAR(0,  7,  30);
        case WR_VAR(0, 10,  3): return WR_VAR(0, 10,   8);
        case WR_VAR(0, 10, 61): return WR_VAR(0, 10,  60);
        case WR_VAR(0, 12,  1): return WR_VAR(0, 12, 101);
        case WR_VAR(0, 12,  2): return WR_VAR(0, 12, 102);
        case WR_VAR(0, 12,  3): return WR_VAR(0, 12, 103);
        default: return code;
    }
}

std::unique_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var)
{
    unique_ptr<Var> copy(newvar(var.code()));
    copy->copy_val_only(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset()) continue;
        auto_ptr<Var> acopy(newvar(map_code_to_dballe(a->code())).release());
        acopy->copy_val_only(*a);
        copy->seta(acopy);
    }

    return copy;
}

std::unique_ptr<wreport::Var> var_copy_without_unset_attrs(
        const wreport::Var& var, wreport::Varcode code)
{
    unique_ptr<Var> copy(newvar(code));
    copy->copy_val_only(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset()) continue;
        auto_ptr<Var> acopy(newvar(map_code_to_dballe(a->code())).release());
        acopy->copy_val_only(*a);
        copy->seta(acopy);
    }

    return copy;
}

}

/* vim:set ts=4 sw=4: */
