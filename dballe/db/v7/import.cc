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

void Transaction::add_msg_to_batch(Tracer<>& trc, const Message& message, const char* repmemo, int flags)
{
    const Msg& msg = Msg::downcast(message);
    const msg::Context* l_ana = msg.find_context(Level(), Trange());
    if (!l_ana)
        throw error_consistency("cannot import into the database a message without station information");

    batch::Station* station;

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
        station = batch.get_station(trc, report, coords, var->enqc());
    else
        station = batch.get_station(trc, report, coords, Ident());

    if (flags & DBA_IMPORT_FULL_PSEUDOANA || (station->is_new && station->id == MISSING_INT))
    {
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;

            station->get_station_data(trc).add(l_ana->data[i], flags & DBA_IMPORT_OVERWRITE ? batch::UPDATE : batch::IGNORE);
        }
    }

    // Fill the bulk insert with the rest of the data
    v7::LevTr& lt = levtr();
    // Defer creation of MeasuredData to prevent complaining about missing
    // datetime info if we have no data to import
    batch::MeasuredData* md = nullptr;
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        if (msg.data[i] == l_ana) continue;
        const msg::Context& ctx = *msg.data[i];

        if (!md)
        {
            Datetime datetime = msg.get_datetime();
            if (datetime.is_missing())
                throw error_notfound("date/time informations not found (or incomplete) in message to insert");
            md = &station->get_measured_data(trc, datetime);
        }

        // Get the database ID of the lev_tr
        int id_levtr = lt.obtain_id(trc, LevTrEntry(ctx.level, ctx.trange));

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            md->add(id_levtr, var, flags & DBA_IMPORT_OVERWRITE ? batch::UPDATE : batch::IGNORE);
        }
    }
}

void Transaction::import_msg(const Message& message, const char* repmemo, int flags)
{
    Tracer<> trc(this->trc ? this->trc->trace_import(1) : nullptr);

    batch.set_write_attrs(flags & DBA_IMPORT_ATTRS);

    add_msg_to_batch(trc, message, repmemo, flags);

    // Run the bulk insert
    batch.write_pending(trc);
}

void Transaction::import_msgs(const Messages& msgs, const char* repmemo, int flags)
{
    Tracer<> trc(this->trc ? this->trc->trace_import(msgs.size()) : nullptr);

    batch.set_write_attrs(flags & DBA_IMPORT_ATTRS);

    for (const auto& i: msgs)
        add_msg_to_batch(trc, *i, repmemo, flags);

    // Run the bulk insert
    batch.write_pending(trc);
}

}
}
}
