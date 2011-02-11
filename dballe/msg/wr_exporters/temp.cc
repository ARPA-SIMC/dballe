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

using namespace wreport;
using namespace std;

#define TEMP_NAME "temp"
#define TEMP_DESC "Temp (2.101)"

#define TEMP_SHIP_NAME "temp-ship"
#define TEMP_SHIP_DESC "Temp ship (2.102)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

// Base template for ships
struct TempBase : public Template
{
    bool is_crex;

    TempBase(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

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

    void add(Varcode code, Varcode srccode, const msg::Context& ctx)
    {
        const Var* var = ctx.find(srccode);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    /// Count the number of sounding levels
    int count_levels() const
    {
        int lev_no = 0;
        for (size_t i = 0; i < msg->data.size(); ++i)
        {
            if (msg->data[i]->find_vsig() != NULL)
                ++lev_no;
        }
        return lev_no;
    }

    int add_sounding_levels()
    {
        int count = count_levels();
        subset->store_variable_i(WR_VAR(0, 31,  1), count);

        // Iterate backwards as we need to add levels in decreasing pressure order
        for (int i = msg->data.size() - 1; i >= 0; --i)
        {
            msg::Context& c = *msg->data[i];

            // We only want levels with a vertical sounding significance
            const Var* vss = c.find_vsig();
            if (vss == NULL) continue;

            /* We only want pressure levels */
            if (c.level.ltype1 != 100) continue;
            double press = c.level.l1;

            /* Add pressure */
            if (const Var* var = c.find(WR_VAR(0, 10, 4)))
                subset->store_variable(WR_VAR(0,  7,  4), *var);
            else
                subset->store_variable_d(WR_VAR(0,  7,  4), press);

            /* Add vertical sounding significance */
            subset->store_variable(WR_VAR(0,  8,  1), *vss);

            /* Add the rest */
            add(WR_VAR(0, 10, 3), WR_VAR(0, 10,   8), c);
            add(WR_VAR(0, 12, 1), WR_VAR(0, 12, 101), c);
            add(WR_VAR(0, 12, 3), WR_VAR(0, 12, 103), c);
            add(WR_VAR(0, 11, 1), WR_VAR(0, 11,   1), c);
            add(WR_VAR(0, 11, 2), WR_VAR(0, 11,   2), c);
        }

        return count;
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
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);
    }
};

struct Temp : public TempBase
{
    Temp(const Exporter::Options& opts, const Msgs& msgs)
        : TempBase(opts, msgs) {}

    virtual const char* name() const { return TEMP_NAME; }
    virtual const char* description() const { return TEMP_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        TempBase::setupBulletin(bulletin);
        bulletin.localsubtype = 101;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  9,  7));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,   0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,   2));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,   0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,   2));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        TempBase::to_subset(msg, subset);
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
        /* 12 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 13 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 14 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 15 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 16 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 17 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 18 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        int lev_no = add_sounding_levels();
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

struct TempShip : public TempBase
{
    TempShip(const Exporter::Options& opts, const Msgs& msgs)
        : TempBase(opts, msgs) {}

    virtual const char* name() const { return TEMP_SHIP_NAME; }
    virtual const char* description() const { return TEMP_SHIP_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        TempBase::setupBulletin(bulletin);
        bulletin.localsubtype = 102;

        // Data descriptor section
        bulletin.datadesc.clear();

        /* FIXME: some expansions are not in CREX D tables yet, so we need to use
         * the expanded version here.  The code needs to be changed when newer CREX
         * D tables are available */
        /* WR_VAR(3,  9, 196), Replaced with expansion: */
                bulletin.datadesc.push_back(WR_VAR(3, 1,  3));
                bulletin.datadesc.push_back(WR_VAR(0, 2, 11));
                bulletin.datadesc.push_back(WR_VAR(0, 2, 12));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 11));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 12));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 23));
                bulletin.datadesc.push_back(WR_VAR(0, 7,  1));
        bulletin.datadesc.push_back(WR_VAR(3,  2,  4));
        bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
        bulletin.datadesc.push_back(WR_VAR(0, 31,  1));
        bulletin.datadesc.push_back(WR_VAR(3,  3, 14));
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
        TempBase::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1, 11), DBA_MSG_IDENT);
        /*  1 */ add(WR_VAR(0,  1, 12), DBA_MSG_ST_DIR);
        /*  2 */ add(WR_VAR(0,  1, 13), DBA_MSG_ST_SPEED);
        /*  3 */ add(WR_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE);
        /*  4 */ add(WR_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD);
        /*  5 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  6 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  7 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  8 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  9 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /* 10 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /* 11 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /* 12 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT);
        /* 13 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 14 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 15 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 16 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 17 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 18 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 19 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        int lev_no = add_sounding_levels();
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

struct TempFactory : public TemplateFactory
{
    TempFactory() { name = TEMP_NAME; description = TEMP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Temp(opts, msgs));
    }
};
struct TempShipFactory : public TemplateFactory
{
    TempShipFactory() { name = TEMP_SHIP_NAME; description = TEMP_SHIP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempShip(opts, msgs));
    }
};

} // anonymous namespace

void register_temp(TemplateRegistry& r)
{
static const TemplateFactory* temp = NULL;
static const TemplateFactory* tempship = NULL;

    if (!temp) temp = new TempFactory;
    if (!tempship) tempship = new TempShipFactory;

    r.register_factory(temp);
    r.register_factory(tempship);
}

}
}
}

/* vim:set ts=4 sw=4: */
