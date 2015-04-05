/*
 * db/import - Import Msg data into the database
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
#include "dballe/db/odbc/internals.h"
#include "dballe/db/sql/repinfo.h"
#include "dballe/db/sql/station.h"
#include "context.h"
#include "data.h"
#include "attr.h"

#include <dballe/msg/msgs.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>
#if 0

#include "dballe/core/conv.h"
#include "dballe/msg/context.h"

#include <sql.h>
#include <sqlext.h>

#include <string.h>
#include <stdlib.h>

#include "config.h"
#endif

using namespace wreport;

namespace dballe {
namespace db {
namespace v5 {

void DB::import_msg(const Msg& msg, const char* repmemo, int flags)
{
    const msg::Context* l_ana = msg.find_context(Level::ana(), Trange());
	if (!l_ana)
		throw error_consistency("cannot import into the database a message without station information");

	// Check if the station is mobile
    bool mobile = msg.get_ident_var() != NULL;

    sql::Station& st = station();
    Context& dc = context();
    Data& dd = data();
    Attr& dq = attr();

    // Begin transaction
    auto t = conn->transaction();

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

    // Check if we can reuse a pseudoana row
    bool inserted_pseudoana = false;
    dc.id_station = st.obtain_id(lat, lon, ident, &inserted_pseudoana);

    // Report code
    if (repmemo != NULL)
        dc.id_report = repinfo().obtain_id(repmemo);
    else {
        // TODO: check if B01194 first
        if (const Var* var = msg.get_rep_memo_var())
            dc.id_report = repinfo().obtain_id(var->value());
        else
            dc.id_report = repinfo().obtain_id(Msg::repmemo_from_type(msg.type));
    }

	if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || inserted_pseudoana)
	{
        dd.id_context = dc.obtain_station_info();

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
				inserted = dd.insert_or_ignore();
			} else {
				dd.insert_or_overwrite();
				inserted = true;
			}

			dq.id_context = dd.id_context;
			dq.id_var = l_ana->data[i]->code();

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

		dc.date.year = year->enqi();
		dc.date.month = month->enqi();
		dc.date.day = day->enqi();
		dc.date.hour = hour->enqi();
		dc.date.minute = min->enqi();
		dc.date.second = sec ? sec->enqi() : 0;
	}

	/* Insert the rest of the data */
	for (size_t i = 0; i < msg.data.size(); ++i)
	{
		const msg::Context& ctx = *msg.data[i];
		bool is_ana_level = ctx.level == Level::ana() && ctx.trange == Trange();

        // Skip the station info level
        if (is_ana_level)
            continue;

		/* Insert the new context */
		dc.ltype1 = ctx.level.ltype1;
		dc.l1 = ctx.level.l1;
		dc.ltype2 = ctx.level.ltype2;
		dc.l2 = ctx.level.l2;
		dc.pind = ctx.trange.pind;
		dc.p1 = ctx.trange.p1;
		dc.p2 = ctx.trange.p2;

		// Get the database ID of the context
        dd.id_context = dc.get_id();
		if (dd.id_context == -1)
            dd.id_context = dc.insert();

		for (size_t j = 0; j < ctx.data.size(); ++j)
		{
            const Var& var = *ctx.data[j];
            if (not var.isset()) continue;

            // Insert the variable
            dd.set(var);
			if (flags & DBA_IMPORT_OVERWRITE)
				dd.insert_or_overwrite();
			else
				dd.insert_or_fail();

			/* Insert the attributes */
			if (flags & DBA_IMPORT_ATTRS)
			{
				dq.id_context = dd.id_context;
				dq.id_var = var.code();

				for (const Var* attr = var.next_attr(); attr; attr = attr->next_attr())
					if (attr->value() != NULL)
					{
                        dq.set(*attr);
                        dq.insert();
					}
			}
		}
	}

    t->commit();
}

}
}
}

/* vim:set ts=4 sw=4: */
