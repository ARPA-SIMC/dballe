/*
 * db/export - Export Msg data from the database
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
#include "cursor.h"
#include "dballe/db/odbc/internals.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/sql/station.h"
#include "dballe/db/sql/attrv5.h"

#include <dballe/msg/msg.h>

#include <memory>
#include <cstring>

#include <sql.h>
#if 0
#include "querybuf.h"
#include <stdlib.h>
#endif

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

void DB::fill_ana_layer(Msg& msg, int id_station, int id_report)
{
    station().get_station_vars(id_station, id_report, [&](std::unique_ptr<wreport::Var> var) {
        msg.set(move(var), Level::ana(), Trange());
    });
}

static inline int sqltimecmp(const SQL_TIMESTAMP_STRUCT* a, const SQL_TIMESTAMP_STRUCT* b)
{
	return memcmp(a, b, sizeof(SQL_TIMESTAMP_STRUCT));
}

void DB::export_msgs(const Record& rec, MsgConsumer& consumer)
{
    sql::AttrV5& at = attr();

    // Message being built
    unique_ptr<Msg> msg;

    auto t = conn->transaction();

    // The big export query
    db::v5::Cursor cur(*this);
	cur.query(rec,
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_CONTEXT_ID |
				DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT | DBA_DB_WANT_LEVEL |
                DBA_DB_WANT_TIMERANGE | DBA_DB_WANT_DATETIME |
                DBA_DB_WANT_VAR_NAME | DBA_DB_WANT_VAR_VALUE |
                DBA_DB_WANT_REPCOD,
				DBA_DB_MODIFIER_SORT_FOR_EXPORT);
			/*	DBA_DB_MODIFIER_STREAM)); */

    // Current context information used to detect context changes
	SQL_TIMESTAMP_STRUCT last_datetime;
	last_datetime.year = 0;
	char last_ident[70];
	last_ident[0] = 0;
	int last_lat = -1;
	int last_lon = -1;
	int last_rep_cod = -1;

	// Retrieve results
	while (cur.next())
	{
		TRACE("Got B%02d%03d %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
				WR_VAR_X(cur.out_varcode), WR_VAR_Y(cur.out_varcode),
				cur.out_ltype1, cur.out_l1, cur.out_ltype2, cur.out_l2, cur.out_pind, cur.out_p1, cur.out_p2,
				cur.out_value);

		bool ident_differs;
		if (cur.out_ident_ind != SQL_NULL_DATA)
			ident_differs = strncmp(last_ident, cur.out_ident, cur.out_ident_ind) != 0;
		else
			ident_differs = last_ident[0] != 0;

		/* Create the variable that we got on this iteration */
        unique_ptr<Var> var(newvar(cur.out_varcode, cur.out_value));

        // Load the attributes from the database
        at.load(cur.out_context_id, *var);

		/* See if we have the start of a new message */
		if (cur.out_lat != last_lat || cur.out_lon != last_lon || sqltimecmp(&(cur.out_datetime), &last_datetime) != 0 || ident_differs || cur.out_rep_cod != last_rep_cod)
		{
			TRACE("New message\n");
			if (msg.get() != NULL)
			{
				TRACE("Sending old message to consumer\n");
				if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
				{
                    unique_ptr<Msg> copy(new Msg);
                    msg->sounding_pack_levels(*copy);
					/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
                    consumer(move(copy));
                    msg.release();
				} else
                    consumer(move(msg));
			}

            // Start writing a new message
            msg.reset(new Msg);
		
            // Fill in report information
			{
                const char* memo = cur.get_rep_memo();
                msg->set_rep_memo(memo);
				msg->type = Msg::type_from_repmemo(memo);
			}

			// Fill in the basic station values
            msg->seti(WR_VAR(0, 5, 1), cur.out_lat, -1, Level::ana(), Trange());
            msg->seti(WR_VAR(0, 6, 1), cur.out_lon, -1, Level::ana(), Trange());
			if (cur.out_ident_ind != SQL_NULL_DATA)
				msg->set_ident(cur.out_ident);

			// Fill in datetime
            msg->set_year(cur.out_datetime.year);
            msg->set_month(cur.out_datetime.month);
            msg->set_day(cur.out_datetime.day);
            msg->set_hour(cur.out_datetime.hour);
            msg->set_minute(cur.out_datetime.minute);
            msg->set_second(cur.out_datetime.second);

            // Fill in extra station info
			fill_ana_layer(*msg, cur.out_ana_id, cur.out_rep_cod);

            // Update current context information
			last_datetime = cur.out_datetime;
			last_lat = cur.out_lat;
			last_lon = cur.out_lon;
			if (cur.out_ident_ind != SQL_NULL_DATA)
				strncpy(last_ident, cur.out_ident, cur.out_ident_ind);
			else
				last_ident[0] = 0;
			last_rep_cod = cur.out_rep_cod;
		}

		TRACE("Inserting var B%02d%03d (%s)\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()), var->value());
        msg->set(move(var),
                Level(cur.out_ltype1, cur.out_l1, cur.out_ltype2, cur.out_l2),
                Trange(cur.out_pind, cur.out_p1, cur.out_p2));
	}

	if (msg.get() != NULL)
	{
		TRACE("Inserting leftover old message\n");
		if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
		{
            unique_ptr<Msg> copy(new Msg);
            msg->sounding_pack_levels(*copy);
            /* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
            consumer(move(copy));
            msg.release();
        } else
            consumer(move(msg));
    }

    /* Useful for Oracle to end the session */
    t->commit();
}

}
}
}

/* vim:set ts=4 sw=4: */
