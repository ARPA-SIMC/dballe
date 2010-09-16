/*
 * msg/defs - Common definitions
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "defs.h"

#include <iostream>

using namespace std;

namespace dballe {

std::ostream& operator<<(std::ostream& out, const Level& l)
{
    if (l.ltype1 == MISSING_INT) out << "-"; else out << l.ltype1;
    out << ",";
    if (l.l1 == MISSING_INT) out << "-"; else out << l.l1;
    out << ",";
    if (l.ltype2 == MISSING_INT) out << "-"; else out << l.ltype2;
    out << ",";
    if (l.l2 == MISSING_INT) out << "-"; else out << l.l2;
    return out;
}

std::ostream& operator<<(std::ostream& out, const Trange& l)
{
    if (l.pind == MISSING_INT) out << "-"; else out << l.pind;
    out << ",";
    if (l.p1 == MISSING_INT) out << "-"; else out << l.p1;
    out << ",";
    if (l.p2 == MISSING_INT) out << "-"; else out << l.p2;
    return out;
}

} // namespace dballe

/* vim:set ts=4 sw=4: */
