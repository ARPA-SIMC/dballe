#include "db.h"
#include "cursor.h"
#include "qbuilder.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include "dballe/core/query.h"
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


struct StationValues : protected std::vector<wreport::Var*>
{
    Tracer<>& trc;

    StationValues(Tracer<>& trc) : trc(trc) {}
    ~StationValues()
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

    void read(v7::Transaction& tr, int id_station)
    {
        reset();
        tr.station().get_station_vars(trc, id_station, [&](std::unique_ptr<wreport::Var> var) {
            push_back(var.release());
        });
    }
};

struct DataRow
{
    dballe::DBStation station;
    int id_levtr;
    Datetime datetime;
    int id_data;
    wreport::Var* var;

    DataRow(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)
        : station(station), id_levtr(id_levtr), datetime(datetime), id_data(id_data), var(var.release())
    {
    }
    DataRow(const DataRow&) = delete;
    DataRow(DataRow&& o)
        : station(move(o.station)), id_levtr(o.id_levtr), datetime(move(o.datetime)), id_data(o.id_data), var(o.var)
    {
        o.var = nullptr;
    }
    ~DataRow()
    {
        delete var;
    }
    DataRow& operator=(const DataRow&) = delete;
    DataRow& operator=(DataRow&& o)
    {
        if (this == &o) return *this;
        delete var;
        var = nullptr;
        station = o.station;
        id_levtr = o.id_levtr;
        datetime = o.datetime;
        id_data = o.id_data;
        var = o.var;
        o.var = nullptr;
        return *this;
    }

    std::unique_ptr<wreport::Var> release_var()
    {
        std::unique_ptr<wreport::Var> res(var);
        var = nullptr;
        return res;
    }
};

}

bool Transaction::export_msgs(const dballe::Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest)
{
    Tracer<> trc(this->trc ? this->trc->trace_export_msgs(query) : nullptr);
    v7::LevTr& lt = levtr();

    // Message being built
    unique_ptr<Msg> msg;

    // The big export query
    DataQueryBuilder qb(dynamic_pointer_cast<v7::Transaction>(shared_from_this()), core::Query::downcast(query), DBA_DB_MODIFIER_SORT_FOR_EXPORT | DBA_DB_MODIFIER_WITH_ATTRIBUTES, false);
    qb.build();

    // Current context information used to detect context changes
    Datetime last_datetime;
    int last_ana_id = -1;

    StationValues station_values(trc);

    if (db->explain_queries)
    {
        fprintf(stderr, "EXPLAIN "); query.print(stderr);
        db->conn->explain(qb.sql_query, stderr);
    }

    // Retrieve results, buffering them locally to avoid performing concurrent
    // queries
    std::vector<DataRow> results;
    data().run_data_query(trc, qb, [&](const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var) {
        results.emplace_back(station, id_levtr, datetime, id_data, move(var));
    });

    for (auto& row: results)
    {
        //TRACE("Got B%02d%03d %ld,%ld, %ld,%ld %ld,%ld,%ld %s\n",
        //        WR_VAR_X(sqlrec.out_varcode), WR_VAR_Y(sqlrec.out_varcode),
        //        sqlrec.out_ltype1, sqlrec.out_l1, sqlrec.out_ltype2, sqlrec.out_l2, sqlrec.out_pind, sqlrec.out_p1, sqlrec.out_p2,
        //        sqlrec.out_value);

        /* See if we have the start of a new message */
        if (row.station.id != last_ana_id || row.datetime != last_datetime)
        {
            // Flush current message
            TRACE("New message\n");
            if (msg.get() != NULL)
            {
                TRACE("Sending old message to consumer\n");
                if (msg->type == MessageType::PILOT || msg->type == MessageType::TEMP || msg->type == MessageType::TEMP_SHIP)
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
            msg->set_datetime(row.datetime);

            msg::Context& c_st = msg->obtain_station_context();

            // Update station layer cache if needed
            if (row.station.id != last_ana_id)
                station_values.read(*this, row.station.id);

            // Fill in report information
            c_st.set(newvar(WR_VAR(0, 1, 194), row.station.report));
            msg->type = Msg::type_from_repmemo(row.station.report.c_str());

            // Fill in the basic station values
            c_st.set(newvar(WR_VAR(0, 5, 1), row.station.coords.lat));
            c_st.set(newvar(WR_VAR(0, 6, 1), row.station.coords.lon));
            if (!row.station.ident.is_missing())
                c_st.set(newvar(WR_VAR(0, 1, 11), (const char*)row.station.ident));

            // Fill in station information
            station_values.to_context(c_st);

            // Update current context information
            last_datetime = row.datetime;
            last_ana_id = row.station.id;
        }

        TRACE("Inserting var %01d%02d%03d (%s)\n", WR_VAR_FXY(var->code()), var->enqc());
        msg::Context* ctx = lt.to_msg(trc, row.id_levtr, *msg);
        if (ctx)
            ctx->set(row.release_var());
    }

    if (msg.get() != NULL)
    {
        TRACE("Inserting leftover old message\n");
        if (msg->type == MessageType::PILOT || msg->type == MessageType::TEMP || msg->type == MessageType::TEMP_SHIP)
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

    return true;
}

}
}
}
