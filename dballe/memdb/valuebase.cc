/*
 * memdb/valuebase - Common implementation for StationValue and Value
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "valuebase.h"
#include "dballe/core/record.h"
#include <algorithm>

using namespace wreport;
using namespace std;

namespace dballe {
namespace memdb {

ValueBase::~ValueBase()
{
    delete var;
}

void ValueBase::replace(std::auto_ptr<Var> var)
{
    delete this->var;
    this->var = var.release();
}

void ValueBase::replace(const Var& var)
{
    this->var->copy_val_only(var);
}

unsigned ValueBase::query_attrs(const std::vector<wreport::Varcode>& qcs, Record& attrs) const
{
    attrs.clear();
    unsigned res = 0;
    if (qcs.empty())
    {
        for (const Var* a = var->next_attr(); a != NULL; a = a->next_attr())
        {
            attrs.set(*a);
            ++res;
        }
    } else {
        for (const Var* a = var->next_attr(); a != NULL; a = a->next_attr())
            if (std::find(qcs.begin(), qcs.end(), a->code()) != qcs.end())
            {
                attrs.set(*a);
                ++res;
            }
    }
    return res;
}

void ValueBase::attr_insert(const Record& attrs)
{
    for (vector<Var*>::const_iterator i = attrs.vars().begin(); i != attrs.vars().end(); ++i)
        var->seta(**i);
}

void ValueBase::attr_remove(const std::vector<wreport::Varcode>& qcs)
{
    // FIXME: if qcs is empty, remove all?
    if (qcs.empty())
    {
        var->clear_attrs();
    } else {
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            var->unseta(*i);
    }
}

}
}
