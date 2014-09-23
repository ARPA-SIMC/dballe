/*
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "common.h"
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

#define SHIP_ECMWF_SECOND_NAME "ship-second"
#define SHIP_ECMWF_SECOND_DESC "Synop ship (second record) (1.12)"

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
    CommonSynopExporter synop;
    bool is_crex;

    ShipBase(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 1;
        bulletin.subtype = 255;
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);
        synop.init(subset);

        // Scan message finding context for the data that follow
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
            synop.scan_context(**i);
    }
};

struct ShipECMWFBase : public ShipBase
{
    ShipECMWFBase(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

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
        ShipBase::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wind = NULL;
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->find(WR_VAR(0, 11, 1)) || c->find(WR_VAR(0, 11, 2)))
                c_wind = c;
        }

        synop.add_ship_head();
        synop.add_year_to_minute();
        synop.add_latlon_coarse();
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

struct ShipAbbr : public ShipECMWFBase
{
    ShipAbbr(const Exporter::Options& opts, const Msgs& msgs)
        : ShipECMWFBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_ABBR_NAME; }
    virtual const char* description() const { return SHIP_ABBR_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipECMWFBase::setupBulletin(bulletin);
        bulletin.localsubtype = 9;

        bulletin.load_tables();
    }
};

struct ShipPlain : public ShipECMWFBase
{
    ShipPlain(const Exporter::Options& opts, const Msgs& msgs)
        : ShipECMWFBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_PLAIN_NAME; }
    virtual const char* description() const { return SHIP_PLAIN_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipECMWFBase::setupBulletin(bulletin);
        bulletin.localsubtype = 11;

        bulletin.load_tables();
    }
};

struct ShipAuto : public ShipECMWFBase
{
    ShipAuto(const Exporter::Options& opts, const Msgs& msgs)
        : ShipECMWFBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_AUTO_NAME; }
    virtual const char* description() const { return SHIP_AUTO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipECMWFBase::setupBulletin(bulletin);
        bulletin.localsubtype = 13;

        bulletin.load_tables();
    }
};

struct ShipReduced : public ShipECMWFBase
{
    ShipReduced(const Exporter::Options& opts, const Msgs& msgs)
        : ShipECMWFBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_REDUCED_NAME; }
    virtual const char* description() const { return SHIP_REDUCED_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipECMWFBase::setupBulletin(bulletin);
        bulletin.localsubtype = 19;

        bulletin.load_tables();
    }
};

struct ShipECMWFSecondRecord : public ShipBase
{
    ShipECMWFSecondRecord(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_ECMWF_SECOND_NAME; }
    virtual const char* description() const { return SHIP_ECMWF_SECOND_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

        bulletin.type = 1;
        bulletin.subtype = 255;
        bulletin.localsubtype = 12;
        bulletin.load_tables();

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  1,  36));
        bulletin.datadesc.push_back(WR_VAR(0, 12,  15));
        bulletin.datadesc.push_back(WR_VAR(0, 12,  14));
        bulletin.datadesc.push_back(WR_VAR(3,  2,  24));
        bulletin.datadesc.push_back(WR_VAR(0, 22,   1));
        bulletin.datadesc.push_back(WR_VAR(0, 22,  11));
        bulletin.datadesc.push_back(WR_VAR(0, 22,  21));
        bulletin.datadesc.push_back(WR_VAR(0, 13,  31));
        bulletin.datadesc.push_back(WR_VAR(0, 14,  15));
        bulletin.datadesc.push_back(WR_VAR(0, 14,  31));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  63));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  63));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  63));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  63));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  33));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  31));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  32));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  34));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  37));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  38));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  36));
        if (!is_crex)
        {
            bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  39));
            bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  39));
            bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        ShipBase::to_subset(msg, subset);

        synop.add_ship_head();
        synop.add_year_to_minute();
        synop.add_latlon_coarse();

        // TODO
        subset.store_variable_undef(WR_VAR(0, 12,  15)); // MINIMUM TEMPERATURE AT 2M, PAST 12 HOURS
        subset.store_variable_undef(WR_VAR(0, 12,  14)); // MAXIMUM TEMPERATURE AT 2M, PAST 12 HOURS
        synop.add_D02024();
        synop.add_plain_waves();
        // TODO
        subset.store_variable_undef(WR_VAR(0, 13,  31)); // EVAPOTRANSPIRATION
        subset.store_variable_undef(WR_VAR(0, 14,  15)); // NET RADIATION INTEGRATED OVER 24HOURS
        subset.store_variable_undef(WR_VAR(0, 14,  31)); // TOTAL SUNSHINE
        subset.store_variable_undef(WR_VAR(0, 20,  63)); // SPECIAL PHENOMENA
        subset.store_variable_undef(WR_VAR(0, 20,  63)); // SPECIAL PHENOMENA
        subset.store_variable_undef(WR_VAR(0, 20,  63)); // SPECIAL PHENOMENA
        subset.store_variable_undef(WR_VAR(0, 20,  63)); // SPECIAL PHENOMENA
        synop.add_ecmwf_ice();

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 39);
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

// Template for WMO ships
struct ShipWMO : public ShipBase
{
    bool is_crex;

    ShipWMO(const Exporter::Options& opts, const Msgs& msgs)
        : ShipBase(opts, msgs) {}

    virtual const char* name() const { return SHIP_WMO_NAME; }
    virtual const char* description() const { return SHIP_WMO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        ShipBase::setupBulletin(bulletin);

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
        ShipBase::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wind = NULL;
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->find(WR_VAR(0, 11, 1)) || c->find(WR_VAR(0, 11, 2)))
                c_wind = c;
        }

        // Ship identification, movement, date/time, horizontal and vertical
        // coordinates
        synop.add_D01093();

        // Pressure data
        synop.add_D02001();

        // Temperature and humidity data
        synop.add_D02052();

        // Visibility data
        synop.add_D02053();

        // Precipitation past 24 hours
        synop.add_D02034();

        // Cloud data
        synop.add_cloud_data();
        subset.store_variable_undef(WR_VAR(0,  8,   2));

        // Icing and ice
        synop.add_D02055();
        // Ship marine data
        synop.add_D02056();
        // Waves
        synop.add_plain_waves();
        // Wind and swell waves
        synop.add_D02024();
        // Ship "period" data
        synop.add_D02038();
        // Precipitation measurement
        synop.add_D02040();
        // Extreme temperature data
        synop.add_D02058();
        // Wind data
        synop.add_D02059();
    }
};


struct ShipFactory : public TemplateFactory
{
    ShipFactory() { name = SHIP_NAME; description = SHIP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        // Scan msgs and pick the right one
        bool maybe_wmo = true;
        bool maybe_plain = true;
        bool maybe_auto = true;
        bool maybe_second = true;
        const Msg& msg = *msgs[0];
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context& c = **i;
            switch (c.level.ltype1)
            {
                case MISSING_INT:
                    for (std::vector<wreport::Var*>::const_iterator vi = c.data.begin();
                            vi != c.data.end(); ++vi)
                    {
                        switch ((*vi)->code())
                        {
                            case WR_VAR(0, 2, 1):
                                switch ((*vi)->enq(0))
                                {
                                    case 0: maybe_plain = false; break;
                                    case 1: maybe_auto = false; break;
                                }
                                break;
                            case WR_VAR(0, 2, 2):
                                maybe_plain = maybe_auto = maybe_second = false;
                                break;
                        }
                    }
                    break;
                case 264: maybe_plain = maybe_auto = false; break;
            }
        }
        //if (maybe_wmo)
        //    return auto_ptr<Template>(new ShipWMO(opts, msgs));
        if (maybe_plain)
            return auto_ptr<Template>(new ShipPlain(opts, msgs));
        if (maybe_auto)
            return auto_ptr<Template>(new ShipAuto(opts, msgs));
        if (maybe_second)
            return auto_ptr<Template>(new ShipECMWFSecondRecord(opts, msgs));
        // Fallback on WMO if we are confused
        return auto_ptr<Template>(new ShipWMO(opts, msgs));
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
struct ShipECMWFSecondRecordFactory : public TemplateFactory
{
    ShipECMWFSecondRecordFactory() { name = SHIP_ECMWF_SECOND_NAME; description = SHIP_ECMWF_SECOND_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new ShipECMWFSecondRecord(opts, msgs));
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
static const TemplateFactory* shipsecond = NULL;
static const TemplateFactory* shipabbr = NULL;
static const TemplateFactory* shipauto = NULL;
static const TemplateFactory* shipreduced = NULL;
static const TemplateFactory* shipwmo = NULL;

    if (!ship) ship = new ShipFactory;
    if (!shipplain) shipplain = new ShipPlainFactory;
    if (!shipsecond) shipsecond = new ShipECMWFSecondRecordFactory;
    if (!shipabbr) shipabbr = new ShipAbbrFactory;
    if (!shipauto) shipauto = new ShipAutoFactory;
    if (!shipreduced) shipreduced = new ShipReducedFactory;
    if (!shipwmo) shipwmo = new ShipWMOFactory;

    r.register_factory(ship);
    r.register_factory(shipplain);
    r.register_factory(shipsecond);
    r.register_factory(shipabbr);
    r.register_factory(shipauto);
    r.register_factory(shipreduced);
    r.register_factory(shipwmo);
}

}
}
}

/* vim:set ts=4 sw=4: */
