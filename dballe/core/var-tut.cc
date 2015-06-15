/*
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

#include "test-utils-core.h"
#include "var.h"

using namespace dballe;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct core_var_shar
{
};
TESTGRP(core_var);

template<> template<>
void to::test<1>()
{
    {
        set<Varcode> codes;
        resolve_varlist("B12101", codes);
        wassert(actual(codes.size()) == 1);
        wassert(actual(*codes.begin()) == WR_VAR(0, 12, 101));
    }

    {
        set<Varcode> codes;
        resolve_varlist("B12101,B12103", codes);
        wassert(actual(codes.size()) == 2);
        set<Varcode>::const_iterator i = codes.begin();
        wassert(actual(*i) == WR_VAR(0, 12, 101));
        ++i;
        wassert(actual(*i) == WR_VAR(0, 12, 103));
    }
}

}

