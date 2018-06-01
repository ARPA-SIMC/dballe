#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/batch.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <cassert>

using namespace wreport;
using dballe::sql::Connection;

namespace dballe {
namespace db {
namespace v7 {

void Transaction::import_msg(const Message& message, const char* repmemo, int flags)
{
    const Msg& msg = Msg::downcast(message);
    const msg::Context* l_ana = msg.find_context(Level(), Trange());
    if (!l_ana)
        throw error_consistency("cannot import into the database a message without station information");

    v7::Batch batch(std::dynamic_pointer_cast<v7::Transaction>(shared_from_this()));
    std::shared_ptr<batch::Station> station;

    // Latitude
    Coords coords;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        coords.lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");

    // Longitude
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        coords.lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Report code
    std::string report;
    if (repmemo != NULL)
        report = repmemo;
    else {
        if (const Var* var = msg.get_rep_memo_var())
            report = var->enqc();
        else
            report = Msg::repmemo_from_type(msg.type);
    }

    // Station identifier
    if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
        station = batch.get_station(report, coords, var->enqc());
    else
        station = batch.get_station(report, coords);

    if (flags & DBA_IMPORT_FULL_PSEUDOANA || (station->is_new && station->id == MISSING_INT))
    {
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;

            station->station_data.add(*l_ana->data[i], flags & DBA_IMPORT_OVERWRITE, flags & DBA_IMPORT_ATTRS);
        }
    }

    // Fill the bulk insert with the rest of the data
    v7::LevTr& lt = levtr();
    Datetime datetime = msg.get_datetime();
    if (datetime.is_missing())
        throw error_notfound("date/time informations not found (or incomplete) in message to insert");
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        if (msg.data[i] == l_ana) continue;
        const msg::Context& ctx = *msg.data[i];

        // Get the database ID of the lev_tr
        int id_levtr = lt.obtain_id(LevTrEntry(ctx.level, ctx.trange));

        batch::MeasuredData& md = station->get_measured_data(id_levtr, datetime);

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            md.add(*var, flags & DBA_IMPORT_OVERWRITE, flags & DBA_IMPORT_ATTRS);
        }
    }

    // Run the bulk insert
    batch.commit();

#if 0
    v7::Station& st = station();
    v7::LevTr& lt = levtr();

    // Fill up the station informations needed to fetch an existing ID
    dballe::Station station;

    // Latitude
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        station.coords.lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");

    // Longitude
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        station.coords.lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Station identifier
    if (mobile)
    {
        if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
            station.ident = var->enqc();
        else
            throw error_notfound("mobile station identifier not found in data to import");
    }

    // Report code
    if (repmemo != NULL)
        station.report = repmemo;
    else {
        if (const Var* var = msg.get_rep_memo_var())
            station.report = var->enqc();
        else
            station.report = Msg::repmemo_from_type(msg.type);
    }

    int station_id = st.obtain_id(*this, station);

    if (flags & DBA_IMPORT_FULL_PSEUDOANA || st.is_newly_inserted(station_id))
    {
        // Prepare a bulk insert
        v7::StationData& sd = station_data();
        v7::bulk::InsertStationVars vars(station_id);
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;
            vars.add(l_ana->data[i]);
        }

        // Run the bulk insert
        sd.insert(*this, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::bulk::UPDATE : v7::bulk::IGNORE, flags & DBA_IMPORT_ATTRS);
    }

    v7::Data& dd = data();

    v7::bulk::InsertVars vars(station_id);

    // Fill the bulk insert with the rest of the data
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        const msg::Context& ctx = *msg.data[i];
        bool is_ana_level = ctx.level == Level() && ctx.trange == Trange();

        // Skip the station info level
        if (is_ana_level) continue;

        // Date and time
        if (not vars.has_datetime())
        {
            vars.set_datetime(msg.get_datetime());
            if (not vars.has_datetime())
                throw error_notfound("date/time informations not found (or incomplete) in message to insert");
        }

        // Get the database ID of the lev_tr
        auto id_levtr = lt.obtain_id(LevTrEntry(ctx.level, ctx.trange));

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            vars.add(var, id_levtr);
        }
    }

    // Run the bulk insert
    dd.insert(*this, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::bulk::UPDATE : v7::bulk::IGNORE, flags & DBA_IMPORT_ATTRS);
#endif
}

}
}
}
