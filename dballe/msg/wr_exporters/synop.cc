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

#warning remove when done with debugging
#include <iostream>

using namespace wreport;
using namespace std;

#define SYNOP_NAME "synop"
#define SYNOP_DESC "Synop (autodetect)"

#define SYNOP_OLD_NAME "synop-old"
#define SYNOP_OLD_DESC "Synop old ECMWF (autodetect) (0.1)"

#define SYNOP_GTS_NAME "synop-gts"
#define SYNOP_GTS_DESC "Synop GTS (0.1)"

#define SYNOP_LAND_NAME "synop-land"
#define SYNOP_LAND_DESC "Synop land (0.1)"

#define SYNOP_LAND_HIGH_NAME "synop-land-high"
#define SYNOP_LAND_HIGH_DESC "Synop land high level station (0.1)"

#define SYNOP_AUTO_NAME "synop-auto"
#define SYNOP_AUTO_DESC "Synop land auto (0.3)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

struct ContextFinder
{
    struct Entry
    {
        Varcode code;
        const Var* var;
        Entry(Varcode code) : code(code), var(0) {}
        void add(Subset& subset)
        {
            if (var)
                subset.store_variable(*var);
            else
                subset.store_variable_undef(code);
        }
        void add(Subset& subset, Varcode as_code)
        {
            if (var)
                subset.store_variable(as_code, *var);
            else
                subset.store_variable_undef(as_code);
        }
    };

    const Msg& msg;
    const msg::Context* ctx;
    vector<Entry> vars;

    ContextFinder(const Msg& msg) : msg(msg), ctx(0)
    {
    }

    void add_var(Varcode code)
    {
        vars.push_back(Entry(code));
    }
    void add_var(int shortcut)
    {
        add_var(shortcutTable[shortcut].code);
    }

    bool scan_vars_in_context(const msg::Context& c)
    {
        bool found = false;
        for (vector<Entry>::iterator i = vars.begin();
                i != vars.end(); ++i)
        {
            if (const Var* var = c.find(i->code))
            {
                found = true;
                i->var = var;
            }
        }
        return found;
    }

    bool find_in_level(int ltype1, int or_shortcut=-1)
    {
        bool found = false;
        if (or_shortcut != -1)
        {
            if (const msg::Context* c = msg.find_context_by_id(or_shortcut))
                if (scan_vars_in_context(*c))
                {
                    ctx = c;
                    found = true;
                }
        }
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                !found && i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->level.ltype1 != ltype1) continue;
            if (scan_vars_in_context(*c))
            {
                ctx = c;
                found = true;
            }
        }
        return found;
    }

    const Var* find_first_attr(Varcode code) const
    {
        for (vector<Entry>::const_iterator i = vars.begin();
                i != vars.end(); ++i)
        {
            if (!i->var) continue;
            if (const Var* a = i->var->enqa(code))
                return a;
        }
        return NULL;
    }

    void add_found_sensor_height(Subset& subset)
    {
        if (!ctx)
        {
            subset.store_variable_undef(WR_VAR(0, 7, 32));
            return;
        }

        // Check the attributes to see if we're exporting a message
        // imported with the 'simplified' method
        if (const Var* a = find_first_attr(WR_VAR(0, 7, 32)))
            subset.store_variable_d(WR_VAR(0, 7, 32), a->enqd());
        else if (ctx->level.ltype1 == 1)
            // Ground level
            subset.store_variable_d(WR_VAR(0, 7, 32), 0);
        else if (ctx->level.ltype1 == 103)
            // Height above ground level
            subset.store_variable_d(WR_VAR(0, 7, 32), double(ctx->level.l1) / 1000.0);
        else
            error_consistency::throwf("Cannot add sensor height from unsupported level type %d", ctx->level.ltype1);
    }
};


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

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

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

    virtual const char* name() const { return SYNOP_LAND_NAME; }
    virtual const char* description() const { return SYNOP_LAND_DESC; }

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

    virtual const char* name() const { return SYNOP_LAND_HIGH_NAME; }
    virtual const char* description() const { return SYNOP_LAND_HIGH_DESC; }

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

    virtual const char* name() const { return SYNOP_AUTO_NAME; }
    virtual const char* description() const { return SYNOP_AUTO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        SynopLandHigh::setupBulletin(bulletin);

        bulletin.localsubtype = 3;
    }
};

