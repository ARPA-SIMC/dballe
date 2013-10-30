/*
 * memdb/stationvalue - In memory representation of station values
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
#include "stationvalue.h"
#include "station.h"
#include <iostream>

using namespace std;

namespace dballe {
namespace memdb {

StationValue::~StationValue()
{
    delete var;
}

void StationValue::replace(std::auto_ptr<wreport::Var> var)
{
    delete this->var;
    this->var = var.release();
}

const StationValue& StationValues::insert_or_replace(const Station& station, std::auto_ptr<wreport::Var> var)
{
    Positions res = by_station.search(&station);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && get(*i)->var->code() == var->code())
        {
            get(*i)->replace(var);
            return *get(*i);
        }

    // Station not found, create it
    size_t pos = value_add(new StationValue(station, var));
    // Index it
    by_station[&station].insert(pos);
    // And return it
    return *get(pos);

}

bool StationValues::remove(const Station& station, wreport::Varcode code)
{
    Positions res = by_station.search(&station);
    for (Positions::const_iterator i = res.begin(); i != res.end(); ++i)
        if (get(*i) && !get(*i)->var->code() == code)
        {
            by_station[&station].erase(*i);
            value_remove(*i);
            return true;
        }
    return false;
}

template class Index<const Station*>;

}
}

