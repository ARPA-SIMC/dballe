/*
 * db/v6/import - Import Msg data into the database
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "data.h"
#include "attr.h"

#include <dballe/msg/msgs.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

using namespace wreport;

namespace dballe {
namespace db {
namespace v6 {

void DB::import_msg(const Msg& msg, const char* repmemo, int flags)
{
    const msg::Context* l_ana = msg.find_context(Level(257), Trange());
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
	if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
		st.lat = var->enqi();
	else
        throw error_notfound("latitude not found in data to import");

	// Longitude
	if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
		st.lon = var->enqi();
	else
        throw error_notfound("longitude not found in data to import");

	// Station identifier
	if (mobile)
	{
		if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
			st.set_ident(var->value());
		else
            throw error_notfound("mobile station identifier not found in data to import");
	} else
        st.set_ident(NULL);

    bool inserted_pseudoana = false;
    // Check if we can reuse a pseudoana row
    dd.id_station = st.get_id();
    if (dd.id_station == -1)
    {
        dd.id_station = st.insert();
        inserted_pseudoana = true;
    }

    // Report code
    if (repmemo != NULL)
        dd.id_report = rep_cod_from_memo(repmemo);
    else {
        // TODO: check if B01194 first
        if (const Var* var = msg.get_rep_memo_var())
            dd.id_report = rep_cod_from_memo(var->value());
        else
            dd.id_report = rep_cod_from_memo(Msg::repmemo_from_type(msg.type));
    }

	if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || inserted_pseudoana)
	{
        dd.set_station_info();

		// Insert the rest of the station information
		for (size_t i = 0; i < l_ana->data.size(); ++i)
		{
            Varcode code = l_ana->data[i]->code();
			// Do not import datetime in the station info context
			if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
				continue;

			dd.set(*l_ana->data[i]);

            bool inserted = false;
			if ((flags & DBA_IMPORT_OVERWRITE) == 0)
			{
				// Insert only if it is missing
				inserted = dd.insert_or_ignore(true);
			} else {
				dd.insert_or_overwrite(true);
				inserted = true;
			}

			dq.id_data = dd.id;

			/* Insert the attributes */
			if (inserted && (flags & DBA_IMPORT_ATTRS))
                for (const Var* attr = l_ana->data[i]->next_attr(); attr != NULL; attr = attr->next_attr())
					if (attr->value() != NULL)
					{
						dq.set(*attr);
                        dq.insert();
					}
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

        dd.date.year = year->enqi();
        dd.date.month = month->enqi();
        dd.date.day = day->enqi();
        dd.date.hour = hour->enqi();
        dd.date.minute = min->enqi();
        dd.date.second = sec ? sec->enqi() : 0;
        dd.date.fraction = 0;
    }

	/* Insert the rest of the data */
	for (size_t i = 0; i < msg.data.size(); ++i)
	{
		const msg::Context& ctx = *msg.data[i];
		bool is_ana_level = ctx.level == Level(257) && ctx.trange == Trange();

        // Skip the station info level
        if (is_ana_level) continue;

        // Insert the new level/timerange
        lt.ltype1 = ctx.level.ltype1;
        lt.l1 = ctx.level.l1;
        lt.ltype2 = ctx.level.ltype2;
        lt.l2 = ctx.level.l2;
        lt.pind = ctx.trange.pind;
        lt.p1 = ctx.trange.p1;
        lt.p2 = ctx.trange.p2;

        // Get the database ID of the lev_tr
        dd.set_id_lev_tr(lt.get_id());
        if (dd.id_lev_tr == -1)
            dd.set_id_lev_tr(lt.insert());

		for (size_t j = 0; j < ctx.data.size(); ++j)
		{
            const Var& var = *ctx.data[j];
            if (not var.isset()) continue;

            // Insert the variable
            dd.set(var);
            if (flags & DBA_IMPORT_OVERWRITE)
                dd.insert_or_overwrite(true);
            else
                dd.insert_or_fail(true);

            /* Insert the attributes */
            if (flags & DBA_IMPORT_ATTRS)
            {
                dq.id_data = dd.id;

				for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
					if (attr->value() != NULL)
					{
                        dq.set(*attr);
                        dq.insert();
					}
			}
		}
	}

    t.commit();
}

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
