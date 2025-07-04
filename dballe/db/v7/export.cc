#include "cursor.h"
#include "db.h"
#include "dballe/core/query.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include "dballe/sql/sql.h"
#include "qbuilder.h"
#include <cstring>
#include <iostream>
#include <map>
#include <memory>

using namespace wreport;
using namespace std;
using dballe::sql::Connection;

namespace dballe {
namespace db {
namespace v7 {

namespace {

struct StationValues : public Values
{
    Tracer<>& trc;

    StationValues(Tracer<>& trc) : trc(trc) {}

    void read(v7::Transaction& tr, int id_station)
    {
        clear();
        tr.station().get_station_vars(
            trc, id_station,
            [&](std::unique_ptr<wreport::Var> var) { set(std::move(var)); });
    }
};

struct ProtoVar
{
    int id_levtr;
    std::unique_ptr<wreport::Var> var;
    ProtoVar(int id_levtr, std::unique_ptr<wreport::Var> var)
        : id_levtr(id_levtr), var(std::move(var))
    {
    }
};

struct ProtoMessage
{
    std::unique_ptr<impl::Message> msg;
    std::vector<ProtoVar> vars;
    ProtoMessage() : msg(new impl::Message) {}
};

struct Cursor : public impl::CursorMessage
{
    typedef std::vector<std::shared_ptr<dballe::Message>> Results;
    Results results;
    Results::iterator cur;
    bool at_start = true;

    bool has_value() const override
    {
        return !at_start && cur != results.end();
    }

    std::shared_ptr<Message> get_message() const override { return *cur; }

    int remaining() const override
    {
        if (at_start)
            return results.size();
        return results.end() - cur;
    }

    bool next() override
    {
        if (at_start)
        {
            cur      = results.begin();
            at_start = false;
        }
        else if (cur != results.end())
        {
            cur->reset();
            ++cur;
        }
        return cur != results.end();
    }

    void discard() override
    {
        results.clear();
        cur = results.end();
    }

    DBStation get_station() const override
    {
        DBStation res;
        res.coords = (*cur)->get_coords();
        res.ident  = (*cur)->get_ident();
        res.report = (*cur)->get_report();
        return res;
    }
};

} // namespace

std::shared_ptr<dballe::CursorMessage>
Transaction::query_messages(const Query& query)
{
    Tracer<> trc(this->trc ? this->trc->trace_export_msgs(query) : nullptr);
    v7::LevTr& lt = levtr();

    // The big export query
    DataQueryBuilder qb(
        dynamic_pointer_cast<v7::Transaction>(shared_from_this()),
        core::Query::downcast(query),
        DBA_DB_MODIFIER_SORT_FOR_EXPORT | DBA_DB_MODIFIER_WITH_ATTRIBUTES,
        false);
    qb.build();

    // Current context information used to detect context changes
    Datetime last_datetime;
    int last_ana_id = -1;

    StationValues station_values(trc);

    if (db->explain_queries)
    {
        fprintf(stderr, "EXPLAIN ");
        query.print(stderr);
        db->conn->explain(qb.sql_query, stderr);
    }

    // Retrieve results, buffering them locally to avoid performing concurrent
    // queries
    std::map<int, std::vector<ProtoMessage>> results;
    std::set<int> id_levtrs;
    ProtoMessage* msg;
    data().run_data_query(
        trc, qb,
        [&](const dballe::DBStation& station, int id_levtr,
            const Datetime& datetime, int id_data,
            std::unique_ptr<wreport::Var> var) {
            if (station.id != last_ana_id || datetime != last_datetime)
            {
                auto& vec = results[station.id];
                vec.emplace_back();
                msg = &vec.back();
                msg->msg->set_datetime(datetime);
                msg->msg->station_data.set(
                    newvar(WR_VAR(0, 1, 194), station.report));
                msg->msg->type =
                    impl::Message::type_from_repmemo(station.report.c_str());
                msg->msg->station_data.set(
                    newvar(WR_VAR(0, 5, 1), station.coords.lat));
                msg->msg->station_data.set(
                    newvar(WR_VAR(0, 6, 1), station.coords.lon));
                if (!station.ident.is_missing())
                    msg->msg->station_data.set(
                        newvar(WR_VAR(0, 1, 11), (const char*)station.ident));
                last_datetime = datetime;
                last_ana_id   = station.id;
            }
            id_levtrs.insert(id_levtr);
            msg->vars.emplace_back(id_levtr, std::move(var));
        });

    lt.prefetch_ids(trc, id_levtrs);

    auto res = std::make_shared<Cursor>();
    for (auto& r : results)
    {
        station_values.read(*this, r.first);
        for (auto& msg : r.second)
        {
            // Fill in station information
            msg.msg->station_data.merge(station_values);

            // Move variables to contexts
            int last_id_levtr       = -1;
            impl::msg::Context* ctx = nullptr;
            for (auto& pvar : msg.vars)
            {
                if (pvar.id_levtr != last_id_levtr)
                {
                    ctx           = lt.to_msg(trc, pvar.id_levtr, *msg.msg);
                    last_id_levtr = pvar.id_levtr;
                }
                ctx->values.set(std::move(pvar.var));
            }
            msg.vars.clear();

            // Send message to consumer
            if (msg.msg->type == MessageType::PILOT ||
                msg.msg->type == MessageType::TEMP ||
                msg.msg->type == MessageType::TEMP_SHIP)
                msg.msg->sounding_pack_levels();

            res->results.emplace_back(std::move(msg.msg));
        }
        r.second.clear();
    }
    results.clear();

    return res;
}

} // namespace v7
} // namespace db
} // namespace dballe
