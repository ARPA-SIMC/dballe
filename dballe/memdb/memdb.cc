/*
 * memdb/memdb - In-memory indexed storage of DB-All.e data
 *
 * Copyright (C) 2013--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "results.h"
#include "dballe/core/record.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {

namespace memdb {

bool SummaryContext::operator<(const SummaryContext& c) const
{
    if (sample.station < c.sample.station) return true;
    if (sample.station > c.sample.station) return false;
    if (sample.levtr < c.sample.levtr) return true;
    if (sample.levtr > c.sample.levtr) return false;
    return sample.var->code() < c.sample.var->code();
}

void SummaryStats::extend(const Datetime& dt)
{
    if (count == 0)
    {
        dtmin = dtmax = dt;
    } else {
        if (dt < dtmin)
            dtmin = dt;
        else if (dt > dtmax)
            dtmax = dt;
    }
    ++count;
}

void Summarizer::insert(const Value* val)
{
    SummaryContext ctx(*val);
    memdb::Summary::iterator out = summary.find(ctx);
    if (out == summary.end())
        summary.insert(make_pair(ctx, memdb::SummaryStats(val->datetime)));
    else
        out->second.extend(val->datetime);
}

}

Memdb::Memdb()
{
}

void Memdb::clear()
{
    values.clear();
    levtrs.clear();
    stationvalues.clear();
    stations.clear();
}

void Memdb::insert_or_replace(const Record& rec)
{
    const Station& station = *stations[stations.obtain(rec)];
    if (rec.is_ana_context())
    {
        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
            stationvalues.insert(station, **i);
    } else {
        const LevTr& levtr = *levtrs[levtrs.obtain(rec)];
        Datetime datetime = rec.get_datetime();

        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
            values.insert(station, levtr, datetime, **i);
    }
}

void Memdb::insert(const Msg& msg, bool replace, bool with_station_info, bool with_attrs, const char* force_report)
{
    const msg::Context* l_ana = msg.find_context(Level::ana(), Trange());
    if (!l_ana)
        throw error_consistency("cannot import into the database a message without station information");

    // Coordinates
    Coords coord;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        coord.lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        coord.lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Report code
    string report;
    if (force_report != NULL)
        report = force_report;
    else if (const Var* var = msg.get_rep_memo_var())
        report = var->value();
    else
        report = Msg::repmemo_from_type(msg.type);

    size_t station_id;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
    {
        // Mobile station
        station_id = stations.obtain_mobile(coord, var->value(), report, true);
    }
    else
    {
        // Fixed station
        station_id = stations.obtain_fixed(coord, report, true);
    }

    const Station& station = *stations[station_id];

    if (with_station_info || stationvalues.has_variables_for(station))
    {
        // Insert the rest of the station information
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;

            unique_ptr<Var> var(new Var(*l_ana->data[i], with_attrs));
            stationvalues.insert(station, std::move(var), replace);
        }
    }

    // Fill up the common context information for the rest of the data

    // Date and time
    if (msg.datetime().is_missing())
        throw error_notfound("date/time informations not found (or incomplete) in message to insert");

    // Insert the rest of the data
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        const msg::Context& ctx = *msg.data[i];
        bool is_ana_level = ctx.level == Level::ana() && ctx.trange == Trange();
        // Skip the station info level
        if (is_ana_level) continue;

        size_t levtr_id = levtrs.obtain(ctx.level, ctx.trange);
        const LevTr& levtr = *levtrs[levtr_id];

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var& var = *ctx.data[j];
            if (not var.isset()) continue;

            unique_ptr<Var> newvar(new Var(var, with_attrs));
            values.insert(station, levtr, msg.datetime(), std::move(newvar), replace);
        }
    }
}

size_t Memdb::insert(
        const Coords& coords, const std::string& ident, const std::string& report,
        const Level& level, const Trange& trange, const Datetime& datetime,
        std::unique_ptr<wreport::Var> var)
{
    size_t id_station;
    if (ident.empty())
        id_station = stations.obtain_fixed(coords, report, true);
    else
        id_station = stations.obtain_mobile(coords, ident, report, true);
    const Station& station = *stations[id_station];

    size_t id_levtr = levtrs.obtain(level, trange);
    const LevTr& levtr = *levtrs[id_levtr];

    return values.insert(station, levtr, datetime, std::move(var));
}

size_t Memdb::insert(
        const Coords& coords, const std::string& ident, const std::string& report,
        const Level& level, const Trange& trange, const Datetime& datetime,
        const Var& var)
{
    unique_ptr<Var> newvar(new Var(var));
    return insert(coords, ident, report, level, trange, datetime, std::move(newvar));
}

void Memdb::remove(Results<Value>& res)
{
#warning TODO: check if ana context, in which case remove from station index
    res.copy_indices_to(stl::eraser(values));
}

void Memdb::dump(FILE* out) const
{
    stations.dump(out);
    stationvalues.dump(out);
    levtrs.dump(out);
    values.dump(out);
}

}

#include "results.tcc"
