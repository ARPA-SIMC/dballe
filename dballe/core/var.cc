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

/**
 * Format a varcode as fast as possible.
 *
 * It assumes res is at least 7 chars long.
 */
static const char* digits6bit = "00010203040506070809"
                                "10111213141516171819"
                                "20212223242526272829"
                                "30313233343536373839"
                                "40414243444546474849"
                                "50515253545556575859"
                                "60616263";

static const char* digits8bit = "000001002003004005006007008009"
                                "010011012013014015016017018019"
                                "020021022023024025026027028029"
                                "030031032033034035036037038039"
                                "040041042043044045046047048049"
                                "050051052053054055056057058059"
                                "060061062063064065066067068069"
                                "070071072073074075076077078079"
                                "080081082083084085086087088089"
                                "090091092093094095096097098099"
                                "100101102103104105106107108109"
                                "110111112113114115116117118119"
                                "120121122123124125126127128129"
                                "130131132133134135136137138139"
                                "140141142143144145146147148149"
                                "150151152153154155156157158159"
                                "160161162163164165166167168169"
                                "170171172173174175176177178179"
                                "180181182183184185186187188189"
                                "190191192193194195196197198199"
                                "200201202203204205206207208209"
                                "210211212213214215216217218219"
                                "220221222223224225226227228229"
                                "230231232233234235236237238239"
                                "240241242243244245246247248249"
                                "250251252253254255";

static inline void format_xy(wreport::Varcode code, char* buf)
{
    const char* src = digits6bit + (WR_VAR_X(code) & 0x3f) * 2;
    buf[1]          = src[0];
    buf[2]          = src[1];
    src             = digits8bit + (WR_VAR_Y(code) & 0xff) * 3;
    buf[3]          = src[0];
    buf[4]          = src[1];
    buf[5]          = src[2];
    buf[6]          = 0;
}

void format_code(wreport::Varcode code, char* buf)
{
    // Format variable code
    switch (WR_VAR_F(code))
    {
        case 0:  buf[0] = 'B'; break;
        case 1:  buf[0] = 'R'; break;
        case 2:  buf[0] = 'C'; break;
        case 3:  buf[0] = 'D'; break;
        default: buf[0] = '?'; break;
    }
    format_xy(code, buf);
}

void format_bcode(wreport::Varcode code, char* buf)
{
    buf[0] = 'B';
    format_xy(code, buf);
}

void resolve_varlist(const std::string& varlist,
                     std::function<void(wreport::Varcode)> dest)
{
    if (varlist.empty())
        throw error_consistency(
            "cannot parse a Varcode list out of an empty string");

    size_t beg = 0;
    while (true)
    {
        size_t end = varlist.find(',', beg);
        if (end == string::npos)
        {
            dest(resolve_varcode(varlist.substr(beg)));
            break;
        }
        else
        {
            dest(resolve_varcode(varlist.substr(beg, end - beg)));
        }
        beg = end + 1;
    }
}

void resolve_varlist(const std::string& varlist,
                     std::set<wreport::Varcode>& out)
{
    resolve_varlist(varlist, [&](wreport::Varcode code) { out.insert(code); });
}

wreport::Varcode map_code_to_dballe(wreport::Varcode code)
{
    switch (code)
    {
        case WR_VAR(0, 7, 1):   return WR_VAR(0, 7, 30);
        case WR_VAR(0, 10, 3):  return WR_VAR(0, 10, 8);
        case WR_VAR(0, 10, 61): return WR_VAR(0, 10, 60);
        case WR_VAR(0, 12, 1):  return WR_VAR(0, 12, 101);
        case WR_VAR(0, 12, 2):  return WR_VAR(0, 12, 102);
        case WR_VAR(0, 12, 3):  return WR_VAR(0, 12, 103);
        default:                return code;
    }
}

std::unique_ptr<wreport::Var>
var_copy_without_unset_attrs(const wreport::Var& var)
{
    unique_ptr<Var> copy(newvar(var.code()));
    copy->setval(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset())
            continue;
        auto acopy = newvar(map_code_to_dballe(a->code()));
        acopy->setval(*a);
        copy->seta(move(acopy));
    }

    return copy;
}

std::unique_ptr<wreport::Var>
var_copy_without_unset_attrs(const wreport::Var& var, wreport::Varcode code)
{
    unique_ptr<Var> copy(newvar(code));
    copy->setval(var); // Copy value performing conversions

    for (const Var* a = var.next_attr(); a; a = a->next_attr())
    {
        // Skip undefined attributes
        if (!a->isset())
            continue;
        auto acopy = newvar(map_code_to_dballe(a->code()));
        acopy->setval(*a);
        copy->seta(move(acopy));
    }

    return copy;
}

} // namespace dballe
