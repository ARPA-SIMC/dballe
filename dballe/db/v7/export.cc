#include "db.h"
#include "cursor.h"
#include "qbuilder.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/repinfo.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/attr.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include "dballe/core/query.h"
#include "dballe/core/structbuf.h"
#include <memory>
#include <cstring>
#include <iostream>

using namespace wreport;
using namespace std;
using dballe::sql::Connection;

namespace dballe {
namespace db {
namespace v7 {

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

    void fill(DB& db, int id_station)
    {
        reset();

        db.station().get_station_vars(id_station, [&](std::unique_ptr<wreport::Var> var) {
            push_back(var.release());
        });
    }
};

}

bool DB::export_msgs(dballe::Transaction& transaction, const dballe::Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest)
{
    auto tr = trace.trace_export_msgs(query);
    v7::Repinfo& ri = repinfo();
    v7::Data& da = data();
    v7::LevTr& lt = lev_tr();

    auto& t = v7::Transaction::downcast(transaction);

    // Message being built
    unique_ptr<Msg> msg;

    // The big export query
    // TODO: also select attrs
    DataQueryBuilder qb(*this, core::Query::downcast(query), DBA_DB_MODIFIER_SORT_FOR_EXPORT, false);
    qb.build();

    // Current context information used to detect context changes
    Datetime last_datetime;
    int last_ana_id = -1;
    int last_rep_cod = -1;

    StationLayerCache station_cache;

    // Retrieve results, buffering them locally to avoid performing concurrent
    // queries
    Structbuf<v7::SQLRecordV7> results;
    driver().run_built_query_v7(qb, [&](v7::SQLRecordV7& sqlrec) {
        results.append(sqlrec);
    });
    results.ready_to_read();

    for (unsigned row = 0; row < results.size(); ++row)
    {
        const v7::SQLRecordV7& sqlrec = results[row];

        //TRACE("Got B%02d%03d %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
        //        WR_VAR_X(sqlrec.out_varcode), WR_VAR_Y(sqlrec.out_varcode),
        //        sqlrec.out_ltype1, sqlrec.out_l1, sqlrec.out_ltype2, sqlrec.out_l2, sqlrec.out_pind, sqlrec.out_p1, sqlrec.out_p2,
        //        sqlrec.out_value);

        /* Create the variable that we got on this iteration */
        unique_ptr<Var> var(newvar(sqlrec.out_varcode, sqlrec.out_value));

        /* Load the attributes from the database */
        da.read_attrs(sqlrec.out_id_data, [&](unique_ptr<Var> attr) { var->seta(move(attr)); });

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
                    if (!dest(move(copy)))
                        return false;
                } else
                    if (!dest(move(msg)))
                        return false;
            }

            // Start writing a new message
            msg.reset(new Msg);

            // Fill in datetime
            msg->set_datetime(sqlrec.out_datetime);

            msg::Context& c_st = msg->obtain_station_context();

            // Update station layer cache if needed
            if (sqlrec.out_ana_id != last_ana_id)
                station_cache.fill(*this, sqlrec.out_ana_id);

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

            // Fill in station information
            station_cache.to_context(c_st);

            // Update current context information
            last_datetime = sqlrec.out_datetime;
            last_ana_id = sqlrec.out_ana_id;
            last_rep_cod = sqlrec.out_rep_cod;
        }

        TRACE("Inserting var %01d%02d%03d (%s)\n", WR_VAR_FXY(var->code()), var->enqc());
        if (sqlrec.out_id_ltr == -1)
        {
            msg->set(move(var), Level(), Trange());
        } else {
            msg::Context* ctx = lt.to_msg(t.state, sqlrec.out_id_ltr, *msg);
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
            if (!dest(move(copy)))
                return false;
            msg.release();
        } else
            if (!dest(move(msg)))
                return false;
    }

    // Useful for Oracle to end the session
    tr->done();
    return true;
}

}
}
}
