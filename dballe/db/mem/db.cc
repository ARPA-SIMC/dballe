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
#include "dballe/core/varmatch.h"
#include "dballe/core/record.h"
#include "dballe/core/defs.h"
#include "dballe/memdb/results.h"
#include "dballe/memdb/serializer.h"
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

DB::DB() {}

DB::DB(const std::string& arg)
    : serialization_dir(arg)
{
}

DB::~DB()
{
    if (!serialization_dir.empty())
    {
        serialize::CSVWriter writer(serialization_dir);
        writer.write(memdb);
        writer.commit();
    }
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
    Results<Value> res(memdb.values);
    raw_query_data(query, res);
    memdb.remove(res);
}

void DB::vacuum()
{
    // Nothing to do
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

struct MatchRepinfo : public Match<Station>
{
    std::set<std::string> report_whitelist;

    virtual bool operator()(const Station& val) const
    {
        return report_whitelist.find(val.report) != report_whitelist.end();
    }
};

}

void DB::raw_query_stations(const Record& rec, memdb::Results<memdb::Station>& res)
{
    // Build a matcher for queries by priority
    int prio = rec.get(DBA_KEY_PRIORITY, MISSING_INT);
    int priomin = rec.get(DBA_KEY_PRIOMIN, MISSING_INT);
    int priomax = rec.get(DBA_KEY_PRIOMAX, MISSING_INT);
    if (prio != MISSING_INT || priomin != MISSING_INT || priomax != MISSING_INT)
    {
        // If priomax == priomin and prio is unset, use exact prio instead of
        // min-max bounds
        if (prio == MISSING_INT && priomax == priomin)
        {
            prio = priomin;
            priomax = priomin = MISSING_INT;
        }

        if (prio != MISSING_INT)
        {
            // Deal with impossible prio/priomin/priomax combinations
            if (priomin != MISSING_INT && prio < priomin)
            {
                res.set_to_empty();
                return;
            }
            if (priomax != MISSING_INT && prio > priomax)
            {
                res.set_to_empty();
                return;
            }

            // We can now ignore priomin and priomax

            std::map<std::string, int> prios = get_repinfo_priorities();
            auto_ptr<MatchRepinfo> m(new MatchRepinfo);
            for (std::map<std::string, int>::const_iterator i = prios.begin();
                    i != prios.end(); ++i)
                if (i->second == prio)
                    m->report_whitelist.insert(i->first);
            res.add(m.release());
        } else {
            // Here, prio is unset and priomin != priomax

            // Deal with priomin > priomax
            if (priomin != MISSING_INT && priomax != MISSING_INT && priomax < priomin)
            {
                res.set_to_empty();
                return;
            }

            std::map<std::string, int> prios = get_repinfo_priorities();
            auto_ptr<MatchRepinfo> m(new MatchRepinfo);
            for (std::map<std::string, int>::const_iterator i = prios.begin();
                    i != prios.end(); ++i)
            {
                if (priomin != MISSING_INT && i->second < priomin) continue;
                if (priomax != MISSING_INT && i->second > priomax) continue;
                m->report_whitelist.insert(i->first);
            }
            res.add(m.release());
        }
    }

    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_FILTER))
    {
        res.add(new MatchAnaFilter(memdb.stationvalues, val));
        trace_query("Found ana filter %s\n", val);
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 1)))
    {
        string query("B01001=");
        query += val;
        res.add(new MatchAnaFilter(memdb.stationvalues, query));
        trace_query("Found block filter %s\n", query.c_str());
    }
    if (const char* val = rec.var_peek_value(WR_VAR(0, 1, 2)))
    {
        string query("B01002=");
        query += val;
        res.add(new MatchAnaFilter(memdb.stationvalues, query));
        trace_query("Found station filter %s\n", query.c_str());
    }

    memdb.stations.query(rec, res);
}


void DB::raw_query_station_data(const Record& rec, memdb::Results<memdb::StationValue>& res)
{
    // Get a list of stations we can match
    Results<Station> res_st(memdb.stations);

    raw_query_stations(rec, res_st);

    memdb.stationvalues.query(rec, res_st, res);
}

void DB::raw_query_data(const Record& rec, memdb::Results<memdb::Value>& res)
{
    // Get a list of stations we can match
    Results<Station> res_st(memdb.stations);
    raw_query_stations(rec, res_st);

    // Get a list of stations we can match
    Results<LevTr> res_tr(memdb.levtrs);
    memdb.levtrs.query(rec, res_tr);

    // Query variables
    memdb.values.query(rec, res_st, res_tr, res);
}

std::auto_ptr<db::Cursor> DB::query_stations(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    Results<Station> res(memdb.stations);

    // Build var/varlist special-cased filter for station queries
    const char* filter_var = query.key_peek_value(DBA_KEY_VAR);
    const char* filter_varlist = query.key_peek_value(DBA_KEY_VARLIST);
    if (filter_var || filter_varlist)
    {
        set<Varcode> varcodes;
        if (filter_var) varcodes.insert(resolve_varcode_safe(filter_var));
        if (filter_varlist) resolve_varlist_safe(filter_varlist, varcodes);

        // Iterate all the possible values, taking note of the stations for
        // variables whose varcodes are in 'varcodes'
        auto_ptr< set<size_t> > id_whitelist(new set<size_t>);
        for (ValueStorage<Value>::index_iterator i = memdb.values.index_begin();
                i != memdb.values.index_end(); ++i)
        {
            const Value& v = *memdb.values[*i];
            if (varcodes.find(v.var->code()) != varcodes.end())
                id_whitelist->insert(*i);
        }
        IF_TRACE_QUERY {
            trace_query("Found var/varlist station filter: %zd items in id whitelist:", id_whitelist->size());
            for (set<size_t>::const_iterator i = id_whitelist->begin(); i != id_whitelist->end(); ++i)
                trace_query(" %zd", *i);
            trace_query("\n");
        }
        res.add_set(id_whitelist);
    }

    raw_query_stations(query, res);
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
            raw_query_station_data(query, res);
            return Cursor::createStationData(*this, modifiers, res);
        }
    } else {
        Results<Value> res(memdb.values);
        raw_query_data(query, res);
        if (modifiers & DBA_DB_MODIFIER_BEST)
        {
            return Cursor::createDataBest(*this, modifiers, res);
        } else {
            return Cursor::createData(*this, modifiers, res);
        }
    }
}

std::auto_ptr<db::Cursor> DB::query_summary(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    bool query_station_vars = (query.get(DBA_KEY_LEVELTYPE1, 0) == 257);
    if (query_station_vars)
    {
        throw error_unimplemented("summary query of station vars");
#warning TODO
    } else {
        Results<Value> res(memdb.values);
        raw_query_data(query, res);
        return Cursor::createSummary(*this, modifiers, res);
    }
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
    raw_query_data(query, res);

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

#include "dballe/memdb/results.tcc"

/* vim:set ts=4 sw=4: */
