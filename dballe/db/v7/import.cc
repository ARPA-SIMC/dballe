#include "db.h"
#include "dballe/sql/sql.h"
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
    v7::DataV7& dd = data();
    v7::AttrV7& dq = attr();

    dballe::sql::Transaction* t = dynamic_cast<dballe::sql::Transaction*>(&transaction);
    assert(t);

    // Fill up the pseudoana informations needed to fetch an existing ID

    // Latitude
    int lat;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LATITUDE))
        lat = var->enqi();
    else
        throw error_notfound("latitude not found in data to import");

    // Longitude
    int lon;
    if (const Var* var = l_ana->find_by_id(DBA_MSG_LONGITUDE))
        lon = var->enqi();
    else
        throw error_notfound("longitude not found in data to import");

    // Station identifier
    const char* ident = NULL;
    if (mobile)
    {
        if (const Var* var = l_ana->find_by_id(DBA_MSG_IDENT))
            ident = var->enqc();
        else
            throw error_notfound("mobile station identifier not found in data to import");
    }

    // Report code
    int id_report;
    if (repmemo != NULL)
        id_report = rep_cod_from_memo(repmemo);
    else {
        // TODO: check if B01194 first
        if (const Var* var = msg.get_rep_memo_var())
            id_report = rep_cod_from_memo(var->enqc());
        else
            id_report = rep_cod_from_memo(Msg::repmemo_from_type(msg.type));
    }

    bool inserted_pseudoana = false;
    int id_station = st.obtain_id(id_report, lat, lon, ident, &inserted_pseudoana);

    if ((flags & DBA_IMPORT_FULL_PSEUDOANA) || inserted_pseudoana)
    {
        // Prepare a bulk insert
        v7::bulk::InsertV7 vars;
        vars.id_station = id_station;
        vars.id_report = id_report;
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
        dd.insert(*t, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::DataV7::UPDATE : v7::DataV7::IGNORE);

        // Insert the attributes
        if (flags & DBA_IMPORT_ATTRS)
        {
#if 0
            for (const auto& v: vars)
                if (v.inserted())
                    dq.add(v.id_data, *v.var);
#else
            v7::bulk::InsertAttrsV7 attrs;
            for (const auto& v: vars)
            {
                if (!v.inserted()) continue;
                attrs.add_all(*v.var, v.id_data);
            }
            if (!attrs.empty())
                dq.insert(*t, attrs, v7::AttrV7::UPDATE);
#endif
        }
    }

    v7::bulk::InsertV7 vars;
    vars.id_station = id_station;
    vars.id_report = id_report;

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
        int id_lev_tr = lt.obtain_id(ctx.level, ctx.trange);

        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            const Var* var = ctx.data[j];
            if (not var->isset()) continue;
            vars.add(var, id_lev_tr);
        }
    }

    // Run the bulk insert
    dd.insert(*t, vars, (flags & DBA_IMPORT_OVERWRITE) ? v7::DataV7::UPDATE : v7::DataV7::IGNORE);

    // Insert the attributes
    if (flags & DBA_IMPORT_ATTRS)
    {
#if 0
        for (const auto& v: vars)
            if (v.inserted())
                dq.add(v.id_data, *v.var);
#else
        v7::bulk::InsertAttrsV7 attrs;
        for (const auto& v: vars)
        {
            if (!v.inserted()) continue;
            attrs.add_all(*v.var, v.id_data);
        }
        if (!attrs.empty())
            dq.insert(*t, attrs, v7::AttrV7::UPDATE);
#endif
    }
}

}
}
}
