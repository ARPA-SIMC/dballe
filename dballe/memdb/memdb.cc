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
#include "query.h"
#include "dballe/core/record.h"
#include "dballe/core/varmatch.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"

using namespace std;
using namespace wreport;
using namespace dballe::memdb;

namespace dballe {

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
        {
            stationvalues.insert(station, **i);
#if 0
            if (can_replace)
                d.insert_or_overwrite(true);
            else
                d.insert_or_fail(true);
            last_insert_varids.push_back(VarID((*i)->code(), d.id));
#endif
        }
    } else {
        const LevTr& levtr = *levtrs[levtrs.obtain(rec)];
        Datetime datetime = rec.get_datetime();

        // Insert all the variables we find
        for (vector<Var*>::const_iterator i = rec.vars().begin(); i != rec.vars().end(); ++i)
        {
            values.insert(station, levtr, datetime, **i);
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

void Memdb::insert(const Msg& msg, bool replace, bool with_station_info, bool with_attrs, const char* force_report)
{
    const msg::Context* l_ana = msg.find_context(Level(257), Trange());
    if (!l_ana)
        throw error_consistency("cannot import into the database a message without station information");

    // Coordinates
    Coord coord;
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

            auto_ptr<Var> var(new Var(*l_ana->data[i], with_attrs));
            stationvalues.insert(station, var, replace);
        }
    }

    // Fill up the common context information for the rest of the data

    // Date and time
    const Var* year = l_ana->find_by_id(DBA_MSG_YEAR);
    const Var* month = l_ana->find_by_id(DBA_MSG_MONTH);
    const Var* day = l_ana->find_by_id(DBA_MSG_DAY);
    const Var* hour = l_ana->find_by_id(DBA_MSG_HOUR);
    const Var* min = l_ana->find_by_id(DBA_MSG_MINUTE);
    const Var* sec = l_ana->find_by_id(DBA_MSG_SECOND);

    if (year == NULL || month == NULL || day == NULL || hour == NULL || min == NULL)
        throw error_notfound("date/time informations not found (or incomplete) in message to insert");

    Datetime dt(year->enqi(), month->enqi(), day->enqi(),
                hour->enqi(), min->enqi(), sec ? sec->enqi() : 0);

    // Insert the rest of the data
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        const msg::Context& ctx = *msg.data[i];
        bool is_ana_level = ctx.level == Level(257) && ctx.trange == Trange();
        // Skip the station info level
        if (is_ana_level) continue;

        size_t levtr_id = levtrs.obtain(ctx.level, ctx.trange);
        const LevTr& levtr = *levtrs[levtr_id];

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var& var = *ctx.data[j];
            if (not var.isset()) continue;

            auto_ptr<Var> newvar(new Var(var, with_attrs));
            values.insert(station, levtr, dt, newvar, replace);
        }
    }
}

namespace {
struct MatchAnaFilter : public Match<Station>
{
    const StationValues& stationvalues;
    Varmatch* match;

    MatchAnaFilter(const StationValues& stationvalues, const std::string& expr)
        : stationvalues(stationvalues), match(Varmatch::parse(expr).release()) {}
    ~MatchAnaFilter() { delete match; }

    virtual bool operator()(const Station& val) const
    {
        const StationValue* sv = stationvalues.get(val, match->code);
        if (!sv) return false;
        return (*match)(*(sv->var));
    }

private:
    MatchAnaFilter(const MatchAnaFilter&);
    MatchAnaFilter& operator=(const MatchAnaFilter&);
};
}

void Memdb::query_stations(const Record& rec, Results<Station>& res) const
{
    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_FILTER))
    {
        new MatchAnaFilter(stationvalues, val);
    }
    stations.query(rec, res);
#warning todo
#if 0
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 1)))
    {
        // No need to escape since the variable is integer
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_blo WHERE %s_blo.id_station=%s.id"
                               " AND %s_blo.id_var=257 AND %s_blo.id_lev_tr == -1 AND %s_blo.value='%s')",
                tbl, tbl, tbl, tbl, tbl, tbl, val);
        c.found = true;
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 2)))
    {
        sql_where.append_listf("EXISTS(SELECT id FROM data %s_sta WHERE %s_sta.id_station=%s.id"
                               " AND %s_sta.id_var=258 AND %s_sta.id_lev_tr == -1 AND %s_sta.value='%s')",
                tbl, tbl, tbl, tbl, tbl, tbl, val);
        c.found = true;
    }

    return c.found;
#endif
}

void Memdb::query_data(const Record& rec, memdb::Results<memdb::Value>& res) const
{
    // Get a list of stations we can match
    Results<Station> res_st(stations);
    query_stations(rec, res_st);

    // Get a list of stations we can match
    Results<LevTr> res_tr(levtrs);
    levtrs.query(rec, res_tr);


    // Query variables
    values.query(rec, res_st, res_tr, res);
}

void Memdb::dump(FILE* out) const
{
    stations.dump(out);
    levtrs.dump(out);
    values.dump(out);
}

}
