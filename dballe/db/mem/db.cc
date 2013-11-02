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
#if 0
    unsigned int modifiers = parse_modifiers(query);
    auto_ptr<Cursor> res;
    if (modifiers & DBA_DB_MODIFIER_BEST)
        res.reset(new CursorBest(*this, modifiers));
    else
        res.reset(new CursorData(*this, modifiers));
    res->query(query);
    return auto_ptr<db::Cursor>(res.release());
#endif
    throw error_unimplemented("not yet implemented in MEM database");
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
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::attr_insert(wreport::Varcode id_var, const Record& attrs, bool can_replace)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::attr_insert(int id_data, wreport::Varcode id_var, const Record& attrs, bool can_replace)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::attr_remove(int id_data, wreport::Varcode id_var, const std::vector<wreport::Varcode>& qcs)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::dump(FILE* out)
{
    fprintf(out, "repinfo data:\n");
    repinfo.dump(out);
    // TODO: dump memdb
}

void DB::import_msg(const Msg& msg, const char* repmemo, int flags)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::import_msgs(const Msgs& msgs, const char* repmemo, int flags)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::export_msgs(const Record& query, MsgConsumer& cons)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

}
}
}

/* vim:set ts=4 sw=4: */