struct SynopGTS : public Template
{
    bool is_crex;
    const msg::Context* c_sunshine1;
    const msg::Context* c_sunshine2;
    const msg::Context* c_prec1;
    const msg::Context* c_prec2;
    const msg::Context* c_wind;
    const msg::Context* c_gust1;
    const msg::Context* c_gust2;

    SynopGTS(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return SYNOP_GTS_NAME; }
    virtual const char* description() const { return SYNOP_GTS_DESC; }

    void add(Varcode code, int shortcut)
    {
        const Var* var = msg->find_by_id(shortcut);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add(Varcode code, const msg::Context& ctx, int shortcut)
    {
        const Var* var = ctx.find_by_id(shortcut);
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

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 0;
        bulletin.subtype = 255;
        bulletin.localsubtype = 1;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3, 7, 80));
        bulletin.load_tables();
    }

    // D02031  Pressure data
    void do_D02031(const Msg& msg, wreport::Subset& subset)
    {
        add(WR_VAR(0, 10,  4), DBA_MSG_PRESS);
        add(WR_VAR(0, 10, 51), DBA_MSG_PRESS_MSL);
        add(WR_VAR(0, 10, 61), DBA_MSG_PRESS_3H);
        add(WR_VAR(0, 10, 63), DBA_MSG_PRESS_TEND);
        add(WR_VAR(0, 10, 62), DBA_MSG_PRESS_24H);

        // Find pressure level of geopotential
        ContextFinder finder(msg);
        finder.add_var(WR_VAR(0, 10, 9));
        if (finder.find_in_level(100))
            subset.store_variable_d(WR_VAR(0, 7, 4), finder.ctx->level.l1);
        else
            subset.store_variable_undef(WR_VAR(0,  7, 4));
        finder.vars[0].add(subset);
    }

    // D02035  Basis synoptic "instantaneous" data
    void do_D02035(const Msg& msg, wreport::Subset& subset)
    {
        //   Temperature and humidity data
        {
            ContextFinder finder(msg);
            finder.add_var(DBA_MSG_TEMP_2M);
            finder.add_var(DBA_MSG_DEWPOINT_2M);
            finder.add_var(DBA_MSG_HUMIDITY);
            finder.find_in_level(103);
            finder.add_found_sensor_height(subset);
            finder.vars[0].add(subset, WR_VAR(0, 12, 101));
            finder.vars[1].add(subset, WR_VAR(0, 12, 103));
            finder.vars[2].add(subset, WR_VAR(0, 13,   3));
        }

        //   Visibility data
        {
            ContextFinder finder(msg);
            finder.add_var(DBA_MSG_VISIBILITY);
            finder.find_in_level(103, DBA_MSG_VISIBILITY);
            finder.add_found_sensor_height(subset);
            finder.vars[0].add(subset, WR_VAR(0, 20, 1));
        }

        //   Precipitation past 24 hours
        {
            ContextFinder finder(msg);
            finder.add_var(DBA_MSG_TOT_PREC24);
            finder.find_in_level(103, DBA_MSG_TOT_PREC24);
            finder.add_found_sensor_height(subset);
            finder.vars[0].add(subset, WR_VAR(0, 13, 23));
            subset.store_variable_undef(WR_VAR(0, 7, 32));
        }

        //   Cloud data
        {
            add(WR_VAR(0, 20,  10), DBA_MSG_CLOUD_N);
            add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
            add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
            add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
            add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
            add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
            add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);

            //   Individual cloud layers or masses
            int max_cloud_group = 0;
            for (int i = 1; ; ++i)
            {
                if (const msg::Context* c = msg.find_context(Level::cloud(259, i), Trange::instant()))
                {
                    if (c->find(WR_VAR(0, 20, 17)))
                        break;
                    max_cloud_group = i;
                } else if (i > 3)
                    break;
            }

            // Number of individual cloud layers or masses
            subset.store_variable_i(WR_VAR(0, 1, 31), max_cloud_group);
            for (int i = 1; i <= max_cloud_group; ++i)
            {
                add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, i), Trange::instant());
                if (const msg::Context* c = msg.find_context(Level::cloud(259, i), Trange::instant()))
                {
                    add(WR_VAR(0, 20, 11), *c, DBA_MSG_CLOUD_N1);
                    add(WR_VAR(0, 20, 12), *c, DBA_MSG_CLOUD_C1);
                    add(WR_VAR(0, 20, 13), *c, DBA_MSG_CLOUD_H1);
                } else {
                    subset.store_variable_undef(WR_VAR(0, 20, 11));
                    subset.store_variable_undef(WR_VAR(0, 20, 12));
                    subset.store_variable_undef(WR_VAR(0, 20, 13));
                }
            }
        }
    }

    // D02036  Clouds with bases below station level
    void do_D02036(const Msg& msg, wreport::Subset& subset)
    {
#warning TODO
        subset.store_variable_i(WR_VAR(0, 1, 31), 0);
        for (int i = 0; i < 0 /* TODO */; ++i)
        {
            /*
            add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(259, TODO), Trange::instant());
            add(WR_VAR(0, 20, 11), TODO);
            add(WR_VAR(0, 20, 12), TODO);
            add(WR_VAR(0, 20, 14), TODO);
            add(WR_VAR(0, 20, 17), TODO);
            */
        }
    }

    // D02047  Direction of cloud drift
    void do_D02047(const Msg& msg, wreport::Subset& subset)
    {
        // D02047  Direction of cloud drift
        for (int i = 1; i <= 3; ++i)
        {
            if (const msg::Context* c = msg.find_context(Level::cloud(260, i), Trange::instant()))
            {
                if (const Var* var = c->find(WR_VAR(0,  8,  2)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0,  8,  2));

                if (const Var* var = c->find(WR_VAR(0, 20, 54)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 20, 54));
            } else {
                subset.store_variable_undef(WR_VAR(0,  8,  2));
                subset.store_variable_undef(WR_VAR(0, 20, 54));
            }
        }
    }

    // D02048  Direction and elevation of cloud
    void do_D02048(const Msg& msg, wreport::Subset& subset)
    {
        if (const msg::Context* c = msg.find_context(Level::cloud(262, 0), Trange::instant()))
        {
            if (const Var* var = c->find(WR_VAR(0,  5, 21)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0,  5, 21));

            if (const Var* var = c->find(WR_VAR(0,  7, 21)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0,  7, 21));

            if (const Var* var = c->find(WR_VAR(0, 20, 12)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0, 20, 12));
        } else {
            subset.store_variable_undef(WR_VAR(0,  5, 21));
            subset.store_variable_undef(WR_VAR(0,  7, 21));
            subset.store_variable_undef(WR_VAR(0, 20, 12));
        }
        subset.store_variable_undef(WR_VAR(0,  5, 21));
        subset.store_variable_undef(WR_VAR(0,  7, 21));
    }

    // D02037  State of ground, snow depth, ground minimum temperature
    void do_D02037(const Msg& msg, wreport::Subset& subset)
    {
        add(WR_VAR(0, 20,  62), DBA_MSG_STATE_GROUND);
        add(WR_VAR(0, 13,  13), DBA_MSG_TOT_SNOW);
        if (const Var* var = msg.find(WR_VAR(0, 12, 121), Level(1), Trange(3, 0, 43200)))
            subset.store_variable(WR_VAR(0, 12, 113), *var);
        else
            subset.store_variable_undef(WR_VAR(0, 12, 113));
    }

    // D02043  Basic synoptic "period" data
    void do_D02043(const Msg& msg, wreport::Subset& subset)
    {
        //   Present and past weather
        add(WR_VAR(0, 20,  3), DBA_MSG_PRES_WTR);
        ContextFinder finder(msg);
        finder.add_var(DBA_MSG_PAST_WTR1);
        finder.add_var(DBA_MSG_PAST_WTR2);
        if (finder.find_in_level(1))
            subset.store_variable_d(WR_VAR(0, 4, 24), finder.ctx->trange.p2);
        else
            subset.store_variable_undef(WR_VAR(0, 4, 24));
        finder.vars[0].add(subset, WR_VAR(0, 20, 4));
        finder.vars[1].add(subset, WR_VAR(0, 20, 5));

        //   Sunshine data (form 1 hour and 24 hour period)
        if (c_sunshine1)
        {
            subset.store_variable_d(WR_VAR(0,  4, 24), c_sunshine1->trange.p2);
            if (const Var* var = c_sunshine1->find(WR_VAR(0, 14, 31)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0, 14, 31));
        } else {
            subset.store_variable_undef(WR_VAR(0,  4, 24));
            subset.store_variable_undef(WR_VAR(0, 14, 31));
        }
        if (c_sunshine2)
        {
            subset.store_variable_d(WR_VAR(0,  4, 24), c_sunshine2->trange.p2);
            if (const Var* var = c_sunshine2->find(WR_VAR(0, 14, 31)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0, 14, 31));
        } else {
            subset.store_variable_undef(WR_VAR(0,  4, 24));
            subset.store_variable_undef(WR_VAR(0, 14, 31));
        }

        //   Precipitation measurement
        if (c_prec1)
        {
            double h = c_prec1->level.ltype1 == 1 ? 0.0 : c_prec1->level.l1 / 1000.0;
            if (c_prec1->level.ltype1 == 1)
                if (const Var* var = c_prec1->find(WR_VAR(0, 13, 11)))
                    if (const Var* a = var->enqa(WR_VAR(0, 13, 11)))
                        h = a->enqd();
            if (h == 0.0)
                subset.store_variable_undef(WR_VAR(0,  7, 32));
            else
                subset.store_variable_d(WR_VAR(0,  7, 32), h);

            subset.store_variable_d(WR_VAR(0,  4, 24), c_prec1->trange.p2 / 3600);
            if (const Var* var = c_prec1->find(WR_VAR(0, 13, 11)))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0, 13, 11));
            if (c_prec2)
            {
                subset.store_variable_d(WR_VAR(0,  4, 24), c_prec2->trange.p2 / 3600);
                if (const Var* var = c_prec2->find(WR_VAR(0, 13, 11)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 13, 11));
            } else {
                subset.store_variable_undef(WR_VAR(0,  4, 24));
                subset.store_variable_undef(WR_VAR(0, 13, 11));
            }
        } else {
            subset.store_variable_undef(WR_VAR(0,  7, 32));
            subset.store_variable_undef(WR_VAR(0,  4, 24));
            subset.store_variable_undef(WR_VAR(0, 13, 11));
            subset.store_variable_undef(WR_VAR(0,  4, 24));
            subset.store_variable_undef(WR_VAR(0, 13, 11));
        }

        //   Extreme temperature data
