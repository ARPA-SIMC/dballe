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

void Transaction::add_msg_to_batch(Tracer<>& trc, const Message& message, const dballe::DBImportOptions& opts)
{
    const impl::Message& msg = impl::Message::downcast(message);

    batch::Station* station;

    // Coordinates
    Coords coords = msg.get_coords();
    if (coords.is_missing())
        throw error_notfound("coordinates not found in data to import");

    // Report code
    std::string report;
    if (!opts.report.empty())
        report = opts.report;
    else
        report = msg.get_report();

    // Station identifier
    Ident ident = msg.get_ident();
    station = batch.get_station(trc, report, coords, ident);

    if (opts.update_station || (station->is_new && station->id == MISSING_INT))
    {
        for (const auto& var: msg.station_data)
        {
            Varcode code = var->code();

            // Do not import datetime in the station info context, unless it has attributes
            if (code == WR_VAR(0, 4, 1) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 2) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 3) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 4) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 5) && !var->next_attr()) continue;
            if (code == WR_VAR(0, 4, 6) && !var->next_attr()) continue;

            station->get_station_data(trc).add(var.get(), opts.overwrite ? batch::UPDATE : batch::IGNORE);
        }
    }

    // Fill the bulk insert with the rest of the data
    v7::LevTr& lt = levtr();
    // Defer creation of MeasuredData to prevent complaining about missing
    // datetime info if we have no data to import
    batch::MeasuredData* md = nullptr;
    for (const auto& ctx: msg.data)
    {
        int id_levtr = -1;

        for (const auto& val: ctx.values)
        {
            if (not val->isset()) continue;
            if (!opts.varlist.empty() && std::find(opts.varlist.begin(), opts.varlist.end(), val->code()) == opts.varlist.end())
                continue;

            if (!md)
            {
                Datetime datetime = msg.get_datetime();
                if (datetime.is_missing())
                    throw error_notfound("date/time informations not found (or incomplete) in message to insert");
                md = &station->get_measured_data(trc, datetime);
            }

            if (id_levtr == -1)
            {
                // Get the database ID of the lev_tr
                id_levtr = lt.obtain_id(trc, LevTrEntry(ctx.level, ctx.trange));
            }

            md->add(id_levtr, val.get(), opts.overwrite ? batch::UPDATE : batch::IGNORE);
        }
    }
}

void Transaction::import_message(const dballe::Message& message, const dballe::DBImportOptions& opts)
{
    Tracer<> trc(this->trc ? this->trc->trace_import(1) : nullptr);

    batch.set_write_attrs(opts.import_attributes);

    add_msg_to_batch(trc, message, opts);

    // Run the bulk insert
    batch.write_pending(trc);
}

void Transaction::import_messages(const std::vector<std::shared_ptr<dballe::Message>>& messages, const dballe::DBImportOptions& opts)
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
