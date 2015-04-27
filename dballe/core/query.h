/*
 * dballe/core/query - Represent a filter for DB-All.e data
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#ifndef DBALLE_CORE_QUERY_H
#define DBALLE_CORE_QUERY_H

#include <dballe/core/defs.h>
#include <dballe/core/record.h>
#include <wreport/varinfo.h>
#include <set>

namespace dballe {

struct Query
{
    int ana_id = MISSING_INT;
    int prio_exact = MISSING_INT;
    int prio_min = MISSING_INT;
    int prio_max = MISSING_INT;
    std::string rep_memo;
    int mobile = MISSING_INT;
    std::string ident;
    Coord coords_exact;
    Coord coords_min;
    Coord coords_max;
    Datetime datetime_exact;
    Datetime datetime_min;
    Datetime datetime_max;
    Level level;
    Trange trange;
    std::set<wreport::Varcode> varcodes;
    std::string query;
    std::string ana_filter;
    std::string data_filter;
    std::string attr_filter;
    int limit = MISSING_INT;
    // DBA_KEY_CONTEXT_ID	= 40,
    // DBA_KEY_VAR_RELATED	= 46,

    void seti(dba_keyword key, int val);
    void setd(dba_keyword key, double val);
    void setc(dba_keyword key, const char* val);
    void setc(dba_keyword key, const std::string& val);
    void unset(dba_keyword key);
};

}

#endif
