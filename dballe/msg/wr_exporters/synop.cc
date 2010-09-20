/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "base.h"
#include <wreport/bulletin.h>
#include "msgs.h"

using namespace wreport;
using namespace std;

#define SYNOP_NAME "synop"
#define SYNOP_DESC "Synop"

#define SYNOP_LAND_NAME "synop-land"
#define SYNOP_LAND_DESC "Synop land"

#define SYNOP_LAND_HIGH_NAME "synop-land-high"
#define SYNOP_LAND_HIGH_DESC "Synop land high level station"

#define SYNOP_AUTO_NAME "synop-auto"
#define SYNOP_AUTO_DESC "Synop land auto"

namespace dballe {
namespace msg {
namespace wr {

namespace {

// Base template for synops
struct Synop : public Template
{
    bool is_crex;
    Varcode prec_code;

    Synop(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return SYNOP_NAME; }
    virtual const char* description() const { return SYNOP_DESC; }

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

    void add_prec()
    {
        const Var* var = NULL;
        switch (prec_code)
        {
            case WR_VAR(0, 13, 23): var = msg->get_tot_prec24_var(); break;
            case WR_VAR(0, 13, 22): var = msg->get_tot_prec12_var(); break;
            case WR_VAR(0, 13, 21): var = msg->get_tot_prec6_var(); break;
            case WR_VAR(0, 13, 20): var = msg->get_tot_prec3_var(); break;
            case WR_VAR(0, 13, 19): var = msg->get_tot_prec1_var(); break;
        }
        if (var)
            subset->store_variable(prec_code, *var);
        else
            subset->store_variable_undef(prec_code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        const Msg& msg = *msgs[0];

        // Use the best kind of precipitation found in the message to encode
        if (msg.get_tot_prec24_var() != NULL)
            prec_code = WR_VAR(0, 13, 23);
        else if (msg.get_tot_prec12_var() != NULL)
            prec_code = WR_VAR(0, 13, 22);
        else if (msg.get_tot_prec6_var() != NULL)
            prec_code = WR_VAR(0, 13, 21);
        else if (msg.get_tot_prec3_var() != NULL)
            prec_code = WR_VAR(0, 13, 20);
        else if (msg.get_tot_prec1_var() != NULL)
            prec_code = WR_VAR(0, 13, 19);
        else
            prec_code = WR_VAR(0, 13, 23);

        bulletin.type = 0;
        bulletin.subtype = 255;
        bulletin.localsubtype = 1;
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1,  1), DBA_MSG_BLOCK);
        /*  1 */ add(WR_VAR(0,  1,  2), DBA_MSG_STATION);
        /*  2 */ add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        /*  3 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  4 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  5 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  6 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  7 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  8 */ add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        /*  9 */ add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
        /* 10 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT);
        /* 11 */ add(WR_VAR(0, 10,  4), DBA_MSG_PRESS);
    }
};

struct SynopLand : public Synop
{
    SynopLand(const Exporter::Options& opts, const Msgs& msgs)
        : Synop(opts, msgs) {}

    virtual const char* name() const { "synopland"; }
    virtual const char* description() const { "Synop Land"; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Synop::setupBulletin(bulletin);

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  7,  5));
        bulletin.datadesc.push_back(prec_code);
        bulletin.datadesc.push_back(WR_VAR(0, 13, 13));
        if (!is_crex)
        {
            bulletin.datadesc.push_back(WR_VAR(2, 22,  0));
            bulletin.datadesc.push_back(WR_VAR(1,  1, 49));
            bulletin.datadesc.push_back(WR_VAR(0, 31, 31));
            bulletin.datadesc.push_back(WR_VAR(0,  1, 31));
            bulletin.datadesc.push_back(WR_VAR(0,  1, 32));
            bulletin.datadesc.push_back(WR_VAR(1,  1, 49));
            bulletin.datadesc.push_back(WR_VAR(0, 33,  7));
        }

        bulletin.load_tables();
	}

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Synop::to_subset(msg, subset);
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
        /* 22 */ add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1);
        /* 23 */ add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2);
        /* 24 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 25 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 26 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 27 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 28 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 29 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 30 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        /* 31 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, 1), Trange::instant());
        /* 32 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_N1);
        /* 33 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_C1);
        /* 34 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_H1);
        /* 35 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, 2), Trange::instant());
        /* 36 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_N2);
        /* 37 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_C2);
        /* 38 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_H2);
        /* 39 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, 3), Trange::instant());
        /* 40 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_N3);
        /* 41 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_C3);
        /* 42 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_H3);
        /* 43 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, 4), Trange::instant());
        /* 44 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_N4);
        /* 45 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_C4);
        /* 46 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_H4);
        /* 47 */ add_prec();
        /* 48 */ add(WR_VAR(0, 13, 13), DBA_MSG_TOT_SNOW);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 49);
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

struct SynopLandHigh : public Synop
{
    SynopLandHigh(const Exporter::Options& opts, const Msgs& msgs)
        : Synop(opts, msgs) {}

