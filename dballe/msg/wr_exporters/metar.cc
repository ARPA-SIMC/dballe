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

#define METAR_NAME "metar"
#define METAR_DESC "Metar (0.140)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

struct Metar : public Template
{
    bool is_crex;

    Metar(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return METAR_NAME; }
    virtual const char* description() const { return METAR_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 0;
        bulletin.subtype = 255;
        bulletin.localsubtype = 140;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  7,  11));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  21));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  21));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wtr = NULL;
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->find(WR_VAR(0, 20, 9)))
                c_wtr = c;
        }

        /*  0 */ add(WR_VAR(0,  1, 63), DBA_MSG_ST_NAME_ICAO);
        /*  1 */ add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        /*  2 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  3 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  4 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  5 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  6 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  7 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /*  8 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /*  9 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT_STATION);
        /* 10 */ subset.store_variable_i(WR_VAR(0,  7,  6), 10);
        /* 11 */ add(WR_VAR(0, 11,  1), DBA_MSG_WIND_DIR);
        /* 12 */ add(WR_VAR(0, 11, 16), DBA_MSG_EX_CCW_WIND);
        /* 13 */ add(WR_VAR(0, 11, 17), DBA_MSG_EX_CW_WIND);
        /* 14 */ add(WR_VAR(0, 11,  2), DBA_MSG_WIND_SPEED);
        /* 15 */ add(WR_VAR(0, 11, 41), DBA_MSG_WIND_GUST_MAX_SPEED);
        /* 16 */ subset.store_variable_i(WR_VAR(0,  7,  6), 2);
        /* 15 */ add(WR_VAR(0, 12,  1), DBA_MSG_TEMP_2M);
        /* 16 */ add(WR_VAR(0, 12,  3), DBA_MSG_DEWPOINT_2M);
        /* 17 */ add(WR_VAR(0, 10, 52), DBA_MSG_QNH);
        /* 18 */ add(WR_VAR(0, 20,  9), c_wtr, DBA_MSG_METAR_WTR);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 21);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
        }
    }
};

struct MetarFactory : public TemplateFactory
{
    MetarFactory() { name = METAR_NAME; description = METAR_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Metar(opts, msgs));
    }
};

} // anonymous namespace

void register_metar(TemplateRegistry& r)
{
static const TemplateFactory* metar = NULL;

    if (!metar) metar = new MetarFactory;
 
    r.register_factory(metar);
}

}
}
}

/* vim:set ts=4 sw=4: */
