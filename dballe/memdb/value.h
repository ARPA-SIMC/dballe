/*
 * memdb/value - In memory representation of a variable with metadata and
 *               attributes
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

#ifndef DBA_MEMDB_VALUE_H
#define DBA_MEMDB_VALUE_H

#include <dballe/core/defs.h>
#include <wreport/var.h>

namespace dballe {
namespace memdb {

    /*
struct StationValue
{
    Station* station;
    wreport::Var var;
};

struct Levtr
{
    Level level;
    Trange trange;
};

struct Value
{
    Station* station;
    Levtr* levtr;
    int datetime[6];
    wreport::Var var;
};
*/

}
}

#endif
