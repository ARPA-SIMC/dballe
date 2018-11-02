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

void Transaction::add_msg_to_batch(Tracer<>& trc, const Message& message, const DBImportMessageOptions& opts)
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
    if (!opts.report.empty())
        report = opts.report;
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

    if (opts.update_station || (station->is_new && station->id == MISSING_INT))
    {
        for (const auto& var: l_ana->data)
        {
            Varcode code = var->code();

            // Do not import datetime in the station info context, unless it has attributes
            if (code == WR_VAR(0, 4, 1) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 2) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 3) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 4) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 5) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 6) && !var->next_attr()) continue;

            station->get_station_data(trc).add(var, opts.overwrite ? batch::UPDATE : batch::IGNORE);
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
            md->add(id_levtr, var, opts.overwrite ? batch::UPDATE : batch::IGNORE);
        }
    }
}

void Transaction::import_message(const Message& message, const DBImportMessageOptions& opts)
{
    Tracer<> trc(this->trc ? this->trc->trace_import(1) : nullptr);

    batch.set_write_attrs(opts.import_attributes);

    add_msg_to_batch(trc, message, opts);

    // Run the bulk insert
    batch.write_pending(trc);
}

void Transaction::import_messages(const Messages& messages, const DBImportMessageOptions& opts)
{
    Tracer<> trc(this->trc ? this->trc->trace_import(messages.size()) : nullptr);

    batch.set_write_attrs(opts.import_attributes);

    for (const auto& i: messages)
        add_msg_to_batch(trc, *i, opts);

    // Run the bulk insert
    batch.write_pending(trc);
}

}
}
}
