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

#include "wr_codec.h"
#include <wreport/bulletin.h>
#include "msgs.h"
#include "context.h"
#include <cstdlib>

using namespace wreport;
using namespace std;

#define PILOT_NAME "pilot"
#define PILOT_DESC "Pilot (2.91)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

struct Pilot : public Template
{
    bool is_crex;

    Pilot(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return PILOT_NAME; }
    virtual const char* description() const { return PILOT_DESC; }

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

        bulletin.type = 2;
        bulletin.subtype = 255;
        bulletin.localsubtype = 91;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  1,   1));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 11));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 12));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 11));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 12));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 22));
        bulletin.datadesc.push_back(WR_VAR(1,  5,  0));
        bulletin.datadesc.push_back(WR_VAR(0, 31,  1));
        bulletin.datadesc.push_back(WR_VAR(0,  7,  4));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  1));
        bulletin.datadesc.push_back(WR_VAR(0, 10,  3));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  1));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  2));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,  0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  2));
                bulletin.datadesc.push_back(WR_VAR(0, 31, 31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  2));
                bulletin.datadesc.push_back(WR_VAR(0, 33,  7));
        }

        bulletin.load_tables();
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        /*  0 */ add(WR_VAR(0,  1,  1), DBA_MSG_BLOCK);
        /*  1 */ add(WR_VAR(0,  1,  2), DBA_MSG_STATION);
        /*  2 */ add(WR_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE);
        /*  3 */ add(WR_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD);
        /*  4 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  5 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  6 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  7 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  8 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  9 */ add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        /* 10 */ add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
        /* 11 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT);

        int var_levcount = subset.size();
        subset.store_variable_undef(WR_VAR(0, 31,  1));

        /* Iterate backwards as we need to add levels in decreasing pressure order */
        int lev_no = 0;
        for (int i = msg.data.size() - 1; i >= 0; --i)
        {
            msg::Context& c = *msg.data[i];

            // We only want levels with a vertical sounding significance
            const Var* vss = c.find_vsig();
            if (vss == NULL) continue;

            /* Add pressure */
            if (const Var* var = c.find(WR_VAR(0, 10, 4)))
                subset.store_variable(WR_VAR(0,  7,  4), *var);
            else if (c.level.ltype1 == 100)
                subset.store_variable_d(WR_VAR(0,  7,  4), c.level.l1);
            else
                subset.store_variable_undef(WR_VAR(0, 7, 4));

            /* Add vertical sounding significance */
            subset.store_variable(WR_VAR(0, 8, 1), *vss);

            /* Add geopotential */
            if (const Var* var = c.find(WR_VAR(0, 10, 3)))
                subset.store_variable(WR_VAR(0, 10, 3), *var);
            else if (const Var* var = c.find(WR_VAR(0, 10, 8)))
                subset.store_variable(WR_VAR(0, 10, 3), *var);
            else if (c.level.ltype1 == 102)
                subset.store_variable_d(WR_VAR(0, 10, 3), (double)c.level.l1 * 9.80665);
            else
                subset.store_variable_undef(WR_VAR(0, 10, 3));

            /* Add wind direction */
            if (const Var* var = c.find(WR_VAR(0, 11, 1)))
                subset.store_variable(WR_VAR(0, 11, 1), *var);
            else
                subset.store_variable_undef(WR_VAR(0, 11, 1));

                /* Add wind speed */
            if (const Var* var = c.find(WR_VAR(0, 11, 2)))
                subset.store_variable(WR_VAR(0, 11, 2), *var);
            else
                subset.store_variable_undef(WR_VAR(0, 11, 2));

            ++lev_no;
        }

        subset[var_levcount].seti(lev_no);

        if (!is_crex)
        {
            int count = subset.append_dpb(WR_VAR(2, 22, 0), subset.size(), WR_VAR(0, 33, 7));
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
            subset.store_variable_i(WR_VAR(0, 31, 2), count);
        }
    }
};

struct PilotFactory : public TemplateFactory
{
    PilotFactory() { name = PILOT_NAME; description = PILOT_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Pilot(opts, msgs));
    }
};

} // anonymous namespace

void register_pilot(TemplateRegistry& r)
{
static const TemplateFactory* pilot = NULL;

    if (!pilot) pilot = new PilotFactory;

    r.register_factory(pilot);
}

}
}
}

/* vim:set ts=4 sw=4: */
