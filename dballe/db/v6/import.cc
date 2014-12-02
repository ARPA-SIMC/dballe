/*
 * db/v6/import - Import Msg data into the database
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/db/internals.h"
#include "dballe/db/v5/station.h"
#include "lev_tr.h"
#include "internals.h"

#include <dballe/msg/msgs.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

void DB::import_msg(const Msg& msg, const char* repmemo, int flags)
{
    const msg::Context* l_ana = msg.find_context(Level::ana(), Trange());
	if (!l_ana)
		throw error_consistency("cannot import into the database a message without station information");

	// Check if the station is mobile
    bool mobile = msg.get_ident_var() != NULL;
	
    v5::Station& st = station();
    LevTr& lt = lev_tr();
    Data& dd = data();
    Attr& dq = attr();

    // Begin transaction
    db::Transaction t(*conn);

    // Fill up the pseudoana informations needed to fetch an existing ID

    // Latitude
    int lat;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");

    // Longitude
    int lon;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Station identifier
    const char* ident = NULL;
    if (mobile)
    {
        if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
            ident = var->value();
        else
            throw error_notfound("mobile station identifier not found in data to import");
    }

    bool inserted_pseudoana = false;
    int id_station = st.obtain_id(lat, lon, ident, &inserted_pseudoana);

    // Report code
    int id_report;
    if (repmemo != NULL)
        id_report = rep_cod_from_memo(repmemo);
    else {
        // TODO: check if B01194 first
        if (const Var* var = msg.get_rep_memo_var())
            id_report = rep_cod_from_memo(var->value());
        else
            id_report = rep_cod_from_memo(Msg::repmemo_from_type(msg.type));
    }

	if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || inserted_pseudoana)
	{
        dd.set_station_info(id_station, id_report);

		// Insert the rest of the station information
		for (size_t i = 0; i < l_ana->data.size(); ++i)
		{
            Varcode code = l_ana->data[i]->code();
			// Do not import datetime in the station info context
			if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
				continue;

            bool inserted = false;
            int id_data;
			if ((flags & DBA_IMPORT_OVERWRITE) == 0)
			{
				// Insert only if it is missing
				inserted = dd.insert_or_ignore(*l_ana->data[i], &id_data);
			} else {
				dd.insert_or_overwrite(*l_ana->data[i], &id_data);
				inserted = true;
			}

            /* Insert the attributes */
            if (inserted && (flags & DBA_IMPORT_ATTRS))
                for (const Var* attr = l_ana->data[i]->next_attr(); attr != NULL; attr = attr->next_attr())
                    if (attr->value() != NULL)
                        dq.write(id_data, *attr);
        }
    }

    // Fill up the common context information for the rest of the data

    // Date and time
    {
        const Var* year = l_ana->find_by_id(DBA_MSG_YEAR);
        const Var* month = l_ana->find_by_id(DBA_MSG_MONTH);
        const Var* day = l_ana->find_by_id(DBA_MSG_DAY);
        const Var* hour = l_ana->find_by_id(DBA_MSG_HOUR);
        const Var* min = l_ana->find_by_id(DBA_MSG_MINUTE);
        const Var* sec = l_ana->find_by_id(DBA_MSG_SECOND);

        if (year == NULL || month == NULL || day == NULL || hour == NULL || min == NULL)
            throw error_notfound("date/time informations not found (or incomplete) in message to insert");

        dd.set_date(year->enqi(), month->enqi(), day->enqi(), hour->enqi(), min->enqi(), sec ? sec->enqi() : 0);
    }

	/* Insert the rest of the data */
	for (size_t i = 0; i < msg.data.size(); ++i)
	{
		const msg::Context& ctx = *msg.data[i];
		bool is_ana_level = ctx.level == Level::ana() && ctx.trange == Trange();

        // Skip the station info level
        if (is_ana_level) continue;

        // Get the database ID of the lev_tr
        int id_lev_tr = lt.obtain_id(ctx.level, ctx.trange);

        dd.set_context(id_station, id_report, id_lev_tr);

		for (size_t j = 0; j < ctx.data.size(); ++j)
		{
            const Var& var = *ctx.data[j];
            if (not var.isset()) continue;

            // Insert the variable
            int id_data;
            if (flags & DBA_IMPORT_OVERWRITE)
                dd.insert_or_overwrite(var, &id_data);
            else
                dd.insert_or_fail(var, &id_data);

            /* Insert the attributes */
            if (flags & DBA_IMPORT_ATTRS)
                for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
                    if (attr->value() != NULL)
                        dq.write(id_data, *attr);
        }
    }

    t.commit();
}

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
