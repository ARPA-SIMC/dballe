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
#include "query.h"
#include "dballe/core/record.h"
#include "dballe/msg/context.h"
#include <cstring>
#include <sstream>
#include <iostream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace memdb {

StationValue::~StationValue()
{
    delete var;
}

void StationValue::replace(std::auto_ptr<Var> var)
{
    delete this->var;
    this->var = var.release();
}

void StationValues::clear()
{
    by_station.clear();
    ValueStorage<StationValue>::clear();
}

const StationValue* StationValues::get(const Station& station, wreport::Varcode code) const
{
    const set<size_t>* res = by_station.search(&station);
    if (res)
        for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
        {
            const StationValue* s = (*this)[*i];
            if (s && s->var->code() == code)
                return s;
        }
    return 0;
}

size_t StationValues::insert(const Station& station, std::auto_ptr<Var> var, bool replace)
{
    const set<size_t>* res = by_station.search(&station);
    if (res)
        for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
        {
            StationValue* s = (*this)[*i];
            if (s && s->var->code() == var->code())
            {
                if (!replace)
                    throw error_consistency("cannot replace an existing station value");
                s->replace(var);
                return *i;
            }
        }

    // Value not found, create it
    size_t pos = value_add(new StationValue(station, var));
    // Index it
    by_station[&station].insert(pos);
    // And return it
    return pos;

}

size_t StationValues::insert(const Station& station, const Var& var, bool replace)
{
    auto_ptr<Var> copy(new Var(var));
    return insert(station, copy, replace);
}

bool StationValues::remove(const Station& station, Varcode code)
{
    const set<size_t>* res = by_station.search(&station);
    if (res)
        for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
        {
            const StationValue* s = (*this)[*i];
            if (s && !s->var->code() == code)
            {
                by_station[&station].erase(*i);
                value_remove(*i);
                return true;
            }
        }
    return false;
}

void StationValues::fill_record(const Station& station, Record& rec) const
{
    const set<size_t>* res = by_station.search(&station);
    if (!res) return;

    for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
    {
        const StationValue* s = (*this)[*i];
        rec.set(*s->var);
    }
}

void StationValues::fill_msg(const Station& station, msg::Context& ctx) const
{
    const set<size_t>* res = by_station.search(&station);
    if (!res) return;

    for (set<size_t>::const_iterator i = res->begin(); i != res->end(); ++i)
    {
        const StationValue* s = (*this)[*i];
        ctx.set(*s->var);
    }
}

void StationValues::query(const Record& rec, Results<Station>& stations, Results<StationValue>& res) const
{
    if (!stations.is_select_all())
    {
        trace_query("Adding selected stations to strategy\n");
        match::SequenceBuilder<Station> lookup_by_station(by_station);
        stations.copy_valptrs_to(stl::trivial_inserter(lookup_by_station));
        if (!lookup_by_station.found_items_in_index())
        {
            trace_query(" no matching stations found, setting empty result\n");
            res.set_to_empty();
            return;
        }
        // OR the results together into a single sequence
        res.add_union(lookup_by_station.release_sequences());
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VAR))
    {
        trace_query("Found varcode=%s\n", val);
        res.add(new match::Varcode<StationValue>(descriptor_code(val)));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_VARLIST))
    {
        set<Varcode> codes;
        size_t pos;
        size_t len;
        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
            codes.insert(WR_STRING_TO_VAR(val + pos + 1));
        res.add(new match::Varcodes<StationValue>(codes));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_DATA_FILTER))
    {
        trace_query("Found data_filter=%s\n", val);
        res.add(new match::DataFilter<StationValue>(val));
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ATTR_FILTER))
    {
        trace_query("Found attr_filter=%s\n", val);
        res.add(new match::AttrFilter<StationValue>(val));
    }
}

void StationValues::dump(FILE* out) const
{
    fprintf(out, "Station values:\n");
    for (size_t pos = 0; pos < values.size(); ++pos)
    {
        if (values[pos])
        {
            stringstream buf;
            values[pos]->var->print_without_attrs(buf);

            fprintf(out, " %4zu: %4zu\t%s", pos, values[pos]->station.id, buf.str().c_str());
            // TODO: print attrs
        } else
            fprintf(out, " %4zu: (empty)\n", pos);
    }
#if 0
    fprintf(out, " coord index:\n");
    for (Index<Coord>::const_iterator i = by_coord.begin(); i != by_coord.end(); ++i)
    {
        fprintf(out, "  %d %d -> ", i->first.lat, i->first.lon);
        i->second.dump(out);
        putc('\n', out);
    }
    fprintf(out, " ident index:\n");
    for (Index<string>::const_iterator i = by_ident.begin(); i != by_ident.end(); ++i)
    {
        fprintf(out, "  %s -> \n", i->first.c_str());
        i->second.dump(out);
        putc('\n', out);
    }
#endif
}

template class Index<const Station*>;
template class ValueStorage<StationValue>;

}
}

#include "core.tcc"
#include "query.tcc"
