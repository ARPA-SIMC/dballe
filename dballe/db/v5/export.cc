/*
 * db/export - Export Msg data from the database
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
#include "cursor.h"
#include "dballe/db/internals.h"
#include "attr.h"

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

void DB::fill_ana_layer(Msg& msg, int id_station, int id_report)
{
	// Perform the query
	static const char query[] =
		"SELECT d.id_var, d.value, a.type, a.value"
		"  FROM context c, data d"
		"  LEFT JOIN attr a ON a.id_context = d.id_context AND a.id_var = d.id_var"
		" WHERE d.id_context = c.id AND c.id_ana = ? AND c.id_report = ?"
		"   AND c.datetime = {ts '1000-01-01 00:00:00.000'} AND c.ltype1 = 257"
		" ORDER BY d.id_var, a.type";

    db::Statement stm(*conn);

	DBALLE_SQL_C_SINT_TYPE in_id_station = id_station;
    stm.bind_in(1, in_id_station);

	DBALLE_SQL_C_SINT_TYPE in_id_report = id_report;
    stm.bind_in(2, in_id_report);

	DBALLE_SQL_C_SINT_TYPE out_varcode;
    stm.bind_out(1, out_varcode);

	char out_value[255];
	stm.bind_out(2, out_value, sizeof(out_value));

	DBALLE_SQL_C_SINT_TYPE out_attr_varcode;		SQLLEN out_attr_varcode_ind;
    stm.bind_out(3, out_attr_varcode, out_attr_varcode_ind);

	char out_attr_value[255];	SQLLEN out_attr_value_ind;
	stm.bind_out(4, out_attr_value, sizeof(out_attr_value), out_attr_value_ind);

	TRACE("fill_ana_layer Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);
    stm.exec_direct(query);

	// Retrieve results
	Varcode last_varcode = 0;
    auto_ptr<Var> var;
	while (stm.fetch())
	{
		TRACE("fill_ana_layer Got B%02ld%03ld %s\n", WR_VAR_X(out_varcode), WR_VAR_Y(out_varcode), out_value);

		// First process the variable, possibly inserting the old one in the message
		if (last_varcode != out_varcode)
		{
			TRACE("fill_ana_layer new var\n");
			if (var.get())
			{
				TRACE("fill_ana_layer inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                msg.set(var, Level(257), Trange());
			}
            var = newvar(out_varcode, out_value);
			last_varcode = out_varcode;
		}

		if (out_attr_varcode_ind != -1)
		{
			TRACE("fill_ana_layer new attribute\n");
            var->seta(newvar(out_attr_varcode, out_attr_value));
		}
	}

	if (var.get())
	{
		TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
        msg.set(var, Level(257), Trange());
	}
}

static inline int sqltimecmp(const SQL_TIMESTAMP_STRUCT* a, const SQL_TIMESTAMP_STRUCT* b)
{
	return memcmp(a, b, sizeof(SQL_TIMESTAMP_STRUCT));
}

void DB::export_msgs(const Record& rec, MsgConsumer& consumer)
{
    db::Attr& at = attr();

    // Message being built
    auto_ptr<Msg> msg;

    db::Transaction t(*conn);

    // The big export query
    db::Cursor cur(*this);
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
        auto_ptr<Var> var(newvar(cur.out_varcode, cur.out_value));

		/* Load the attributes from the database */
        at.id_context = cur.out_context_id;
        at.load(*var);

		/* See if we have the start of a new message */
		if (cur.out_lat != last_lat || cur.out_lon != last_lon || sqltimecmp(&(cur.out_datetime), &last_datetime) != 0 || ident_differs || cur.out_rep_cod != last_rep_cod)
		{
			TRACE("New message\n");
			if (msg.get() != NULL)
			{
				TRACE("Sending old message to consumer\n");
				if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
				{
                    auto_ptr<Msg> copy(new Msg);
                    msg->sounding_pack_levels(*copy);
					/* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
                    consumer(copy);
                    msg.release();
				} else
                    consumer(msg);
			}

            // Start writing a new message
            msg.reset(new Msg);
		
            // Fill in report information
			{
				const char* memo = rep_memo_from_cod(cur.out_rep_cod).c_str();
                msg->set_rep_memo(memo);
				msg->type = Msg::type_from_repmemo(memo);
			}

			// Fill in the basic station values
            msg->seti(WR_VAR(0, 5, 1), cur.out_lat, -1, Level(257), Trange());
            msg->seti(WR_VAR(0, 6, 1), cur.out_lon, -1, Level(257), Trange());
			if (cur.out_ident_ind != SQL_NULL_DATA)
				msg->set_ident(cur.out_ident);

			// Fill in datetime
            msg->set_year(cur.out_datetime.year);
            msg->set_month(cur.out_datetime.month);
            msg->set_day(cur.out_datetime.day);
            msg->set_hour(cur.out_datetime.hour);
            msg->set_minute(cur.out_datetime.minute);

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
        msg->set(var,
                Level(cur.out_ltype1, cur.out_l1, cur.out_ltype2, cur.out_l2),
                Trange(cur.out_pind, cur.out_p1, cur.out_p2));
	}

	if (msg.get() != NULL)
	{
		TRACE("Inserting leftover old message\n");
		if (msg->type == MSG_PILOT || msg->type == MSG_TEMP || msg->type == MSG_TEMP_SHIP)
		{
            auto_ptr<Msg> copy(new Msg);
            msg->sounding_pack_levels(*copy);
            /* DBA_RUN_OR_GOTO(cleanup, dba_msg_sounding_reverse_levels(msg)); */
            consumer(copy);
            msg.release();
		} else
            consumer(msg);
	}

	/* Useful for Oracle to end the session */
    t.commit();
}

} // namespace dballe

/* vim:set ts=4 sw=4: */
