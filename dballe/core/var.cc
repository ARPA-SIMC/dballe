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

wreport::Varcode map_code_to_dballe(wreport::Varcode code)
{
    switch (code)
    {
        case WR_VAR(0,  7,  1): return WR_VAR(0,  7,  30);
        case WR_VAR(0, 10,  3): return WR_VAR(0, 10,   8);
        case WR_VAR(0, 10, 61): return WR_VAR(0, 10,  60);
        case WR_VAR(0, 12,  1): return WR_VAR(0, 12, 101);
        case WR_VAR(0, 12,  3): return WR_VAR(0, 12, 103);
        default: return code;
    }
}

std::auto_ptr<wreport::Var> var_copy_without_unset_attrs(const wreport::Var& var)
{
    auto_ptr<Var> copy(newvar(var.code()));
    copy->copy_val_only(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset()) continue;
        auto_ptr<Var> acopy(newvar(map_code_to_dballe(a->code())));
        acopy->copy_val_only(*a);
        copy->seta(acopy);
    }

    return copy;
}

std::auto_ptr<wreport::Var> var_copy_without_unset_attrs(
        const wreport::Var& var, wreport::Varcode code)
{
    auto_ptr<Var> copy(newvar(code));
    copy->copy_val_only(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset()) continue;
        auto_ptr<Var> acopy(newvar(map_code_to_dballe(a->code())));
        acopy->copy_val_only(*a);
        copy->seta(acopy);
    }

    return copy;
}

}

/* vim:set ts=4 sw=4: */
