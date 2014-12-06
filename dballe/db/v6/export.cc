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
#include "internals.h"
#include "qbuilder.h"
#include "dballe/db/modifiers.h"
#include "dballe/db/sql.h"

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

        db.station().get_station_vars(id_station, id_report, [&](std::unique_ptr<wreport::Var> var) {
            push_back(var.release());
        });
    }
};

}

static inline int sqltimecmp(const SQL_TIMESTAMP_STRUCT* a, const SQL_TIMESTAMP_STRUCT* b)
{
    return memcmp(a, b, sizeof(SQL_TIMESTAMP_STRUCT));
}

void DB::export_msgs(const Query& query, MsgConsumer& consumer)
{
    v5::Repinfo& ri = repinfo();
    Attr& at = attr();
    LevTrCache& ltrc = lev_tr_cache();

    // Message being built
    unique_ptr<Msg> msg;

    auto t = conn->transaction();

    // The big export query
    auto stm = conn->statement();
    stm->set_cursor_forward_only();

    DataQueryBuilder qb(*this, *stm, query, DBA_DB_MODIFIER_SORT_FOR_EXPORT);
    qb.build();
    stm->prepare(qb.sql_query);

    // Current context information used to detect context changes
    Datetime last_datetime;
    int last_ana_id = -1;
    int last_rep_cod = -1;

    StationLayerCache station_cache;

    // Retrieve results
    run_built_query(
            qb.stm,
            qb.select_station, qb.select_varinfo, qb.select_data_id, qb.select_data,
            [&](SQLRecord& sqlrec) {
        //TRACE("Got B%02d%03d %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
        //        WR_VAR_X(sqlrec.out_varcode), WR_VAR_Y(sqlrec.out_varcode),
        //        sqlrec.out_ltype1, sqlrec.out_l1, sqlrec.out_ltype2, sqlrec.out_l2, sqlrec.out_pind, sqlrec.out_p1, sqlrec.out_p2,
        //        sqlrec.out_value);

        /* Create the variable that we got on this iteration */
        unique_ptr<Var> var(newvar(sqlrec.out_varcode, sqlrec.out_value));

        /* Load the attributes from the database */
        at.read(sqlrec.out_id_data, [&](unique_ptr<Var> attr) { var->seta(auto_ptr<Var>(attr.release())); });

        /* See if we have the start of a new message */
        if (sqlrec.out_ana_id != last_ana_id
         || sqlrec.out_rep_cod != last_rep_cod
         || sqlrec.out_datetime != last_datetime)
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
            if (sqlrec.out_ana_id != last_ana_id || sqlrec.out_rep_cod != last_rep_cod)
                station_cache.fill(*this, sqlrec.out_ana_id, sqlrec.out_rep_cod);

            // Fill in report information
            {
                const char* memo = ri.get_rep_memo(sqlrec.out_rep_cod);
                c_st.set_rep_memo(memo);
                msg->type = Msg::type_from_repmemo(memo);
            }

            // Fill in the basic station values
            c_st.seti(WR_VAR(0, 5, 1), sqlrec.out_lat);
            c_st.seti(WR_VAR(0, 6, 1), sqlrec.out_lon);
            if (sqlrec.out_ident_size != -1)
                c_st.set_ident(sqlrec.out_ident);

            // Fill in datetime
            c_st.set_year(sqlrec.out_datetime.date.year);
            c_st.set_month(sqlrec.out_datetime.date.month);
            c_st.set_day(sqlrec.out_datetime.date.day);
            c_st.set_hour(sqlrec.out_datetime.time.hour);
            c_st.set_minute(sqlrec.out_datetime.time.minute);
            c_st.set_second(sqlrec.out_datetime.time.second);

            // Fill in station information
            station_cache.to_context(c_st);

            // Update current context information
            last_datetime = sqlrec.out_datetime;
            last_ana_id = sqlrec.out_ana_id;
            last_rep_cod = sqlrec.out_rep_cod;
        }

        TRACE("Inserting var B%02d%03d (%s)\n", WR_VAR_X(var->code()), WR_VAR_Y(var->code()), var->value());
        if (sqlrec.out_id_ltr == -1)
        {
            msg->set(move(var), Level::ana(), Trange::ana());
        } else {
            msg::Context* ctx = ltrc.to_msg(sqlrec.out_id_ltr, *msg);
            if (ctx)
                ctx->set(move(var));
        }
    });

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
