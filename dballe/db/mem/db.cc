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
#include <dballe/core/record.h>
#include <dballe/core/defs.h>
#include <dballe/memdb/query.h>
#include <algorithm>

#if 0
#include <cstdio>
#include <limits.h>
#include <unistd.h>
#endif

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

void DB::remove(const Record& rec)
{
    throw error_unimplemented("not yet implemented in MEM database");
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
    return auto_ptr<db::Cursor>(new CursorStations(*this, modifiers, res));
}

std::auto_ptr<db::Cursor> DB::query_data(const Record& query)
{
    unsigned int modifiers = parse_modifiers(query);
    Results<Value> res(memdb.values);
    memdb.query_data(query, res);
    return auto_ptr<db::Cursor>(new CursorData(*this, modifiers, res));
#if 0
        if (query_station_vars)
            sql_where.append_list("d.id_lev_tr == -1");
        else
            sql_where.append_list("d.id_lev_tr != -1");
#endif
#if 0
    auto_ptr<Cursor> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
        res.reset(new CursorBest(*this, modifiers));
    else
        res.reset(new CursorData(*this, modifiers));
    res->query(query);
    return auto_ptr<db::Cursor>(res.release());
#endif
}

std::auto_ptr<db::Cursor> DB::query_summary(const Record& rec)
{
#if 0
    auto_ptr<Cursor> res(new CursorSummary(*this, 0));
    res->query(rec);
    return auto_ptr<db::Cursor>(res.release());
#endif
    throw error_unimplemented("not yet implemented in MEM database");
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

void DB::attr_insert(wreport::Varcode id_var, const Record& attrs, bool can_replace)
{
    // Find the data id for id_var
    for (vector<VarID>::const_iterator i = last_insert_varids.begin();
            i != last_insert_varids.end(); ++i)
        if (i->code == id_var)
        {
            attr_insert(i->id, id_var, attrs, can_replace);
            return;
        }
    error_notfound::throwf("variable B%02d%03d was not involved in the last insert operation", WR_VAR_X(id_var), WR_VAR_Y(id_var));
}

void DB::attr_insert(int id_data, wreport::Varcode id_var, const Record& attrs, bool can_replace)
{
    Value& val = *memdb.values[id_data];
    for (vector<Var*>::const_iterator i = attrs.vars().begin(); i != attrs.vars().end(); ++i)
    {
        if (!can_replace && val.var->enqa((*i)->code()))
            error_consistency::throwf("attribute B%02d%03d already exists on variable B%02d%03d",
                    WR_VAR_X((*i)->code()), WR_VAR_Y((*i)->code()),
                    WR_VAR_X(val.var->code()), WR_VAR_Y(val.var->code()));
        val.var->seta(**i);
    }
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

void DB::export_msgs(const Record& query, MsgConsumer& cons)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

}
}
}

/* vim:set ts=4 sw=4: */