    virtual const char* name() const { "synoplandhigh"; }
    virtual const char* description() const { "Synop land high level station"; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Synop::setupBulletin(bulletin);

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  7,  7));
        bulletin.datadesc.push_back(prec_code);
        bulletin.datadesc.push_back(WR_VAR(0, 13, 13));
        if (!is_crex)
        {
            bulletin.datadesc.push_back(WR_VAR(2, 22,  0));
            bulletin.datadesc.push_back(WR_VAR(1,  1, 34));
            bulletin.datadesc.push_back(WR_VAR(0, 31, 31));
            bulletin.datadesc.push_back(WR_VAR(0,  1, 31));
            bulletin.datadesc.push_back(WR_VAR(0,  1, 32));
            bulletin.datadesc.push_back(WR_VAR(1,  1, 34));
            bulletin.datadesc.push_back(WR_VAR(0, 33,  7));
        }

        bulletin.load_tables();
	}

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Synop::to_subset(msg, subset);
        /* 12 */ add(WR_VAR(0,  7,  4), DBA_MSG_ISOBARIC_SURFACE);
        /* 13 */ add(WR_VAR(0, 10,  3), DBA_MSG_GEOPOTENTIAL);
        /* 14 */ add(WR_VAR(0, 10, 61), DBA_MSG_PRESS_3H);
        /* 15 */ add(WR_VAR(0, 10, 63), DBA_MSG_PRESS_TEND);
        /* 16 */ add(WR_VAR(0, 11, 11), DBA_MSG_WIND_DIR);
        /* 17 */ add(WR_VAR(0, 11, 12), DBA_MSG_WIND_SPEED);
        /* 18 */ add(WR_VAR(0, 12,  4), DBA_MSG_TEMP_2M);
        /* 19 */ add(WR_VAR(0, 12,  6), DBA_MSG_DEWPOINT_2M);
        /* 20 */ add(WR_VAR(0, 13,  3), DBA_MSG_HUMIDITY);
        /* 21 */ add(WR_VAR(0, 20,  1), DBA_MSG_VISIBILITY);
        /* 22 */ add(WR_VAR(0, 20,  3), DBA_MSG_PRES_WTR);
        /* 23 */ add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1);
        /* 24 */ add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2);
        /* 25 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 26 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 27 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 28 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 29 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 30 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 31 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        /* 32 */ add_prec();
        /* 33 */ add(WR_VAR(0, 13, 13), DBA_MSG_TOT_SNOW);

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

// Same as SynopLandHigh but just with a different local subtype
struct SynopAuto : public SynopLandHigh
{
    SynopAuto(const Exporter::Options& opts, const Msgs& msgs)
        : SynopLandHigh(opts, msgs) {}

    virtual const char* name() const { "synoplandauto"; }
    virtual const char* description() const { "Synop land automatic station"; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        SynopLandHigh::setupBulletin(bulletin);

        bulletin.localsubtype = 3;
	}
};

struct SynopFactory : public TemplateFactory
{
    SynopFactory() { name = SYNOP_NAME; description = SYNOP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        // Scan msgs and pick the right one
        const Msg& msg = *msgs[0];
        const Var* var = msg.get_st_type_var();
        if (var != NULL && var->enqi() == 1)
            return auto_ptr<Template>(new SynopAuto(opts, msgs));
        else if ((var = msg.get_geopotential_var()) != NULL)
            return auto_ptr<Template>(new SynopLandHigh(opts, msgs));
        else
            return auto_ptr<Template>(new SynopLand(opts, msgs));
    }
};

struct SynopLandFactory : public TemplateFactory
{
    SynopLandFactory() { name = SYNOP_LAND_NAME; description = SYNOP_LAND_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new SynopLand(opts, msgs));
    }
};

struct SynopLandHighFactory : public TemplateFactory
{
    SynopLandHighFactory() { name = SYNOP_LAND_HIGH_NAME; description = SYNOP_LAND_HIGH_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new SynopLandHigh(opts, msgs));
    }
};

struct SynopAutoFactory : public TemplateFactory
{
    SynopAutoFactory() { name = SYNOP_AUTO_NAME; description = SYNOP_AUTO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new SynopAuto(opts, msgs));
    }
};


} // anonymous namespace

void register_synop(TemplateRegistry& r)
{
static const TemplateFactory* synop = NULL;
static const TemplateFactory* synopland = NULL;
static const TemplateFactory* synoplandhigh = NULL;
static const TemplateFactory* synopauto = NULL;

    if (!synop) synop = new SynopFactory;
    if (!synopland) synopland = new SynopLandFactory;
    if (!synoplandhigh) synoplandhigh = new SynopLandHighFactory;
    if (!synopauto) synopauto = new SynopAutoFactory;
 
    r.register_factory(synop);
    r.register_factory(synopland);
    r.register_factory(synoplandhigh);
    r.register_factory(synopauto);
}

}
}
}
/* vim:set ts=4 sw=4: */