#warning TODO
        subset.store_variable_undef(WR_VAR(0,  7, 32)); // TODO
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0, 12, 111)); // TODO
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0, 12, 112)); // TODO

        //   Wind data
        if (c_wind || c_gust1 || c_gust2)
        {
            const msg::Context* c_first = c_wind ? c_wind : c_gust1 ? c_gust1 : c_gust2;
            bool has_h = false;
            if (c_first->level.l1 == 10 * 1000)
            {
                // override h based on attributes of B11001, B11002, B11043 or B11041 in c_first
                for (std::vector<wreport::Var*>::const_iterator i = c_first->data.begin();
                        i != c_first->data.end(); ++i)
                {
                    const Var& var = **i;
                    switch (var.code())
                    {
                        case WR_VAR(0, 11, 1):
                        case WR_VAR(0, 11, 2):
                        case WR_VAR(0, 11, 41):
                        case WR_VAR(0, 11, 43):
                            if (const Var* a = var.enqa(WR_VAR(0, 7, 32)))
                            {
                                subset.store_variable(*a);
                                has_h = true;
                            }
                            break;
                    }
                }
            }
            if (!has_h)
                subset.store_variable_d(WR_VAR(0,  7, 32), c_first->level.l1 / 1000.0);

            if (const Var* var = msg.find(WR_VAR(0,  2,  2), c_first->level, Trange::instant()))
                subset.store_variable(*var);
            else
                subset.store_variable_undef(WR_VAR(0,  2,  2));

            subset.store_variable_i(WR_VAR(0,  8, 21), 2);

            if (c_wind)
            {
                if (c_wind->trange.pind == 0)
                    subset.store_variable_d(WR_VAR(0,  4, 25), -c_wind->trange.p2 / 60.0);
                else
                    subset.store_variable_undef(WR_VAR(0,  4, 25));
                if (const Var* var = c_wind->find(WR_VAR(0, 11, 1)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11,  1));
                if (const Var* var = c_wind->find(WR_VAR(0, 11, 2)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11,  2));
            } else {
                subset.store_variable_undef(WR_VAR(0,  4, 25));
                subset.store_variable_undef(WR_VAR(0, 11,  1));
                subset.store_variable_undef(WR_VAR(0, 11,  2));
            }
            subset.store_variable_undef(WR_VAR(0,  8, 21));

            if (c_gust1)
            {
                if (c_gust1->trange.pind == 0)
                    subset.store_variable_d(WR_VAR(0,  4, 25), -c_gust1->trange.p2 / 60.0);
                else
                    subset.store_variable_undef(WR_VAR(0,  4, 25));
                if (const Var* var = c_gust1->find(WR_VAR(0, 11, 43)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11,  43));
                if (const Var* var = c_gust1->find(WR_VAR(0, 11, 41)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11, 41));
            } else {
                subset.store_variable_undef(WR_VAR(0,  4, 25));
                subset.store_variable_undef(WR_VAR(0, 11, 43));
                subset.store_variable_undef(WR_VAR(0, 11, 41));
            }
            if (c_gust2)
            {
                if (c_gust2->trange.pind == 0)
                    subset.store_variable_d(WR_VAR(0,  4, 25), -c_gust2->trange.p2 / 60.0);
                else
                    subset.store_variable_undef(WR_VAR(0,  4, 25));
                if (const Var* var = c_gust2->find(WR_VAR(0, 11, 43)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11,  43));
                if (const Var* var = c_gust2->find(WR_VAR(0, 11, 41)))
                    subset.store_variable(*var);
                else
                    subset.store_variable_undef(WR_VAR(0, 11, 41));
            } else {
                subset.store_variable_undef(WR_VAR(0,  4, 25));
                subset.store_variable_undef(WR_VAR(0, 11, 43));
                subset.store_variable_undef(WR_VAR(0, 11, 41));
            }
        } else {
            subset.store_variable_undef(WR_VAR(0,  7, 32));
            subset.store_variable_undef(WR_VAR(0,  2,  2));
            subset.store_variable_i(WR_VAR(0,  8, 21), 2);
            subset.store_variable_undef(WR_VAR(0,  4, 25));
            subset.store_variable_undef(WR_VAR(0, 11,  1));
            subset.store_variable_undef(WR_VAR(0, 11,  2));
            subset.store_variable_undef(WR_VAR(0,  8, 21));
            for (int i = 1; i <= 2; ++i)
            {
                subset.store_variable_undef(WR_VAR(0,  4, 25));
                subset.store_variable_undef(WR_VAR(0, 11, 43));
                subset.store_variable_undef(WR_VAR(0, 11, 41));
            }
        }
        subset.store_variable_undef(WR_VAR(0,  7, 32));
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        c_sunshine1 = NULL;
        c_sunshine2 = NULL;
        c_prec1 = NULL;
        c_prec2 = NULL;
        c_wind = NULL;
        c_gust1 = NULL;
        c_gust2 = NULL;

        // Scan message finding context for the data that follow
        for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                i != msg.data.end(); ++i)
        {
            const msg::Context* c = *i;
            if (c->level == Level(1))
            {
                if (c->trange.pind == 1 && c->trange.p1 == 0)
                {
                    // msg->set(var, WR_VAR(0, 14, 31), Level(1), Trange(1, 0, time_period));
                    if (c->find(WR_VAR(0, 14, 31)))
                    {
                        if (!c_sunshine1)
                            c_sunshine1 = c;
                        else if (!c_sunshine2)
                            c_sunshine2 = c;
                    }

                    // set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, time_period));
                    if (c->find(WR_VAR(0, 13, 11)))
                    {
                        if (!c_prec1)
                            c_prec1 = c;
                        else if (!c_prec2)
                            c_prec2 = c;
                    }
                }
            } else if (c->level.ltype1 == 103) {
                if (c->trange.pind == 1 && c->trange.p1 == 0)
                {
                    // set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, time_period));
                    if (c->find(WR_VAR(0, 13, 11)))
                    {
                        if (!c_prec1)
                            c_prec1 = c;
                        else if (!c_prec2)
                            c_prec2 = c;
                    }
                }
                if (c->find(WR_VAR(0, 11, 1)) || c->find(WR_VAR(0, 11, 2)))
                    if (!c_wind)
                        c_wind = c;
                if (c->find(WR_VAR(0, 11, 41)) || c->find(WR_VAR(0, 11, 43)))
                {
                    if (!c_gust1)
                        c_gust1 = c;
                    else if (!c_gust2)
                        c_gust2 = c;
                }
            }
        }

        // D01090  Fixed surface identification, time, horizontal and vertical coordinates
        add(WR_VAR(0,  1,  1), DBA_MSG_BLOCK);
        add(WR_VAR(0,  1,  2), DBA_MSG_STATION);
        // Set station name, truncating it if it's too long
        if (const wreport::Var* var = msg.get_st_name_var())
        {
            Varinfo info = subset.btable->query(WR_VAR(0,  1, 15));
            Var name(info);
            if (var->value())
                name.setc_truncate(var->value());
            subset.store_variable(name);
        }
        else
            subset.store_variable_undef(WR_VAR(0,  1, 15));
        add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
        add(WR_VAR(0,  7, 30), DBA_MSG_HEIGHT);
        add(WR_VAR(0,  7, 31), DBA_MSG_HEIGHT_BARO);
        // D02031  Pressure data
        do_D02031(msg, subset);
        // D02035  Basis synoptic "instantaneous" data
        do_D02035(msg, subset);
        // D02036  Clouds with bases below station level
        do_D02036(msg, subset);
        // D02047  Direction of cloud drift
        do_D02047(msg, subset);
        // B08002
        subset.store_variable_undef(WR_VAR(0, 8, 2));
        // D02048  Direction and elevation of cloud
        do_D02048(msg, subset);
        // D02037  State of ground, snow depth, ground minimum temperature
        do_D02037(msg, subset);
        // D02043  Basic synoptic "period" data
        do_D02043(msg, subset);
