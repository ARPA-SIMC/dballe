#include "db.h"
#include "dballe/sql/sql.h"
#include "dballe/db/v7/transaction.h"
#include "dballe/db/v7/driver.h"
#include "dballe/db/v7/station.h"
#include "dballe/db/v7/levtr.h"
#include "dballe/db/v7/data.h"
#include "dballe/db/v7/attr.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <cassert>

using namespace wreport;
using dballe::sql::Connection;

namespace dballe {
namespace db {
namespace v7 {

void DB::import_msg(dballe::Transaction& transaction, const Message& message, const char* repmemo, int flags)
{
    const Msg& msg = Msg::downcast(message);
    const msg::Context* l_ana = msg.find_context(Level(), Trange());
	if (!l_ana)
		throw error_consistency("cannot import into the database a message without station information");

	// Check if the station is mobile
    bool mobile = msg.get_ident_var() != NULL;

    v7::Station& st = station();
    v7::LevTr& lt = lev_tr();
    v7::Data& dd = data();

    auto& t = v7::Transaction::downcast(transaction);

    // Fill up the station informations needed to fetch an existing ID
    StationDesc station_desc;

    // Latitude
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        station_desc.coords.lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");

    // Longitude
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        station_desc.coords.lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Station identifier
    if (mobile)
    {
        if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
            station_desc.ident = var->enqc();
        else
            throw error_notfound("mobile station identifier not found in data to import");
    }

    // Report code
    if (repmemo != NULL)
        station_desc.rep = rep_cod_from_memo(repmemo);
    else {
        // TODO: check if B01194 first
        if (const Var* var = msg.get_rep_memo_var())
            station_desc.rep = rep_cod_from_memo(var->enqc());
        else
            station_desc.rep = rep_cod_from_memo(Msg::repmemo_from_type(msg.type));
    }

    auto sstate = st.obtain_id(t.state, station_desc);

    if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || sstate->second.is_new)
    {
        // Prepare a bulk insert
        v7::bulk::InsertVars vars;
        vars.station = sstate->second;
        vars.datetime = Datetime(1000, 1, 1, 0, 0, 0);
        for (size_t i = 0; i < l_ana->data.size(); ++i)
        {
            Varcode code = l_ana->data[i]->code();
            // Do not import datetime in the station info context
            if (code >= WR_VAR(0, 4, 1) && code <= WR_VAR(0, 4, 6))
                continue;
            vars.add(l_ana->data[i], -1);
        }

        // Run the bulk insert
        dd.insert(t, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::bulk::UPDATE : v7::bulk::IGNORE);

        // Insert the attributes
        if (flags & DBA_IMPORT_ATTRS)
        {
            v7::Attr& a = station_attr();
            v7::bulk::InsertAttrsV7 attrs;
            for (const auto& v: vars)
            {
                if (!v.inserted()) continue;
                attrs.add_all(*v.var, v.id_data);
            }
            if (!attrs.empty())
                a.insert(t, attrs, v7::Attr::UPDATE);
        }
    }

    v7::bulk::InsertVars vars;
    vars.station = sstate->second;

    // Date and time
    if (msg.get_datetime().is_missing())
        throw error_notfound("date/time informations not found (or incomplete) in message to insert");
    vars.datetime = msg.get_datetime();

    // Fill the bulk insert with the rest of the data
    for (size_t i = 0; i < msg.data.size(); ++i)
    {
        const msg::Context& ctx = *msg.data[i];
        bool is_ana_level = ctx.level == Level() && ctx.trange == Trange();

        // Skip the station info level
        if (is_ana_level) continue;

        // Get the database ID of the lev_tr
        auto levtri = lt.obtain_id(t.state, LevTrDesc(ctx.level, ctx.trange));

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            vars.add(var, levtri->second.id);
        }
    }

    // Run the bulk insert
    dd.insert(t, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::bulk::UPDATE : v7::bulk::IGNORE);

    // Insert the attributes
    if (flags & DBA_IMPORT_ATTRS)
    {
        v7::Attr& a = attr();
        v7::bulk::InsertAttrsV7 attrs;
        for (const auto& v: vars)
        {
            if (!v.inserted()) continue;
            attrs.add_all(*v.var, v.id_data);
        }
        if (!attrs.empty())
            a.insert(t, attrs, v7::Attr::UPDATE);
    }
}

}
}
}
