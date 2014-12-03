/*
 * db/v6/export - Export Msg data from the database
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
#include "internals.h"

#include "lev_tr.h"
#include <dballe/msg/msg.h>
#include <dballe/msg/context.h>

#include <memory>
#include <cstring>
#include <iostream>

#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v6 {

namespace {


struct StationLayerCache : protected std::vector<wreport::Var*>
{
    ~StationLayerCache()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
    }

    void reset()
    {
        for (iterator i = begin(); i != end(); ++i)
            delete *i;
        clear();
    }

    void to_context(msg::Context& c) const
    {
        for (const_iterator i = begin(); i != end(); ++i)
            c.set(**i);
    }

    void fill(DB& db, int id_station, int id_report)
    {
        reset();

        // Perform the query
        static const char query[] =
            "SELECT d.id_var, d.value, a.type, a.value"
            "  FROM data d"
            "  LEFT JOIN attr a ON a.id_data = d.id"
            " WHERE d.id_station = ? AND d.id_report = ?"
            "   AND d.id_lev_tr = -1"
            " ORDER BY d.id_var, a.type";

        auto stm = db.conn->odbcstatement();

        stm->bind_in(1, id_station);
        stm->bind_in(2, id_report);

        Varcode out_varcode;
        stm->bind_out(1, out_varcode);

        char out_value[255];
        stm->bind_out(2, out_value, sizeof(out_value));

        Varcode out_attr_varcode;
        SQLLEN out_attr_varcode_ind;
        stm->bind_out(3, out_attr_varcode, out_attr_varcode_ind);

        char out_attr_value[255];
        SQLLEN out_attr_value_ind;
        stm->bind_out(4, out_attr_value, sizeof(out_attr_value), out_attr_value_ind);

        TRACE("StationLayerCache::fill Performing query: %s with idst %d idrep %d\n", query, id_station, id_report);
        stm->exec_direct(query);

        // Retrieve results
        Varcode last_varcode = 0;
        unique_ptr<Var> var;
        while (stm->fetch())
        {
            TRACE("StationLayerCache::fill Got B%02ld%03ld %s\n", WR_VAR_X(out_varcode), WR_VAR_Y(out_varcode), out_value);

            // First process the variable, possibly inserting the old one in the message
            if (last_varcode != out_varcode)
            {
                TRACE("StationLayerCache::fill new var\n");
                if (var.get())
                {
                    TRACE("StationLayerCache::fill inserting old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
                    push_back(var.release());
                }
                var = newvar(out_varcode, out_value);
                last_varcode = out_varcode;
            }

            if (out_attr_varcode_ind != -1)
            {
                TRACE("fill_ana_layer new attribute\n");
                var->seta(ap_newvar(out_attr_varcode, out_attr_value));
            }
        }

        if (var.get())
        {
            TRACE("fill_ana_layer inserting leftover old var B%02d%03d\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()));
            push_back(var.release());
        }
    }
};

}

static inline int sqltimecmp(const SQL_TIMESTAMP_STRUCT* a, const SQL_TIMESTAMP_STRUCT* b)
{
	return memcmp(a, b, sizeof(SQL_TIMESTAMP_STRUCT));
}

void DB::export_msgs(const Record& rec, MsgConsumer& consumer)
{
    Attr& at = attr();
    LevTrCache& ltrc = lev_tr_cache();

    // Message being built
    unique_ptr<Msg> msg;

    auto t = conn->transaction();

    // The big export query
    CursorData cur(*this, DBA_DB_MODIFIER_SORT_FOR_EXPORT);
            /*  DBA_DB_MODIFIER_STREAM)); */
    cur.query(rec);

    // Current context information used to detect context changes
	SQL_TIMESTAMP_STRUCT last_datetime;
	last_datetime.year = 0;
	int last_ana_id = -1;
	int last_rep_cod = -1;

    StationLayerCache station_cache;

    // Retrieve results
    while (cur.next())
    {
        //TRACE("Got B%02d%03d %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
        //        WR_VAR_X(cur.sqlrec.out_varcode), WR_VAR_Y(cur.sqlrec.out_varcode),
        //        cur.sqlrec.out_ltype1, cur.sqlrec.out_l1, cur.sqlrec.out_ltype2, cur.sqlrec.out_l2, cur.sqlrec.out_pind, cur.sqlrec.out_p1, cur.sqlrec.out_p2,
        //        cur.sqlrec.out_value);

        /* Create the variable that we got on this iteration */
        unique_ptr<Var> var(newvar(cur.sqlrec.out_varcode, cur.sqlrec.out_value));

        /* Load the attributes from the database */
        at.read(cur.sqlrec.out_id_data, *var);

        /* See if we have the start of a new message */
        if (cur.sqlrec.out_ana_id != last_ana_id
         || cur.sqlrec.out_rep_cod != last_rep_cod
         || sqltimecmp(&(cur.sqlrec.out_datetime), &last_datetime) != 0)
        {
            // Flush current message
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
                } else
                    consumer(move(msg));
            }

            // Start writing a new message
            msg.reset(new Msg);
            msg::Context& c_st = msg->obtain_station_context();

            // Update station layer cache if needed
            if (cur.sqlrec.out_ana_id != last_ana_id || cur.sqlrec.out_rep_cod != last_rep_cod)
                station_cache.fill(*this, cur.sqlrec.out_ana_id, cur.sqlrec.out_rep_cod);

            // Fill in report information
            {
                const char* memo = cur.get_rep_memo();
                c_st.set_rep_memo(memo);
                msg->type = Msg::type_from_repmemo(memo);
            }

            // Fill in the basic station values
            c_st.seti(WR_VAR(0, 5, 1), cur.sqlrec.out_lat);
            c_st.seti(WR_VAR(0, 6, 1), cur.sqlrec.out_lon);
            if (cur.sqlrec.out_ident_ind != SQL_NULL_DATA)
                c_st.set_ident(cur.sqlrec.out_ident);

            // Fill in datetime
            c_st.set_year(cur.sqlrec.out_datetime.year);
            c_st.set_month(cur.sqlrec.out_datetime.month);
            c_st.set_day(cur.sqlrec.out_datetime.day);
            c_st.set_hour(cur.sqlrec.out_datetime.hour);
            c_st.set_minute(cur.sqlrec.out_datetime.minute);
            c_st.set_second(cur.sqlrec.out_datetime.second);

            // Fill in station information
            station_cache.to_context(c_st);

            // Update current context information
            last_datetime = cur.sqlrec.out_datetime;
            last_ana_id = cur.sqlrec.out_ana_id;
            last_rep_cod = cur.sqlrec.out_rep_cod;
        }

        TRACE("Inserting var B%02d%03d (%s)\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()), var->value());
        if (cur.sqlrec.out_id_ltr == -1)
        {
            msg->set(move(var), Level::ana(), Trange::ana());
        } else {
            msg::Context* ctx = ltrc.to_msg(cur.sqlrec.out_id_ltr, *msg);
            if (ctx)
                ctx->set(move(var));
        }
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
