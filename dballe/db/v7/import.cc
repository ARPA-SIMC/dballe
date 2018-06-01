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
        station = batch.get_station(report, coords, var->enqc());
    else
        station = batch.get_station(report, coords, Ident());

    if (flags & DBA_IMPORT_FULL_PSEUDOANA || (station->is_new && station->id == MISSING_INT))
    {
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;

            station->get_station_data().add(l_ana->data[i], flags & DBA_IMPORT_OVERWRITE);
        }
    }

    // Fill the bulk insert with the rest of the data
    v7::LevTr& lt = levtr();
    Datetime datetime = msg.get_datetime();
    if (datetime.is_missing())
        throw error_notfound("date/time informations not found (or incomplete) in message to insert");
    batch::MeasuredData& md = station->get_measured_data(datetime);
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        if (msg.data[i] == l_ana) continue;
        const msg::Context& ctx = *msg.data[i];

        // Get the database ID of the lev_tr
        int id_levtr = lt.obtain_id(LevTrEntry(ctx.level, ctx.trange));

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            md.add(id_levtr, var, flags & DBA_IMPORT_OVERWRITE);
        }
    }

    // Run the bulk insert
    batch.commit(flags & DBA_IMPORT_ATTRS);
}

}
}
}
