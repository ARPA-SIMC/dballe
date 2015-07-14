/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "msg/wr_codec.h"
#include "msg/msg.h"
#include "msg/context.h"
#include <wreport/bulletin.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

#define GENERIC_NAME "generic"
#define GENERIC_DESC "Generic (255.0)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

struct Generic : public Template
{
    Bulletin* bulletin;

    Generic(const Exporter::Options& opts, const Messages& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return GENERIC_NAME; }
    virtual const char* description() const { return GENERIC_DESC; }

    void add(Varcode code, int shortcut)
    {
        const Var* var = msg->find_by_id(shortcut);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add(Varcode code, Varcode srccode, const Level& level, const Trange& trange)
    {
        const Var* var = msg->get(srccode, level, trange);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add_var_and_attrs(const Var& var)
    {
        // Store the variable
        subset->store_variable(var.code(), var);

        // Store the attributes
        for (const Var* attr = var.next_attr(); attr != NULL; attr = attr->next_attr())
        {
            if (WR_VAR_X(attr->code()) != 33)
                // Skip non-B33yyy attributes, as they won't be decoded properly
                if (WR_VAR_X(attr->code()) != 33)
                    continue;
                    //error_consistency::throwf("attempt to encode attribute B%02d%03d which is not B33YYY",
                    //        WR_VAR_X(attr->code()), WR_VAR_Y(attr->code()));
            subset->store_variable(attr->code(), *attr);
        }
    }

    void add_datetime_var(Varcode code, bool has_default, int def)
    {
        if (const Var* var = find_station_var(code))
            add_var_and_attrs(*var);
        else if (has_default)
            subset->store_variable_i(code, def);
        else
            subset->store_variable_undef(code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        bulletin.data_category = 255;
        bulletin.data_subcategory = 255;
        bulletin.data_subcategory_local = 0;
        bulletin.originating_centre = 200;
        bulletin.originating_subcentre = 0;

        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table_version_number = 14;
            b->master_table_version_number_local = 1;
        }
        if (CrexBulletin* b = dynamic_cast<CrexBulletin*>(&bulletin))
        {
            // TODO: change to BUFR-like info when we can encode BUFR table info in Crex
            b->master_table_version_number = 99;
        }

        // The data descriptor section will be generated later, as it depends
        // on the contents of the message

        bulletin.load_tables();

        // Store a pointer to it because we modify it later
        this->bulletin = &bulletin;
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        const Var* repmemo = msg.get_rep_memo_var();
        Level lev;
        Trange tr;

        // Report type
        if (repmemo)
            subset.store_variable(repmemo->code(), *repmemo);
        else if (msg.type != MSG_GENERIC) // It is generic by default, no need to repeat it
            subset.store_variable_c(WR_VAR(0, 1, 194), Msg::repmemo_from_type(msg.type));
        else
            subset.store_variable_undef(WR_VAR(0, 1, 194));

        // Datetime
        Datetime dt = msg.get_datetime();
        add_datetime_var(WR_VAR(0, 4, 1), !dt.is_missing(), dt.year);
        add_datetime_var(WR_VAR(0, 4, 2), !dt.is_missing(), dt.month);
        add_datetime_var(WR_VAR(0, 4, 3), !dt.is_missing(), dt.day);
        add_datetime_var(WR_VAR(0, 4, 4), !dt.is_missing(), dt.hour);
        add_datetime_var(WR_VAR(0, 4, 5), !dt.is_missing(), dt.minute);
        add_datetime_var(WR_VAR(0, 4, 6), !dt.is_missing(), dt.second);

        // Then the station context
        if (const msg::Context* ctx = msg.find_station_context())
        {
            for (size_t j = 0; j < ctx->data.size(); ++j)
            {
                const Var& var = *(ctx->data[j]);

                // Do not add rep_memo and datetime twice
                switch (var.code())
                {
                    case WR_VAR(0, 1, 194):
                    case WR_VAR(0, 4, 1):
                    case WR_VAR(0, 4, 2):
                    case WR_VAR(0, 4, 3):
                    case WR_VAR(0, 4, 4):
                    case WR_VAR(0, 4, 5):
                    case WR_VAR(0, 4, 6):
                        continue;
                    default:
                        break;
                }

                // Store the variable
                add_var_and_attrs(var);
            }
        }

        // Then do the other contexts
        for (size_t i = 0; i < msg.data.size(); ++i)
        {
            const msg::Context& ctx = *msg.data[i];
            if (ctx.is_station()) continue;

            for (size_t j = 0; j < ctx.data.size(); ++j)
            {
                const Var& var = *ctx.data[j];
                if (!var.isset()) continue; // Don't add undef vars
                if (&var == repmemo) continue; // Don't add rep_memo again

                /* Update the context in the message, if needed */
                if (lev.ltype1 != ctx.level.ltype1)
                {
                    if (ctx.level.ltype1 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 7, 192));
                    else
                        subset.store_variable_i(WR_VAR(0, 7, 192), ctx.level.ltype1);
                    lev.ltype1 = ctx.level.ltype1;
                }
                if (lev.l1 != ctx.level.l1)
                {
                    if (ctx.level.l1 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 7, 193));
                    else
                        subset.store_variable_i(WR_VAR(0, 7, 193), ctx.level.l1);
                    lev.l1 = ctx.level.l1;
                }
                if (lev.ltype2 != ctx.level.ltype2)
                {
                    if (ctx.level.ltype2 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 7, 195));
                    else
                        subset.store_variable_i(WR_VAR(0, 7, 195), ctx.level.ltype2);
                    lev.ltype2 = ctx.level.ltype2;
                }
                if (lev.l2 != ctx.level.l2)
                {
                    if (ctx.level.l2 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 7, 194));
                    else
                        subset.store_variable_i(WR_VAR(0, 7, 194), ctx.level.l2);
                    lev.l2 = ctx.level.l2;
                }
                if (tr.pind != ctx.trange.pind)
                {
                    if (ctx.trange.pind == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 4, 192));
                    else
                        subset.store_variable_i(WR_VAR(0, 4, 192), ctx.trange.pind);
                    tr.pind = ctx.trange.pind;
                }
                if (tr.p1 != ctx.trange.p1)
                {
                    if (ctx.trange.p1 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 4, 193));
                    else
                        subset.store_variable_i(WR_VAR(0, 4, 193), ctx.trange.p1);
                    tr.p1 = ctx.trange.p1;
                }
                if (tr.p2 != ctx.trange.p2)
                {
                    if (ctx.trange.p2 == MISSING_INT)
                        subset.store_variable_undef(WR_VAR(0, 4, 194));
                    else
                        subset.store_variable_i(WR_VAR(0, 4, 194), ctx.trange.p2);
                    tr.p2 = ctx.trange.p2;
                }

                // Store the variable
                add_var_and_attrs(var);
            }
        }

        // Generate data descriptor section
        bulletin->datadesc.clear();
        for (size_t i = 0; i < subset.size(); ++i)
            bulletin->datadesc.push_back(subset[i].code());
    }
};

} // anonymous namespace

void register_generic(TemplateRegistry& r)
{
    r.register_factory(255, GENERIC_NAME, GENERIC_DESC,
            [](const Exporter::Options& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Generic(opts, msgs));
            });
}

}
}
}
