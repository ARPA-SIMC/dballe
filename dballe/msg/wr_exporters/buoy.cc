/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <wreport/bulletin.h>
#include "msg/msgs.h"
#include <cstdlib>

using namespace wreport;
using namespace std;

#define BUOY_NAME "buoy"
#define BUOY_DESC "Buoy (1.21)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

struct Buoy : public Template
{
    bool is_crex;

    Buoy(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return BUOY_NAME; }
    virtual const char* description() const { return BUOY_DESC; }

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
        const Var* var = msg->find(srccode, level, trange);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 1;
        bulletin.subtype = 255;
        bulletin.localsubtype = 21;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  8,   3));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 201));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        /*  0 */
        const Var* var = msg.get_ident_var();
        const char* val = var->value();
        if (val != NULL)
        {
                subset.store_variable_i(WR_VAR(0, 1, 5), strtol(val, 0, 10));
                subset.back().copy_attrs(*var);
        } else
                subset.store_variable_undef(WR_VAR(0, 1, 5));
        /*  1 */ add(WR_VAR(0,  1, 12), DBA_MSG_ST_DIR);
        /*  2 */ add(WR_VAR(0,  1, 13), DBA_MSG_ST_SPEED);
        /*  3 */ add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        do_D01011();
        do_D01012();
        /*  9 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /* 10 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /* 11 */ add(WR_VAR(0, 10,  4), DBA_MSG_PRESS);
        /* 12 */ add(WR_VAR(0, 10, 51), DBA_MSG_PRESS_MSL);
        /* 13 */ add(WR_VAR(0, 10, 61), DBA_MSG_PRESS_3H);
        /* 14 */ add(WR_VAR(0, 10, 63), DBA_MSG_PRESS_TEND);
        /* 15 */ add(WR_VAR(0, 11, 11), DBA_MSG_WIND_DIR);
        /* 16 */ add(WR_VAR(0, 11, 12), DBA_MSG_WIND_SPEED);
        /* 17 */ add(WR_VAR(0, 12,  4), DBA_MSG_TEMP_2M);
        /* 18 */ add(WR_VAR(0, 12,  6), DBA_MSG_DEWPOINT_2M);
        /* 19 */ add(WR_VAR(0, 13,  3), DBA_MSG_HUMIDITY);
        /* 20 */ add(WR_VAR(0, 20,  1), DBA_MSG_VISIBILITY);
        /* 21 */ add(WR_VAR(0, 20,  3), DBA_MSG_PRES_WTR);
        do_ecmwf_past_wtr();
        /* 24 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 25 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 26 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 27 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 28 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 29 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 30 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        /* 31 */ add(WR_VAR(0, 22, 42), DBA_MSG_WATER_TEMP);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 32);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 201), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 201));
        }
    }
};

struct BuoyFactory : public TemplateFactory
{
    BuoyFactory() { name = BUOY_NAME; description = BUOY_DESC; }

    std::unique_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return unique_ptr<Template>(new Buoy(opts, msgs));
    }
};

} // anonymous namespace

void register_buoy(TemplateRegistry& r)
{
static const TemplateFactory* buoy = NULL;

    if (!buoy) buoy = new BuoyFactory;

    r.register_factory(buoy);
}

}
}
}

/* vim:set ts=4 sw=4: */
