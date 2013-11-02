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
#include <dballe/core/record.h>
#include <dballe/core/defs.h>

#if 0
#include <cstdio>
#include <limits.h>
#include <unistd.h>
#endif

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

    // TODO:
#if 0
    /* Populate the tables with values */
    {
        int added, deleted, updated;
        repinfo().update(repinfo_file, &added, &deleted, &updated);
    }
#endif
}

void DB::update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

std::map<std::string, int> DB::get_repinfo_priorities()
{
    throw error_unimplemented("not yet implemented in MEM database");
}

const std::string& DB::rep_memo_from_cod(int rep_cod)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

void DB::insert(const Record& rec, bool can_replace, bool station_can_add)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

int DB::last_station_id() const
{
    throw error_unimplemented("not yet implemented in MEM database");
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
    throw error_unimplemented("not yet implemented in MEM database");
}

std::auto_ptr<db::Cursor> DB::query_data(const Record& query)
{
    throw error_unimplemented("not yet implemented in MEM database");
}

std::auto_ptr<db::Cursor> DB::query_summary(const Record& rec)
{
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
    throw error_unimplemented("not yet implemented in MEM database");
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
