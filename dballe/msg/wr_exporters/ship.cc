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
#include <wreport/bulletin.h>
#include "msg/msgs.h"
#include "msg/context.h"

using namespace wreport;
using namespace std;

#define SHIP_NAME "ship"
#define SHIP_DESC "Synop ship (autodetect)"

#define SHIP_PLAIN_NAME "ship-plain"
#define SHIP_PLAIN_DESC "Synop ship (normal) (1.11)"

#define SHIP_ABBR_NAME "ship-abbr"
#define SHIP_ABBR_DESC "Synop ship (abbreviated) (1.9)"

#define SHIP_AUTO_NAME "ship-auto"
#define SHIP_AUTO_DESC "Synop ship (auto) (1.13)"

#define SHIP_REDUCED_NAME "ship-reduced"
#define SHIP_REDUCED_DESC "Synop ship (reduced) (1.19)"

#define SHIP_WMO_NAME "ship-wmo"
#define SHIP_WMO_DESC "Ship WMO"

namespace dballe {
namespace msg {
namespace wr {

namespace {

// Base template for ships
struct ShipBase : public Template
{
    bool is_crex;

    ShipBase(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

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

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  8,   4));
        bulletin.datadesc.push_back(WR_VAR(0, 12,   5));
        bulletin.datadesc.push_back(WR_VAR(0, 10, 197));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  34));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  34));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wind = NULL;
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->find(WR_VAR(0, 11, 1)) || c->find(WR_VAR(0, 11, 2)))
                c_wind = c;
        }

        /*  0 */ add(WR_VAR(0,  1, 11), DBA_MSG_IDENT);
        /*  1 */ add(WR_VAR(0,  1, 12), DBA_MSG_ST_DIR);
        /*  2 */ add(WR_VAR(0,  1, 13), DBA_MSG_ST_SPEED);
        /*  3 */ add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        /*  4 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  5 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  6 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  7 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  8 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  9 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /* 10 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /* 11 */ add(WR_VAR(0, 10,  4), DBA_MSG_PRESS);
        /* 12 */ add(WR_VAR(0, 10, 51), DBA_MSG_PRESS_MSL);
        /* 13 */ add(WR_VAR(0, 10, 61), DBA_MSG_PRESS_3H);
        /* 14 */ add(WR_VAR(0, 10, 63), DBA_MSG_PRESS_TEND);
        /* 15 */ add(WR_VAR(0, 11, 11), c_wind, DBA_MSG_WIND_DIR);
        /* 16 */ add(WR_VAR(0, 11, 12), c_wind, DBA_MSG_WIND_SPEED);
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
        /* 32 */ add(WR_VAR(0, 12,  5), DBA_MSG_WET_TEMP_2M);
        /* 33 */ add(WR_VAR(0, 10,197), DBA_MSG_HEIGHT_ANEM);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 34);
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

struct ShipAbbr : public ShipBase
{
    ShipAbbr(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_ABBR_NAME; }
    virtual const char* description() const { return SHIP_ABBR_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);
        bulletin.localsubtype = 9;

        bulletin.load_tables();
    }
};

struct ShipPlain : public ShipBase
{
    ShipPlain(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_PLAIN_NAME; }
    virtual const char* description() const { return SHIP_PLAIN_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);
        bulletin.localsubtype = 11;

        bulletin.load_tables();
    }
};

struct ShipAuto : public ShipBase
{
    ShipAuto(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_AUTO_NAME; }
    virtual const char* description() const { return SHIP_AUTO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);
        bulletin.localsubtype = 13;

        bulletin.load_tables();
    }
};

struct ShipReduced : public ShipBase
{
    ShipReduced(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_REDUCED_NAME; }
    virtual const char* description() const { return SHIP_REDUCED_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);
        bulletin.localsubtype = 19;

        bulletin.load_tables();
    }
};

// Template for WMO ships
struct ShipWMO : public Template
{
    bool is_crex;

