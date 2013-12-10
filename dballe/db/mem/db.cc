/*
 * dballe/mem/db - Archive for point-based meteorological data, in-memory db
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

#include "db.h"
#include "cursor.h"
#include "dballe/db/modifiers.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <dballe/core/record.h>
#include <dballe/core/defs.h>
#include <dballe/memdb/query.h>
#include <algorithm>
#include <queue>

// #define TRACE_SOURCE
#include "dballe/core/trace.h"

using namespace dballe::memdb;
using namespace std;
using namespace wreport;

namespace dballe {
namespace db {
namespace mem {

DB::~DB()
{
}

void DB::disappear()
{
    memdb.clear();
}

void DB::reset(const char* repinfo_file)
{
    disappear();
    repinfo.load(repinfo_file);
}

void DB::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    repinfo.update(repinfo_file, added, deleted, updated);
}

std::map<std::string, int> DB::get_repinfo_priorities()
{
    return repinfo.get_priorities();
}

void DB::insert(const Record& rec, bool can_replace, bool station_can_add)
{
    // Obtain the station
    m_last_station_id = memdb.stations.obtain(rec, station_can_add);
    const Station& station = *memdb.stations[m_last_station_id];

    // Obtain values
    last_insert_varids.clear();
    if (rec.is_ana_context())
    {
        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        {
            size_t pos = memdb.stationvalues.insert(station, **i, can_replace);
            last_insert_varids.push_back(VarID((*i)->code(), true, pos));
        }
    } else {
        const LevTr& levtr = *memdb.levtrs[memdb.levtrs.obtain(rec)];
        Datetime datetime = rec.get_datetime();

        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        {
            size_t pos = memdb.values.insert(station, levtr, datetime, **i, can_replace);
            last_insert_varids.push_back(VarID((*i)->code(), false, pos));
        }
    }
}

int DB::last_station_id() const
{
    return m_last_station_id;
}

void DB::remove(const Record& query)
{
    memdb.remove(query);
}

void DB::vacuum()
{
    // Nothing to do
}

std::auto_ptr<db::Cursor> DB::query_stations(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    Results<Station> res(memdb.stations);
    memdb.query_stations(query, res);
    return Cursor::createStations(*this, modifiers, res);
}

std::auto_ptr<db::Cursor> DB::query_data(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    bool query_station_vars = (query.get(DBA_KEY_LEVELTYPE1, 0) == 257);
    if (query_station_vars)
    {
        if (modifiers & DBA_DB_MODIFIER_BEST)
        {
            throw error_unimplemented("best queries of station vars");
#warning TODO
        } else {
            Results<StationValue> res(memdb.stationvalues);
            memdb.query_station_data(query, res);
            return Cursor::createStationData(*this, modifiers, res);
        }
    } else {
        if (modifiers & DBA_DB_MODIFIER_BEST)
        {
            throw error_unimplemented("best queries of data vars");
#warning TODO
        } else {
            Results<Value> res(memdb.values);
            memdb.query_data(query, res);
            return Cursor::createData(*this, modifiers, res);
        }
    }
}

std::auto_ptr<db::Cursor> DB::query_summary(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    Results<Value> res(memdb.values);
    memdb.query_data(query, res);
    return Cursor::createSummary(*this, modifiers, res);
}

unsigned DB::query_attrs(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs, Record& attrs)
{
    Value& val = *memdb.values[id_data];
    unsigned res = 0;
    if (qcs.empty())
    {
        for (const Var* a = val.var->next_attr(); a != NULL; a = a->next_attr())
        {
            attrs.set(*a);
            ++res;
        }
    } else {
        for (const Var* a = val.var->next_attr(); a != NULL; a = a->next_attr())
            if (std::find(qcs.begin(), qcs.end(), a->code()) != qcs.end())
            {
                attrs.set(*a);
                ++res;
            }
    }
    return res;
}

void DB::attr_insert(wreport::Varcode id_var, const Record& attrs)
{
    // Find the data id for id_var
    for (vector<VarID>::const_iterator i = last_insert_varids.begin();
            i != last_insert_varids.end(); ++i)
        if (i->code == id_var)
        {
            attr_insert(i->id, id_var, attrs);
            return;
        }
    error_notfound::throwf("variable B%02d%03d was not involved in the last insert operation", WR_VAR_X(id_var), WR_VAR_Y(id_var));
}

void DB::attr_insert(int id_data, wreport::Varcode id_var, const Record& attrs)
{
    Value& val = *memdb.values[id_data];
    for (vector<Var*>::const_iterator i = attrs.vars().begin(); i != attrs.vars().end(); ++i)
        val.var->seta(**i);
}

void DB::attr_remove(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs)
{
    Value& val = *memdb.values[id_data];
    for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
        val.var->unseta(*i);
}

void DB::dump(FILE* out)
{
    fprintf(out, "repinfo data:\n");
    repinfo.dump(out);
    memdb.dump(out);
}

void DB::import_msg(const Msg& msg, const char* repmemo, int flags)
{
    memdb.insert(msg,
            flags | DBA_IMPORT_OVERWRITE,
            flags | DBA_IMPORT_FULL_PSEUDOANA,
            flags | DBA_IMPORT_ATTRS,
            repmemo);
}

namespace {

struct CompareForExport
{
    // Return an inverse comparison, so that the priority queue gives us the
    // smallest items first
    bool operator() (const Value* x, const Value* y) const
    {
        // Compare station and report
        if (x->station.id < y->station.id) return false;
        if (x->station.id > y->station.id) return true;
        // Compare datetime
        return x->datetime > y->datetime;
    }
};

}

void DB::export_msgs(const Record& query, MsgConsumer& cons)
{
    Results<Value> res(memdb.values);
    memdb.query_data(query, res);

    // Sorted value IDs
    priority_queue<const Value*, vector<const Value*>, CompareForExport> values;
    res.copy_valptrs_to(stl::pusher(values));

    TRACE("export_msgs: %zd values in priority queue\n", values.size());

    // Message being built
    auto_ptr<Msg> msg;

    // Last value seen, used to detect when we can move on to the next message
    const Value* old_val = 0;

    // Iterate all results, sorted
    for ( ; !values.empty(); values.pop())
    {
        const Value* val = values.top();
        TRACE("Got %zd %04d-%02d-%02d %02d:%02d:%02d B%02d%03d %d,%d, %d,%d %d,%d,%d %s\n",
                val->station.id,
                (int)val->datetime.date.year, (int)val->datetime.date.month, (int)val->datetime.date.day,
                (int)val->datetime.time.hour, (int)val->datetime.time.minute, (int)val->datetime.time.second,
                WR_VAR_X(val->var->code()), WR_VAR_Y(val->var->code()),
                val->levtr.level.ltype1, val->levtr.level.l1, val->levtr.level.ltype2, val->levtr.level.l2,
                val->levtr.trange.pind, val->levtr.trange.p1, val->levtr.trange.p2,
                val->var->value());

        // See if we have the start of a new message
        if (!old_val || old_val->station.id != val->station.id ||
                old_val->datetime != val->datetime)
        {
            // Flush current message
            TRACE("New message\n");
            if (msg.get() != NULL)
            {
                //TRACE("Sending old message to consumer\n");
                if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
                {
                    auto_ptr<Msg> copy(new Msg);
                    msg->sounding_pack_levels(*copy);
                    cons(copy);
                } else
                    cons(msg);
            }

            // Start writing a new message
            msg.reset(new Msg);

            // Fill station info
            msg::Context& c_st = val->station.fill_msg(*msg);

            // Fill station vars
            memdb.stationvalues.fill_msg(val->station, c_st);

            // Fill datetime
            c_st.set_year(val->datetime.date.year);
            c_st.set_month(val->datetime.date.month);
            c_st.set_day(val->datetime.date.day);
            c_st.set_hour(val->datetime.time.hour);
            c_st.set_minute(val->datetime.time.minute);
            c_st.set_second(val->datetime.time.second);

            // Update last value seen info
            old_val = val;
        }

        TRACE("Inserting var B%02d%03d (%s)\n", WR_VAR_X(val->var->code()), WR_VAR_Y(val->var->code()), val->var->value());
        msg::Context& ctx = msg->obtain_context(val->levtr.level, val->levtr.trange);
        ctx.set(*val->var);
    }

    if (msg.get() != NULL)
    {
        TRACE("Inserting leftover old message\n");
        if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
        {
            auto_ptr<Msg> copy(new Msg);
            msg->sounding_pack_levels(*copy);
            cons(copy);
        } else
            cons(msg);
    }
}

}
}
}

#include "dballe/memdb/core.tcc"
#include "dballe/memdb/query.tcc"

/* vim:set ts=4 sw=4: */