#warning TODO

        // D02044  Evaporation data
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0,  2,  4)); // TODO
        subset.store_variable_undef(WR_VAR(0, 13, 33)); // TODO
        // D02045  Radiation data (1 hour period)
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14,  2)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14,  4)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 16)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 28)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 29)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 30)); // TODO
        // D02045  Radiation data (24 hour period)
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14,  2)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14,  4)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 16)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 28)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 29)); // TODO
        subset.store_variable_undef(WR_VAR(0, 14, 30)); // TODO
        // D02046  Temperature change
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0,  4, 24)); // TODO
        subset.store_variable_undef(WR_VAR(0, 12, 49)); // TODO
    }
};


struct SynopGTSFactory : public TemplateFactory
{
    SynopGTSFactory() { name = SYNOP_GTS_NAME; description = SYNOP_GTS_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        // Scan msgs and pick the right one
        return auto_ptr<Template>(new SynopGTS(opts, msgs));
    }
};

struct SynopOldFactory : public TemplateFactory
{
    SynopOldFactory() { name = SYNOP_OLD_NAME; description = SYNOP_OLD_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        const Msg& msg = *msgs[0];
        const Var* var = msg.get_st_type_var();
        if (var != NULL && var->enqi() == 0)
            return auto_ptr<Template>(new SynopAuto(opts, msgs));
        else if ((var = msg.get_geopotential_var()) != NULL)
            return auto_ptr<Template>(new SynopLandHigh(opts, msgs));
        else
            return auto_ptr<Template>(new SynopLand(opts, msgs));
    }
};

struct SynopFactory : public SynopOldFactory
{
    SynopFactory() { name = SYNOP_NAME; description = SYNOP_DESC; }
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
static const TemplateFactory* synopgts = NULL;
static const TemplateFactory* synopold = NULL;
static const TemplateFactory* synopland = NULL;
static const TemplateFactory* synoplandhigh = NULL;
static const TemplateFactory* synopauto = NULL;

    if (!synop) synop = new SynopFactory;
    if (!synopgts) synopgts = new SynopGTSFactory;
    if (!synopold) synopold = new SynopOldFactory;
    if (!synopland) synopland = new SynopLandFactory;
    if (!synoplandhigh) synoplandhigh = new SynopLandHighFactory;
    if (!synopauto) synopauto = new SynopAutoFactory;

    r.register_factory(synop);
    r.register_factory(synopgts);
    r.register_factory(synopold);
    r.register_factory(synopland);
    r.register_factory(synoplandhigh);
    r.register_factory(synopauto);
}

}
}
}
/* vim:set ts=4 sw=4: */