    ShipWMO(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return SHIP_WMO_NAME; }
    virtual const char* description() const { return SHIP_WMO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 1;
        bulletin.subtype = 0;
        bulletin.localsubtype = 255;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  8,   9));

        bulletin.load_tables();
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wind = NULL;
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->find(WR_VAR(0, 11, 1)) || c->find(WR_VAR(0, 11, 2)))
                c_wind = c;
        }

        // Ship identification
        add(WR_VAR(0,  1,  11), DBA_MSG_IDENT);
        add(WR_VAR(0,  1,  12), DBA_MSG_ST_DIR);
        add(WR_VAR(0,  1,  13), DBA_MSG_ST_SPEED);
        add(WR_VAR(0,  2,   1), DBA_MSG_ST_TYPE);
        add(WR_VAR(0,  4,   1), DBA_MSG_YEAR);
        add(WR_VAR(0,  4,   2), DBA_MSG_MONTH);
        add(WR_VAR(0,  4,   3), DBA_MSG_DAY);
        add(WR_VAR(0,  4,   4), DBA_MSG_HOUR);
        add(WR_VAR(0,  4,   5), DBA_MSG_MINUTE);
        add(WR_VAR(0,  5,   2), DBA_MSG_LATITUDE);
        add(WR_VAR(0,  6,   2), DBA_MSG_LONGITUDE);
        add(WR_VAR(0,  7,  30), DBA_MSG_HEIGHT_STATION);
        add(WR_VAR(0,  7,  31), DBA_MSG_HEIGHT_BARO);
        // Pressure data
        add(WR_VAR(0, 10,   4), DBA_MSG_PRESS);
        add(WR_VAR(0, 10,  51), DBA_MSG_PRESS_MSL);
        add(WR_VAR(0, 10,  61), DBA_MSG_PRESS_3H);
        add(WR_VAR(0, 10,  63), DBA_MSG_PRESS_TEND);
        // Temperature and humidity data
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  33)); // FIXME
        add(WR_VAR(0, 12, 101), DBA_MSG_TEMP_2M);
        subset.store_variable_undef(WR_VAR(0,  2,  39)); // FIXME
        add(WR_VAR(0, 12, 102), DBA_MSG_WET_TEMP_2M);
        add(WR_VAR(0, 12, 103), DBA_MSG_DEWPOINT_2M);
        add(WR_VAR(0, 13,   3), DBA_MSG_HUMIDITY);
        // Visibility data
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  33)); // FIXME
        add(WR_VAR(0, 20,   1), DBA_MSG_VISIBILITY);
        subset.store_variable_undef(WR_VAR(0,  7,  33));
        // Precipitation past 24 hours
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 13,  23)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  32));
        // Cloud data
        /* 24 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 25 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 26 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 27 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 28 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 29 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 30 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        subset.store_variable_i(WR_VAR(0, 31,   1), 0); // FIXME
        subset.store_variable_undef(WR_VAR(0,  8,   2));
        // Icing and ice
        subset.store_variable_undef(WR_VAR(0, 20,  31)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  33)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  34)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  35)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  36)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  37)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 20,  38)); // FIXME
        // Ship marine data
        subset.store_variable_undef(WR_VAR(0,  2,  38)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  63)); // FIXME
        add(WR_VAR(0, 22, 43), DBA_MSG_WATER_TEMP);
        subset.store_variable_undef(WR_VAR(0,  7,  63));
        // Waves
        subset.store_variable_undef(WR_VAR(0, 22,   1)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  11)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  21)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,   2)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  12)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  22)); // FIXME
        //  2 systems of swell waves
        subset.store_variable_undef(WR_VAR(0, 22,   3)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  13)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  23)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,   3)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  13)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 22,  23)); // FIXME
        // Ship "period" data
        add(WR_VAR(0, 20,  3), DBA_MSG_PRES_WTR);
        int hour = 0;
        if (const Var* var = msg.get_hour_var())
            hour = var->enqi();
        if (hour % 6 == 0)
        {
            subset.store_variable_i(WR_VAR(0, 4, 24), 6);
            add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1_6H);
            add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2_6H);
        } else {
            subset.store_variable_i(WR_VAR(0, 4, 24), 3);
            add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1_3H);
            add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2_3H);
        }
        // Precipitation measurement
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 13,  11)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 13,  11)); // FIXME
        // Extreme temperature data
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  33)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 12, 111)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  24)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 12, 112)); // FIXME
        // Wind data
        subset.store_variable_undef(WR_VAR(0,  7,  32)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  7,  33)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  2,   2)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  8,  21)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  25)); // FIXME
        add(WR_VAR(0, 11,  1), c_wind, DBA_MSG_WIND_DIR);
        add(WR_VAR(0, 11,  2), c_wind, DBA_MSG_WIND_SPEED);
        subset.store_variable_undef(WR_VAR(0,  8,  21));
        subset.store_variable_undef(WR_VAR(0,  4,  25)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 11,  43)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 11,  41)); // FIXME
        subset.store_variable_undef(WR_VAR(0,  4,  25)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 11,  43)); // FIXME
        subset.store_variable_undef(WR_VAR(0, 11,  41)); // FIXME

        // /* 33 */ add(WR_VAR(0, 10,197), DBA_MSG_HEIGHT_ANEM);
    }
};


