/*
 * memdb/memdb - In-memory indexed storage of DB-All.e data
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

#include "memdb.h"
#include <dballe/core/record.h>
#include <dballe/msg/msg.h>

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {

void Memdb::insert_or_replace(const Record& rec)
{
    const Station& station = stations.obtain(rec);
    if (rec.is_ana_context())
    {
        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        {
            stationvalues.insert_or_replace(station, **i);
#if 0
            if (can_replace)
                d.insert_or_overwrite(true);
            else
                d.insert_or_fail(true);
            last_insert_varids.push_back(VarID((*i)->code(), d.id));
#endif
        }
    } else {
        const LevTr& levtr = levtrs.obtain(rec);
        Datetime datetime = rec.get_datetime();

        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        {
            values.insert_or_replace(station, levtr, datetime, **i);
#if 0
            if (can_replace)
                d.insert_or_overwrite(true);
            else
                d.insert_or_fail(true);
            last_insert_varids.push_back(VarID((*i)->code(), d.id));
#endif
        }
    }
}

void Memdb::insert_or_replace(const Msg& msg)
{
}

}
