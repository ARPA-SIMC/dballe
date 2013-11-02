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
#include <dballe/core/record.h>
#include <dballe/msg/msg.h>

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

void Memdb::insert_or_replace(const Msg& msg)
{
}

void Memdb::query_stations(const Record& rec, Results<Station>& res) const
{
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
    if (const char* val = rec.key_peek_value(DBA_KEY_ANA_FILTER))
    {
        const char *op, *value, *value1;
        Varinfo info = decode_data_filter(val, &op, &value, &value1);

        sql_where.append_listf("EXISTS(SELECT id FROM data %s_af WHERE %s_af.id_station=%s.id"
                               " AND %s_af.id_lev_tr == -1"
                               " AND %s_af.id_var=%d", tbl, tbl, tbl, tbl, tbl, info->var);

        if (value[0] == '\'')
            if (value1 == NULL)
                sql_where.appendf(" AND %s_af.value%s%s)", tbl, op, value);
            else
                sql_where.appendf(" AND %s_af.value BETWEEN %s AND %s)", tbl, value, value1);
        else
        {
            const char* type = (db.conn->server_type == MYSQL) ? "SIGNED" : "INT";
            if (value1 == NULL)
                sql_where.appendf(" AND CAST(%s_af.value AS %s)%s%s)", tbl, type, op, value);
            else
                sql_where.appendf(" AND CAST(%s_af.value AS %s) BETWEEN %s AND %s)", tbl, type, value, value1);
        }

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

    // Query variables
    values.query(rec, res_st, res);
}

void Memdb::dump(FILE* out) const
{
    stations.dump(out);
}

}