struct ShipFactory : public TemplateFactory
{
    ShipFactory() { name = SHIP_NAME; description = SHIP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        // Scan msgs and pick the right one
        const Msg& msg = *msgs[0];
        const Var* var = msg.get_st_type_var();
        if (var == NULL || var->enqi() == 1)
            return auto_ptr<Template>(new ShipPlain(opts, msgs));
        else
            return auto_ptr<Template>(new ShipAuto(opts, msgs));
    }
};

struct ShipPlainFactory : public TemplateFactory
{
    ShipPlainFactory() { name = SHIP_PLAIN_NAME; description = SHIP_PLAIN_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipPlain(opts, msgs));
    }
};
struct ShipAbbrFactory : public TemplateFactory
{
    ShipAbbrFactory() { name = SHIP_ABBR_NAME; description = SHIP_ABBR_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipAbbr(opts, msgs));
    }
};
struct ShipAutoFactory : public TemplateFactory
{
    ShipAutoFactory() { name = SHIP_AUTO_NAME; description = SHIP_AUTO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipAuto(opts, msgs));
    }
};
struct ShipReducedFactory : public TemplateFactory
{
    ShipReducedFactory() { name = SHIP_REDUCED_NAME; description = SHIP_REDUCED_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipReduced(opts, msgs));
    }
};
struct ShipWMOFactory : public TemplateFactory
{
    ShipWMOFactory() { name = SHIP_WMO_NAME; description = SHIP_WMO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipWMO(opts, msgs));
    }
};

} // anonymous namespace

void register_ship(TemplateRegistry& r)
{
static const TemplateFactory* ship = NULL;
static const TemplateFactory* shipplain = NULL;
static const TemplateFactory* shipabbr = NULL;
static const TemplateFactory* shipauto = NULL;
static const TemplateFactory* shipreduced = NULL;
static const TemplateFactory* shipwmo = NULL;

    if (!ship) ship = new ShipFactory;
    if (!shipplain) shipplain = new ShipPlainFactory;
    if (!shipabbr) shipabbr = new ShipAbbrFactory;
    if (!shipauto) shipauto = new ShipAutoFactory;
    if (!shipreduced) shipreduced = new ShipReducedFactory;
    if (!shipwmo) shipwmo = new ShipWMOFactory;

    r.register_factory(ship);
    r.register_factory(shipplain);
    r.register_factory(shipabbr);
    r.register_factory(shipauto);
    r.register_factory(shipreduced);
    r.register_factory(shipwmo);
}

}
}
}

/* vim:set ts=4 sw=4: */
